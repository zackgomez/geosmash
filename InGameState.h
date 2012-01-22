#pragma once
#include "GameState.h"
#include <cstdlib>

class GameEntity;
class Fighter;
class Player;

class InGameState : public GameState
{
public:
    InGameState(const std::vector<Player *> &players,
            const std::vector<Fighter *> &fighters, bool makeHazard);
    virtual ~InGameState();

    virtual GameState* processInput(const std::vector<Controller*> &joysticks,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);

    bool stealLife(int teamID);

    // XXX: Get rid of this hack
    static InGameState *instance;

    const std::vector<const Fighter*> getFighters() const;

private:
    std::vector<Player*> players_;
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;

    bool paused_;
    bool criticalMusic_;
    size_t startTime_;

    int pausingPlayer_;


    // Helper functions
    void integrate(float dt);
    void collisionDetection();
    void renderHUD();
    void renderPause();
    void renderArrow(const Fighter *fighter);

    void togglePause(int controllerID);
};
