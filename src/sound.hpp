#pragma once

#include <expected>
#include <vector>
#include "types.hpp"

enum SoundError
{
    SoundError_InvalidBtsndFile,
    SoundError_InvalidOrUnsupportedWaveFile,
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

    static std::expected<Sound, SoundError>
    fromBtsnd(const std::vector<u8>& data)
    {
        return fromBtsnd(reinterpret_cast<const void*>(data.data()),
                         data.size());
    }
    static std::expected<Sound, SoundError> fromBtsnd(const void* data,
                                                      size_t data_size);
    static std::expected<Sound, SoundError>
    fromWave(const std::vector<u8>& data)
    {
        return fromWave(reinterpret_cast<const void*>(data.data()),
                        data.size());
    }
    static std::expected<Sound, SoundError> fromWave(const void* data,
                                                     size_t data_size);

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

private:
    std::vector<u8> m_sample_data;
    size_t m_channels;
    size_t m_byte_per_sample;
    size_t m_sample_rate;
    size_t m_sample_count;
    size_t m_loop_sample;
    SoundTarget m_target;
};
