#include "sound.hpp"
#include <cstdlib>

struct [[gnu::packed]] RiffHeader
{
    u32 riff_magic;
    u32 file_size;
    u32 wav_magic;
};

struct [[gnu::packed]] RiffSection
{
    u32 magic;
    u32 length;
};

struct [[gnu::packed]] FmtSection
{
    u16 format;
    u16 channels;
    u32 sample_rate;
    u32 second_size; // sample_rate * byte_per_sampel * channels
    u16 stride;      // byte_per_sample * channels
    u16 bit_per_sample;
    u32 data_magic;
    u8 data[];
};

static constexpr u32 FMT_HEADER = 0x20746D66;
static constexpr u32 DATA_HEADER = 0x61746164;

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
        return std::byteswap(*reinterpret_cast<const u32*>(
            reinterpret_cast<const u8*>(data) + off));
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
            samples16[i] = std::byteswap(samples16[i]);
        }
    }

    return Sound(std::move(vec), 2, sizeof(u16), 48000, sample_count,
                 sample_loop, target);
}

std::expected<Sound, SoundError> Sound::fromWave(const void*, size_t)
{
    return std::unexpected(SoundError_InvalidOrUnsupportedWaveFile);
}
