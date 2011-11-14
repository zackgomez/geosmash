#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"

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

void AudioManager::setSoundtrack(const std::string &file)
{
    soundtrack_.Stop();
    soundtrack_.OpenFromFile(file);
    soundtrack_.SetLoop(true);
    soundtrack_.SetVolume(getParam("soundtrack.volume"));
}

void AudioManager::startSoundtrack()
{
    soundtrack_.Play();
}

void AudioManager::pauseSoundtrack()
{
    soundtrack_.Pause();
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
        {
            delete *it;
            it = currentSounds_.erase(it);
        }
        else
            it++;
    }
}

