#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"

static sf::Music music;

void start_song(const char *filename)
{
    if (!music.OpenFromFile(filename))
    {
        std::cout << "Unable to open music file\n";
        return;
    }
    music.SetLoop(true);
    music.SetVolume(getParam("soundtrack.volume"));
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



AudioManager::AudioManager() {}

void AudioManager::playSound(const std::string &fname)
{
    std::string fullPath = "sfx/" + fname + ".aif";
    sf::Music *music = new sf::Music();
    assert(music);
    assert(music->OpenFromFile(fullPath));
    music->Play();

    currentSounds_.push_back(music);

}

AudioManager* AudioManager::get() 
{
    static AudioManager am;
    return &am;
}

void AudioManager::update(float dt) 
{
    std::vector<sf::Music *>::iterator it;
    for (it = currentSounds_.begin(); it != currentSounds_.end(); )
    {
        bool notplaying = (*it)->GetStatus() == sf::Sound::Stopped;
        if (notplaying)
            it = currentSounds_.erase(it);
        else
            it++;
    }
}

