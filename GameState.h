#pragma once
#include <vector>

class Controller;

class GameState
{
public:
    GameState() {}
    virtual ~GameState() {}

    virtual GameState* processInput(const std::vector<Controller*> &controllers,
            float dt) = 0;
    virtual void update(float dt) = 0;
    virtual void render(float dt) = 0;
};

