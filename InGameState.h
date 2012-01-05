#pragma once
#include "GameState.h"

class Controller;
class GameEntity;
class Fighter;

class InGameState : public GameState
{
public:
    InGameState(const std::vector<Controller *> &controllers,
            const std::vector<Fighter *> &fighters);
    virtual ~InGameState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*> &joysticks,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);

private:
    std::vector<Controller*> controllers_;
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;

    bool paused_;
    bool criticalMusic_;
    size_t startTime_;


    // Helper functions
    void integrate(float dt);
    void collisionDetection();
    void renderHUD();
    void renderArrow(const Fighter *fighter);
};
