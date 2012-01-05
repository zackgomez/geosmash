#pragma once
#include "GameState.h"
#include <GL/glew.h>

class Fighter;

class StatsGameState
{
public:
    StatsGameState(const std::vector<Fighter *> fighters, int winningTeam);
    virtual ~StatsGameState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*> &joysticks,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);

private:
    std::vector<Fighter *> fighters_;
    int winningTeam_;

    GLuint backgroundTex_;
};

