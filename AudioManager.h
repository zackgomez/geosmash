#pragma once

#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Logger.h"
#include "irrKlang.h"

//
// SFML Notes
// SFML uses OpenAL, which will NOT use spatialization features
// on audio files that contain more than one channel.
//
// YOU MUST USE MONO AUDIO FILES TO USE PANNING! 
//

class AudioManager
{
public:
    static AudioManager* get();

    // Play an audio file (for example, on being hit with an attack)
    // (Calling object is responsible for building the identifier, 
    // not the full filename)
    //
    // @param loc: Where is the fighter in world coords
    //                       We will figure out the rest 
    //
    // @param damage: at 300 damage, sound will be saturated, or full volume.
    // 
    // Intended use: playing small sounds like attack noises.
    void playSound(const std::string &audioIdentifier,
            glm::vec2 loc, 
            double damage = -1); // between 0, ~300 usually

    // Useful for playing sounds that don't emanate from a single location
    // (like KO noise)
    void playSound(const std::string &audioID);

    // Loads the filename as the soundtrack - does not start playing it until
    // startSoundtrack() is called.
    void setSoundtrack(const std::string &filename);
    // Starts the soundtrack song
    void startSoundtrack();
    // Pauses the soundtrack song
    void pauseSoundtrack();

    // Mutes all sounds
    void mute();
    void unmute();

    // Called every frame, allows AudioManager to cleanup its state 
    // (free members, etc)
    void update(float dt);
	
private:
    static AudioManager *am;
	AudioManager();
    ~AudioManager();
    LoggerPtr logger_;
	irrklang::ISoundEngine *engine;

    bool muted_;

    // List of small sounds playing 
    // Each sound will point to a buffer in soundBuffers_.
    std::vector<irrklang::ISound *> currentSounds_;

    irrklang::ISound *soundtrack_;

    // Helper functions
    float getPanningFactor(const glm::vec2 &worldPos);
};

