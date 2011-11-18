#pragma once

#include <SFML/Audio.hpp>
#include <map>

class AudioManager
{
public:
    static AudioManager* get();

    // Play an audio file (for example, on being hit with an attack)
    // (Calling object is responsible for building the identifier, 
    // not the full filename)
    // 
    // Intended use: playing small sounds like KO or attack noises.
    void playSound(const std::string &audioIdentifier);

    // Play a longer music file. This function streams from disk.
    void playMusic(const std::string &audioIdentifier);

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

    // List of all buffers representing small sounds, like attack noises
    std::map<std::string, sf::SoundBuffer *>buffers_;

    // List of small sounds playing 
    // Each sound will point to a buffer in soundBuffers_.
    std::vector<sf::Sound *> currentSounds_;

    sf::Music soundtrack_;
};


