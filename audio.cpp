#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"

AudioManager::AudioManager() 
{
    // Load in all small sound files, like attack noise
    std::string fname = "sfx/ko.aif";
    sf::SoundBuffer sb;
    sb.LoadFromFile(fname);
    sf::Sound s1;
    sf::Sound s2;
    s1.SetBuffer(sb);
    s2.SetBuffer(sb);
    s1.SetPosition(1, 0, 0);
    s2.SetPosition(-1, 0, 0);
    s1.Play();
    while (s1.GetStatus() == sf::Sound::Playing)
    s2.Play();
}

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

