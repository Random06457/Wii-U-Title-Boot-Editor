#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "sound.hpp"

class SoundPlayer
{
public:
    SoundPlayer(const Sound& sound);
    ~SoundPlayer()
    {
        pause();
        SDL_CloseAudioDevice(m_audio_device);
    }

    void play() const { SDL_PauseAudioDevice(m_audio_device, 0); }
    void pause() const { SDL_PauseAudioDevice(m_audio_device, 1); }
    bool isPlaying() const
    {
        return SDL_GetAudioDeviceStatus(m_audio_device) == SDL_AUDIO_PLAYING;
    }
    const Sound& sound() const { return m_curr_sound; }
    size_t getBuffered() const { return m_curr_buffered; }
    size_t getCurrSample() const
    {
        return m_curr_buffered / m_curr_sound.sampleStride();
    }

    void setCurrSample(size_t sample)
    {
        m_curr_buffered = sample * m_curr_sound.sampleStride();
    }

    float getCurrTime() const
    {
        return (float)getCurrSample() / (float)m_curr_sound.sampleRate();
    }

    void setCurrTime(float t)
    {
        setCurrSample((size_t)(t * (float)m_curr_sound.sampleRate()));
    }

private:
    const Sound& m_curr_sound;
    size_t m_curr_buffered;
    SDL_AudioDeviceID m_audio_device;
};
