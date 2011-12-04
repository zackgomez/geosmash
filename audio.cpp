#include "audio.h"
#include <iostream>
#include <cassert>
#include "ParamReader.h"
#include "glutils.h"

AudioManager::AudioManager() 
{
    // TODO: possibly preload small sound files.
}

AudioManager::~AudioManager()
{
    for (unsigned i = 0; i < currentSounds_.size(); i++)
        delete currentSounds_[i];
    currentSounds_.clear();

    for (std::map<std::string, sf::SoundBuffer *>::iterator it = buffers_.begin();
            it != buffers_.end(); it++)
        delete it->second;
    buffers_.clear();
}

void AudioManager::playSound(const std::string &fname)
{
    // Now that the data is loaded into a buffer, add the data to a Sound obj
    // object that has a location in 3D space, volume, etc.
    sf::Sound *s = new sf::Sound();
    s->SetBuffer(*getBuffer(fname));
    float volume = getParam("sfx.volume");
    std::cout << "Looking for param: " << "sfx." + fname + ".volume\n";
    if (ParamReader::get()->hasParam("sfx." + fname + ".volume"))
        volume *= getParam("sfx." + fname + ".volume");
    s->SetVolume(volume);
    s->Play();
    currentSounds_.push_back(s);

}
void AudioManager::playSound(const std::string &fname, 
        glm::vec2 pos,
        double damage)
{
    double vol;
    double D1 = getParam("sfx.damagefloor");
    double D2 = getParam("sfx.damageceiling");
    double V1 = getParam("sfx.minvolume");
    double V2 = getParam("sfx.maxvolume");
    double panningFactor = getPanningFactor(pos);
    sf::Sound *s = new sf::Sound();
    s->SetBuffer(*getBuffer(fname));
    s->SetPosition(panningFactor, 0, 0);

    if (damage == -1) {
        // No damage was specified. Play the sound at max volume
        vol = 100;
    }
    if (damage > D2) damage = D2;
    // The damage --> volume function is stepwise
    if (damage > 0 && damage < D1) {
        // At low damage, we clamp volume to "minvolume"
        vol = V1;
    }
    else if (damage > 0 && damage < D2) {
        // In the middle range, we increase linearly between V1 and V2
        // According to how far along [D1, D2] the receiver is.
        vol = V1 + ((V2 - V1) / (D2 - D1)) * (damage - D1);
    }
    else if (damage > D2) {
        // Clamp volume on the high end to sfx.maxvolume
        vol = V2;
    }
    // TODO: MAKE THIS ASSERTION VALID! (stage hazard hits)
    //assert(vol >= V1 && vol <= V2);
    float volume = getParam("sfx.volume");
    std::cout << "Looking for param: " << "sfx." + fname + ".volume\n";
    if (ParamReader::get()->hasParam("sfx." + fname + ".volume"))
        volume *= getParam("sfx." + fname + ".volume");
    s->SetVolume(volume * vol/100.f);
    s->Play();
    currentSounds_.push_back(s);
}

sf::SoundBuffer* AudioManager::getBuffer(const std::string &fname)
{
    sf::SoundBuffer* result = NULL;
    // check if the file has already been loaded in
    if (buffers_.count(fname) != 1)
    {
        sf::SoundBuffer *sb = new sf::SoundBuffer();
        assert(sb);
        sb->LoadFromFile("sfx/" + fname + ".aif");
        buffers_[fname] = sb;
    }
    result = buffers_[fname];
    // We should really panic if we haven't retrieved this value by now.
    assert(result);
    return result;
}

float AudioManager::getPanningFactor(const glm::vec2 &worldPos)
{
    glm::vec4 screenPos = getProjectionMatrix() * getViewMatrix() * glm::vec4(worldPos, 0.f, 1.f);
    screenPos /= screenPos.w;

    // if the player is hit off the screen, we need to clip this val to be in
    // the valid range (-1,1)
    if (screenPos.x < -1)
        return -1;
    if (screenPos.x > 1)
        return 1;
    return screenPos.x;
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

