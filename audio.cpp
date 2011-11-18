#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"

AudioManager::AudioManager() 
{
    // Load in all small sound files, like attack noise
    std::string fname = "sfx/ko.aif";
    sf::SoundBuffer *sb = new sf::SoundBuffer();
    sb->LoadFromFile(fname);
    sf::Sound *s1, *s2;
    s2 = new sf::Sound();
    s1 = new sf::Sound();
    s1->SetBuffer(*sb);
    s2->SetBuffer(*sb);
    s1->SetRelativeToListener(1);
    s1->SetPosition(10000, 10000, 1e8);
    s2->SetPosition(1, 0, 0);
    s1->Play();
    while (s1->GetStatus() == sf::Sound::Playing) {}
    std::cout << "Playing sound 2 (ko?)" << std::endl;
    s2->Play();
    s1->SetAttenuation(100);
    std::cout << "min distance, attenuation: " << 
    s1->GetMinDistance() << ", " <<
        s1->GetAttenuation() << std::endl;
}


void AudioManager::playSound(const std::string &fname)
{
    // check if the file has already been loaded in
    if (buffers_.count(fname) != 1) {
        sf::SoundBuffer *sb = new sf::SoundBuffer();
        assert(sb);
        sb->LoadFromFile("sfx" + fname + ".aif");
        buffers_[fname] = sb;
    }
    // play that shit
    sf::Sound *s = new sf::Sound();
    
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
    std::vector<sf::Sound *>::iterator it;
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

