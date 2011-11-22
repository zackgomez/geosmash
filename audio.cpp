#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"

AudioManager::AudioManager() 
{
    // Load in all small sound files, like attack noise
    // Sample code for loading and playing a sound file
    // played through the left speaker follows 
    /*std::string fname = "sfx/ko-01.wav";
    sf::SoundBuffer *sb = new sf::SoundBuffer();
    sb->LoadFromFile(fname);
    s2->SetPosition(-1, 0, 0);
    s1->SetAttenuation(100);*/
}
#define D1 25.0f
#define D2 300.0f
#define V1 25.0f
#define V2 100.0f

void AudioManager::playSound(const std::string &fname)
{
    // check if the file has already been loaded in
    if (buffers_.count(fname) != 1) {
        sf::SoundBuffer *sb = new sf::SoundBuffer();
        assert(sb);
        sb->LoadFromFile("sfx" + fname + ".aif");
        buffers_[fname] = sb;
    }
    sf::Sound *s = new sf::Sound();
    s->SetBuffer(*buffers_[fname]);
    s->Play();
    currentSounds_.push_back(s);

}
void AudioManager::playSound(const std::string &fname, 
        glm::vec2 pos,
        double damage)
{
    assert(damage >= 0);
    double panningFactor = -2;
    if (damage > 300) damage = 300;
    // check if the file has already been loaded in
    if (buffers_.count(fname) != 1) {
        sf::SoundBuffer *sb = new sf::SoundBuffer();
        assert(sb);
        sb->LoadFromFile("sfx" + fname + ".aif");
        buffers_[fname] = sb;
    }
    sf::Sound *s = new sf::Sound();
    s->SetBuffer(*buffers_[fname]);
    s->SetPosition(panningFactor, 0, 0);
    assert(panningFactor >= -1 && panningFactor <= 1);
    double vol;
    if (damage > 0 && damage < D1) {
        vol = V1;
    }
    else if (damage < D2) {
        vol = V1 + ((V2 - V1) / (D2 - D1)) * (damage - D1);
    }
    else if (damage > D2) {
        vol = V2;
    }
    assert(vol >= V1 && vol <= V2);
    s->SetVolume(vol);
    // play that shit
    s->Play();
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

