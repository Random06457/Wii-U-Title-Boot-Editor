#pragma once

#include <SDL_audio.h>
#include <string>
#include <vector>
#include "types.hpp"
#include "utils.hpp"

enum SoundError
{
    SoundError_InvalidBtsndFile,
    SoundError_InvalidOrUnsupportedWaveFile,
};

enum WaveFileError
{
    WaveFileError_FileTooSmall,
    WaveFileError_InvalidHeader,
    WaveFileError_DuplicateSections,
    WaveFileError_MissingSection,
    WaveFileError_UnsupportedFormat,
    WaveFileError_LoadWaveFailed,
};

enum SoundTarget : u32
{
    SoundTarget_Tv = 0,
    SoundTarget_Drc = 1,
    SoundTarget_Both = 2,
};

class Sound
{
private:
    Sound(std::vector<u8>&& sample_data, size_t channels,
          size_t byte_per_sample, size_t sample_rate, size_t sample_count,
          size_t loop_sample, SoundTarget target);

public:
    Sound() = default;
    Sound(Sound&& other) { *this = std::move(other); }
    Sound& operator=(Sound&&);

    template<typename T>
    static auto fromBtsnd(const std::vector<T>& data)
        -> Result<Sound, SoundError>
    {
        return fromBtsnd(reinterpret_cast<const void*>(data.data()),
                         data.size() * sizeof(T));
    }
    static auto fromBtsnd(const void* data, size_t data_size)
        -> Result<Sound, SoundError>;
    template<typename T>
    static auto fromWave(const std::vector<T>& data,
                         SDL_AudioSpec* out_spec = nullptr)
        -> Result<Sound, WaveFileError>
    {
        return fromWave(reinterpret_cast<const void*>(data.data()),
                        data.size() * sizeof(T), out_spec);
    }
    static auto fromWave(const void* data, size_t data_size,
                         SDL_AudioSpec* out_spec = nullptr)
        -> Result<Sound, WaveFileError>;

    template<typename T>
    const T* sampleData(size_t off) const
    {
        return reinterpret_cast<const T*>(m_sample_data.data() + off);
    }

    size_t sampleDataSize() const { return sampleStride() * m_sample_count; }

    template<typename T>
    T sample(size_t idx, size_t channel = 0) const
    {
        return *sampleData<T>(idx * sampleStride() +
                              channel * m_byte_per_sample);
    }

    float sampleNormalized(size_t idx, size_t channel) const;

    size_t channels() const { return m_channels; }
    size_t bitsPerSample() const { return m_byte_per_sample * 8; }
    size_t bytesPerSample() const { return m_byte_per_sample; }
    size_t sampleRate() const { return m_sample_rate; }
    size_t sampleStride() const { return bytesPerSample() * channels(); }
    size_t sampleCount() const { return m_sample_count; }
    size_t loopSample() const { return m_loop_sample; }
    void setLoopSample(size_t sample) { m_loop_sample = sample; }
    SoundTarget target() const { return m_target; }
    void setTarget(SoundTarget target) { m_target = target; }

    float getDuration() const
    {
        return (float)sampleCount() / (float)sampleRate();
    }

    std::string formatName() const;

    std::vector<u8> toWave() const;
    std::vector<u8> toBtsnd() const;

private:
    std::vector<u8> m_sample_data;
    size_t m_channels;
    size_t m_byte_per_sample;
    size_t m_sample_rate;
    size_t m_sample_count;
    size_t m_loop_sample;
    SoundTarget m_target;
};
