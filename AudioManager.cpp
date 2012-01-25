#include "AudioManager.h"
#include <cassert>
#include "ParamReader.h"
#include "Engine.h"

AudioManager *AudioManager::am = NULL;

AudioManager::AudioManager() : muted_(false), soundtrack_(NULL)
{
    // TODO: possibly preload small sound files.
    logger_ = Logger::getLogger("AudioManager");
	engine = irrklang::createIrrKlangDevice();
	assert(engine);
}

AudioManager::~AudioManager()
{
	engine->drop();
}

void AudioManager::playSound(const std::string &soundIdentifier)
{
    if (muted_) 
	{
        return;
	}

	std::string fname = "sfx/" + soundIdentifier + ".aif";
	
	// Grab volume (optionally scaled) from params
    float volume = getParam("sfx.volume");
    logger_->debug() << "Looking for param: " << "sfx." + fname + ".volume\n";
    if (ParamReader::get()->hasParam("sfx." + fname + ".volume")) 
	{
        volume *= getParam("sfx." + fname + ".volume");
	}
	volume /= 100.0f; // Scale it to be between 0 and 1
	irrklang::ISound *sound = engine->play2D(fname.c_str(), false, true, true);
	if (!sound)
	{
		return;
	}
	sound->setVolume(volume / 100.0f);
	sound->setIsPaused(false);
    
}

void AudioManager::playSound(const std::string &audioID, 
        glm::vec2 pos,
        double damage)
{
	if (audioID.empty()) 
	{
		return;
	}
    if (muted_)
	{
        return;
	}
    double vol;
    double D1 = getParam("sfx.damagefloor");
    double D2 = getParam("sfx.damageceiling");
    double V1 = getParam("sfx.minvolume");
    double V2 = getParam("sfx.maxvolume");
    double panningFactor = getPanningFactor(pos);
	std::string fname = "sfx/" + audioID + ".ogg";
	irrklang::ISound *sound = engine->play2D(fname.c_str(), false, true, true);
	if (!sound)
	{
		// Don't crash if this happens. But freak the fuck out!
		return;
	}
	sound->setPan(panningFactor);
    
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
    logger_->debug() << "Looking for param: " << "sfx." + fname + ".volume\n";
    if (ParamReader::get()->hasParam("sfx." + fname + ".volume")) 
	{
        volume *= getParam("sfx." + fname + ".volume");
	}
	sound->setVolume((volume * vol/100.f) / 100);
	sound->setIsPaused(false);
}

sf::SoundBuffer* AudioManager::getBuffer(const std::string &fname)
{
	return NULL;
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
    glm::vec4 screenPos = getProjectionMatrixStack().current() *
        getViewMatrixStack().current() * glm::vec4(worldPos, 0.f, 1.f);
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
	if (soundtrack_) 
	{
		soundtrack_->stop();
	}
	soundtrack_ = engine->play2D(file.c_str(), true, true, true);
}

void AudioManager::startSoundtrack()
{
    if (muted_) 
	{
        return;
	}
	if (soundtrack_) 
	{
		soundtrack_->setIsPaused(false);
	}
}

void AudioManager::pauseSoundtrack()
{
	soundtrack_->setIsPaused();
}

AudioManager* AudioManager::get() 
{
	if (am == NULL) 
	{
		am = new AudioManager;
	}
    return am;
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

void AudioManager::mute()
{
    muted_ = true;
}

void AudioManager::unmute()
{
    muted_ = false;
}
