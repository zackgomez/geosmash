#pragma once
#include <vector>
#include <SDL/SDL.h>

class GameState
{
public:
    GameState() {}
    virtual ~GameState() {}

    virtual GameState* processInput(const std::vector<SDL_Joystick*> &joysticks) = 0;
    virtual void update(float dt) = 0;
    virtual void render(float dt) = 0;

};

