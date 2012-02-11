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
    // Called at the beginning of each frame, before controllers are updated
    virtual void preFrame() = 0;
    virtual void update(float dt) = 0;
    virtual void render(float dt) = 0;
    // Called after render, but before the engine postrender
    virtual void postFrame() = 0;
};

