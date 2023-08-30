#include "sound_player.hpp"

#include <fmt/format.h>

SoundPlayer ::SoundPlayer(const Sound* sound) :
    m_curr_sound(nullptr),
    m_curr_buffered(0),
    m_audio_device(0)
{
    setSound(sound);
}

void SoundPlayer::setSound(const Sound* sound)
{
    if (m_audio_device != 0)
        pause();

    m_curr_buffered = 0;

    // assign new sound
    auto old_sound = m_curr_sound;
    m_curr_sound = sound;

    // clear everything
    if (sound == nullptr)
    {
        if (m_audio_device != 0)
            SDL_CloseAudioDevice(m_audio_device);
        m_audio_device = 0;
        return;
    }

    // if the sound is in the same format, no need to create a new device
    if (old_sound != nullptr &&
        old_sound->bitsPerSample() == sound->bitsPerSample() &&
        old_sound->sampleRate() == sound->sampleRate() &&
        old_sound->channels() == sound->channels())
    {
        return;
    }

    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
    {
        if (SDL_Init(SDL_INIT_AUDIO) != 0)
        {
            fmt::print(stderr, "{}\n", SDL_GetError());
            std::abort();
        }
    }

    SDL_AudioSpec wanted;
    auto fill_audio = [](void* udata, Uint8* stream, int len)
    {
        auto x = reinterpret_cast<SoundPlayer*>(udata);

        size_t rest = x->sound().sampleDataSize() - x->m_curr_buffered;
        size_t to_copy = std::min<size_t>(len, rest);
        memcpy(stream, x->sound().sampleData<Uint8>(x->m_curr_buffered),
               to_copy);
        memset(stream + to_copy, 0, len - to_copy);
        x->m_curr_buffered += to_copy;

        if (rest == 0)
        {
            x->pause();
        }
    };

    wanted.freq = (int)m_curr_sound->sampleRate();
    if (m_curr_sound->bitsPerSample() == 8)
        wanted.format = AUDIO_U8;
    else if (m_curr_sound->bitsPerSample() == 16)
        wanted.format = AUDIO_S16;
    else
        std::abort();
    wanted.channels = (Uint8)m_curr_sound->channels();
    wanted.samples = 1024;
    wanted.callback = fill_audio;
    wanted.userdata = this;

    /* Open the audio device, forcing the desired format */
    if ((m_audio_device =
             SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0)) == 0)
    {
        fmt::print(stderr, "SDL_OpenAudio: {}\n", SDL_GetError());
        std::abort();
    }
}

SoundPlayer::~SoundPlayer()
{
    if (m_audio_device == 0)
        return;

    pause();
    SDL_CloseAudioDevice(m_audio_device);
}
