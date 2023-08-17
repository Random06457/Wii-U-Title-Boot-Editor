#include "sound_player.hpp"

#include <fmt/format.h>

SoundPlayer ::SoundPlayer(const Sound& sound) :
    m_curr_sound(sound),
    m_curr_buffered(0),
    m_audio_device(0)
{
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

    wanted.freq = (int)m_curr_sound.sampleRate();
    if (m_curr_sound.bitsPerSample() == 8)
        wanted.format = AUDIO_S8;
    else if (m_curr_sound.bitsPerSample() == 16)
        wanted.format = AUDIO_S16;
    else
        std::abort();
    wanted.channels = (Uint8)m_curr_sound.channels();
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
