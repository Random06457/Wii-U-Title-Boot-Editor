#include "sound.hpp"
#include <cstdlib>
#include <cstring>
#include <string_view>

struct [[gnu::packed]] RiffHeader
{
    u32 riff_magic;
    u32 file_size;
    u32 wav_magic;
};

struct [[gnu::packed]] RiffSection
{
    u32 magic;
    u32 size;
};

struct [[gnu::packed]] FmtSection
{
    u16 format;
    u16 channels;
    u32 sample_rate;
    u32 second_size; // sample_rate * byte_per_sampel * channels
    u16 stride;      // byte_per_sample * channels
    u16 bit_per_sample;
};

static constexpr u16 WAVE_FORMAT_PCM = 1;

#ifndef __cpp_lib_constexpr_string_view
#error "constexpr string_view unsupported"
#endif

static constexpr u32 MAGIC(char a, char b, char c, char d)
{
    if constexpr (std::endian::native == std::endian::little)
        return d << 24 | c << 16 | b << 8 | a;
    else
        return a << 24 | b << 16 | c << 8 | d;
}
static constexpr u32 MAGIC(std::string_view s)
{
    return MAGIC(s[0], s[1], s[2], s[3]);
}

static constexpr u32 RIFF_MAGIC = MAGIC("RIFF");
static constexpr u32 FMT_MAGIC = MAGIC("fmt ");
static constexpr u32 DATA_MAGIC = MAGIC("data");
static constexpr u32 WAVE_MAGIC = MAGIC("WAVE");

float Sound::sampleNormalized(size_t idx, size_t channel) const
{
    if (idx >= sampleCount())
    {
        idx = 0;
    }
    if (bitsPerSample() == 8)
    {
        return (float)sample<s8>(idx, channel) / (float)0x80;
    }
    else if (bitsPerSample() == 16)
    {
        return (float)sample<s16>(idx, channel) / (float)0x8000;
    }
    else if (bitsPerSample() == 32)
    {
        return (float)sample<s32>(idx, channel) / (float)0x80000000;
    }

    std::abort();
}

Sound::Sound(std::vector<u8>&& sample_data, size_t channels,
             size_t byte_per_sample, size_t sample_rate, size_t sample_count,
             size_t loop_sample, SoundTarget target) :
    m_sample_data(std::move(sample_data)),
    m_channels(channels),
    m_byte_per_sample(byte_per_sample),
    m_sample_rate(sample_rate),
    m_sample_count(sample_count),
    m_loop_sample(loop_sample),
    m_target(target)
{
}

Sound& Sound::operator=(Sound&& other)
{
    m_sample_data = std::move(other.m_sample_data);
    m_channels = other.m_channels;
    m_byte_per_sample = other.m_byte_per_sample;
    m_sample_rate = other.m_sample_rate;
    m_sample_count = other.m_sample_count;
    m_loop_sample = other.m_loop_sample;
    m_target = other.m_target;
    return *this;
}

std::expected<Sound, SoundError> Sound::fromBtsnd(const void* data,
                                                  size_t data_size)
{
    auto read32 = [&data](size_t off)
    {
#ifdef __cpp_lib_byteswap
        return std::byteswap(*reinterpret_cast<const u32*>(
            reinterpret_cast<const u8*>(data) + off));
#else
        return __builtin_bswap32(*reinterpret_cast<const u32*>(
            reinterpret_cast<const u8*>(data) + off));
#endif
    };

    if (data_size < 8)
        return std::unexpected(SoundError_InvalidBtsndFile);

    auto target = static_cast<SoundTarget>(read32(0));
    size_t sample_loop = read32(4);
    size_t sample_count = (data_size - 8) / (2 * sizeof(u16));
    const u8* samples = reinterpret_cast<const u8*>(data) + 8;

    if (target > SoundTarget_Both)
        return std::unexpected(SoundError_InvalidBtsndFile);

    auto vec = std::vector<u8>(samples, samples + data_size - 8);

    {
        u16* samples16 = reinterpret_cast<u16*>(vec.data());
        for (size_t i = 0; i < vec.size() / sizeof(u16); i++)
        {
#ifdef __cpp_lib_byteswap
            samples16[i] = std::byteswap(samples16[i]);
#else
            samples16[i] = __builtin_bswap16(samples16[i]);
#endif
        }
    }

    return Sound(std::move(vec), 2, sizeof(u16), 48000, sample_count,
                 sample_loop, target);
}

std::expected<Sound, WaveFileError> Sound::fromWave(const void* data,
                                                    size_t data_size)
{
    if (data_size < sizeof(RiffHeader) + sizeof(RiffSection) +
                        sizeof(FmtSection) + sizeof(RiffSection))
        return std::unexpected(WaveFileError_FileTooSmall);

    size_t pos = 0;

    auto read = [&pos, &data]<typename T>() -> T*
    {
        auto ret = reinterpret_cast<T*>(reinterpret_cast<intptr_t>(data) + pos);
        pos += sizeof(T);
        return ret;
    };
    auto readVec = [&pos, &data]<typename T>(size_t n) -> std::vector<T>
    {
        auto ret = reinterpret_cast<T*>(reinterpret_cast<intptr_t>(data) + pos);
        pos += n;
        std::vector<T> vec(n);
        std::memcpy(vec.data(), ret, n);
        return vec;
    };

    auto riff_hdr = read.operator()<RiffHeader>();
    if (riff_hdr->riff_magic != RIFF_MAGIC)
        return std::unexpected(WaveFileError_InvalidHeader);
    if (riff_hdr->wav_magic != WAVE_MAGIC)
        return std::unexpected(WaveFileError_InvalidHeader);
    if (riff_hdr->file_size + 8 > data_size)
        return std::unexpected(WaveFileError_InvalidHeader);

    const FmtSection* fmt = nullptr;
    bool has_data = false;
    std::vector<u8> sample_data;

    while (pos < data_size)
    {
        auto section = read.operator()<RiffSection>();
        if (pos + section->size > data_size)
            return std::unexpected(WaveFileError_InvalidHeader);

        if (section->magic == FMT_MAGIC)
        {
            if (fmt != nullptr)
                return std::unexpected(WaveFileError_DuplicateSections);
            fmt = read.operator()<FmtSection>();
        }
        else if (section->magic == DATA_MAGIC)
        {
            if (sample_data.data() != nullptr)
                return std::unexpected(WaveFileError_DuplicateSections);
            sample_data = readVec.operator()<u8>(section->size);
            has_data = true;
        }
        else
        {
            pos += section->size;
        }
    }

    if (fmt == nullptr)
        return std::unexpected(WaveFileError_MissingSection);
    if (!has_data)
        return std::unexpected(WaveFileError_MissingSection);

    return Sound(std::move(sample_data), fmt->channels, fmt->bit_per_sample / 8,
                 fmt->sample_rate, sample_data.size() / fmt->stride, 0,
                 SoundTarget_Both);
}

std::vector<u8> Sound::toWave() const
{
    size_t pos = 0;
    std::vector<u8> ret;

    ret.reserve(sizeof(RiffHeader) + sizeof(RiffSection) + sizeof(FmtSection) +
                sizeof(RiffSection) + sampleDataSize());

    auto write = [&pos, &ret]<typename T>(const T& x)
    {
        size_t off = ret.size();
        ret.resize(off + sizeof(T));
        std::memcpy(ret.data() + off, &x, sizeof(T));
    };

    auto writeVec = [&pos, &ret]<typename T>(const std::vector<T>& x)
    {
        size_t off = ret.size();
        ret.resize(off + x.size() * sizeof(T));
        std::memcpy(ret.data() + off, x.data(), x.size() * sizeof(T));
    };

    auto getPtr = [&ret]<typename T>(size_t off) -> T*
    { return reinterpret_cast<T*>(ret.data() + off); };

    size_t riff_hdr_off = pos;

    // riff header
    write(RiffHeader{
        .riff_magic = RIFF_MAGIC,
        .file_size = 0,
        .wav_magic = WAVE_MAGIC,
    });

    // fmt header
    write(RiffSection{ .magic = FMT_MAGIC, .size = sizeof(FmtSection) });

    // fmt section
    write(FmtSection{
        .format = WAVE_FORMAT_PCM,
        .channels = 2,
        .sample_rate = static_cast<u32>(sampleRate()),
        .second_size = static_cast<u32>(sampleStride() * sampleRate()),
        .stride = static_cast<u16>(sampleStride()),
        .bit_per_sample = static_cast<u16>(bitsPerSample()),
    });

    // data header
    write(RiffSection{ .magic = DATA_MAGIC,
                       .size = static_cast<u32>(sampleDataSize()) });

    // data section
    writeVec(m_sample_data);

    // write file size
    getPtr.operator()<RiffHeader>(riff_hdr_off)->file_size =
        static_cast<u32>(pos - 8);

    return ret;
}
