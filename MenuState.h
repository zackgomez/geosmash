#pragma once
#include "GameState.h"


struct NumPlayersEntry
{
    int val;
    std::string name;
};

class MenuState : public GameState
{
public:
    MenuState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*>&);
    virtual void update(float);
    virtual void render(float);
    
private:
    std::vector<NumPlayersEntry> playersMenuItems_;
};

