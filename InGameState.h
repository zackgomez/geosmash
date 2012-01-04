#pragma once
#include "GameState.h"

class Controller;
class GameEntity;
class Fighter;

class InGameState
{
public:
    InGameState(std::vector<Controller *> &controllers, bool teams);
    virtual ~InGameState();

    virtual GameState* processInput(std::vector<SDL_Joystick*> &joysticks);
    virtual void update();
    virtual void render();

private:
    bool teams_;
    bool makeHazard_;

    bool muteMusic_;
    bool criticalMusic_;

    int pausedPlayer_;

    size_t startTime_;

    std::vector<Controller*> controllers_;
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;

    // Helper functions
    void integrate(float dt);
    void collisionDetection();
    void renderHUD();
    void renderArrow(const Fighter *fighter);
};
