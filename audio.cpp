#include "audio.h"
#include <iostream>

static sf::Music music;

void play_song(const char *filename)
{
    if (!music.OpenFromFile(filename))
    {
        std::cout << "Unable to open music file\n";
        return;
    }
    music.SetLoop(true);
    music.Play();
}
