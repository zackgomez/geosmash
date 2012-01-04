#pragma once
#include "GameState.h"

class Controller;
class GameEntity;
class Fighter;

class InGameState
{
public:
    InGameState(const std::vector<Fighter *> &fighters);
    virtual ~InGameState();

    virtual GameState* processInput(std::vector<SDL_Joystick*> &joysticks);
    virtual void update();
    virtual void render();

private:
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;

    bool criticalMusic_;
    size_t startTime_;


    // Helper functions
    void integrate(float dt);
    void collisionDetection();
    void renderHUD();
    void renderArrow(const Fighter *fighter);
};
