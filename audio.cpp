#include "audio.h"
#include <iostream>

static sf::Music music;

void start_song(const char *filename)
{
    if (!music.OpenFromFile(filename))
    {
        std::cout << "Unable to open music file\n";
        return;
    }
    music.SetLoop(true);
    music.Play();
}

void stop_song()
{
    music.Stop();
};

void play_song()
{
    music.Play();
}
