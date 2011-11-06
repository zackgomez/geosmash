#pragma once

#include <SFML/Audio.hpp>

void start_song(const char *filename);
void play_song();
void stop_song();


class AudioManager
{
public:
    static AudioManager* get();

    // Play an audio file (for example, on being hit with an attack)
    // (Calling object is responsible for building the identifier, 
    // not the full filename)
    void playSound(const std::string &audioFilename);

    // Called every frame, allows AudioManager to cleanup its state 
    // (free members, etc)
    void update(float dt);

private:
    AudioManager();

    // list of sounds currently playing. we need to periodically check 
    // this list to not leak memory.
    std::vector<sf::Music *> currentSounds_;



};


