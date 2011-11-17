#pragma once

#include <SFML/Audio.hpp>

class AudioManager
{
public:
    static AudioManager* get();

    // Play an audio file (for example, on being hit with an attack)
    // (Calling object is responsible for building the identifier, 
    // not the full filename)
    void playSound(const std::string &audioIdentifier);

    // Loads the filename as the soundtrack - does not start playing it until
    // startSoundtrack() is called.
    void setSoundtrack(const std::string &filename);
    // Starts the soundtrack song
    void startSoundtrack();
    // Pauses the soundtrack song
    void pauseSoundtrack();

    // Called every frame, allows AudioManager to cleanup its state 
    // (free members, etc)
    void update(float dt);

private:
    AudioManager();

    // list of sounds currently playing. we need to periodically check 
    // this list to not leak memory.
    std::vector<sf::Music *> currentSounds_;

    // List of all buffers representing small sounds, like attack noises
    std::vector<sf::SoundBuffer *>soundBuffers_;

    // List of small sounds playing 
    // Each sound will point to a buffer in soundBuffers_.
    std::vector<sf::Sound *> playingSounds_;

    sf::Music soundtrack_;
};


