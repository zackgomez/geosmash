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

	std::string fname = "sfx/" + soundIdentifier + ".ogg";
	
	// Grab volume (optionally scaled) from params
    float volume = getParam("sfx.volume");
    logger_->debug() << "Looking for param: " << "sfx." + soundIdentifier + ".volume\n";
    if (ParamReader::get()->hasFloat("sfx." + soundIdentifier + ".volume")) 
	{
        volume *= getParam("sfx." + soundIdentifier + ".volume");
	}
	volume /= 100.0f; // Scale it to be between 0 and 1
	irrklang::ISound *sound = engine->play2D(fname.c_str(), false, true, true);
	if (!sound)
	{
		return;
	}
	sound->setVolume(volume);
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
    logger_->debug() << "Looking for param: " << "sfx." + audioID + ".volume\n";
    if (ParamReader::get()->hasFloat("sfx." + audioID + ".volume")) 
	{
        volume *= getParam("sfx." + audioID + ".volume");
	}
	sound->setVolume((volume * vol/100.f) / 100);
	sound->setIsPaused(false);
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
	assert(soundtrack_ && "Couldn't find the soundtrack. Did you add the SFX directory?");
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
    // Much ado about nothing
	// TODO: need to drop finished sounds
}

void AudioManager::mute()
{
    muted_ = true;
}

void AudioManager::unmute()
{
    muted_ = false;
}
