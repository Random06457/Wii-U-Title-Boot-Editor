#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <mutex>
#include "sound.hpp"

class SoundPlayer
{
public:
    SoundPlayer() : m_curr_sound(nullptr), m_curr_buffered(0), m_audio_device(0)
    {
    }
    SoundPlayer(const Sound* sound);
    ~SoundPlayer();

    void setSound(const Sound* sound);

    void play() const { SDL_PauseAudioDevice(m_audio_device, 0); }
    void pause() const { SDL_PauseAudioDevice(m_audio_device, 1); }
    bool isPlaying() const
    {
        return SDL_GetAudioDeviceStatus(m_audio_device) == SDL_AUDIO_PLAYING;
    }
    const Sound& sound() const { return *m_curr_sound; }
    size_t getBuffered()
    {
        std::lock_guard<std::mutex> lock(m_buffered_lock);
        return m_curr_buffered;
    }
    size_t getCurrSample()
    {
        return getBuffered() / m_curr_sound->sampleStride();
    }

    void setCurrSample(size_t sample)
    {
        std::lock_guard<std::mutex> lock(m_buffered_lock);
        m_curr_buffered = sample * m_curr_sound->sampleStride();
    }

    float getCurrTime()
    {
        return (float)getCurrSample() / (float)m_curr_sound->sampleRate();
    }

    void setCurrTime(float t)
    {
        setCurrSample((size_t)(t * (float)m_curr_sound->sampleRate()));
    }

private:
    const Sound* m_curr_sound;
    size_t m_curr_buffered;
    SDL_AudioDeviceID m_audio_device;
    std::mutex m_buffered_lock;
};
