#pragma once
#include "GameState.h"
#include <cstdlib>
#include <string>

class GameEntity;
class Fighter;
class Player;
class GameListener;

class InGameState : public GameState
{
public:
    InGameState(const std::vector<Player *> &players,
            const std::vector<Fighter *> &fighters, bool keepStats,
            const std::string &stage);
    virtual ~InGameState();

    virtual GameState* processInput(const std::vector<Controller*> &joysticks,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);

    const std::vector<const Fighter*> getFighters() const;

private:
    std::vector<Player*> players_;
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;

    bool paused_;
    bool criticalMusic_;
    size_t startTime_;

    int pausingPlayer_;
    bool keepStats_;

    std::vector<GameListener *> listeners_;

    // Helper functions
    void integrate(float dt);
    void collisionDetection(float dt);
    void renderHUD();
    void renderPause();
    void renderArrow(const Fighter *fighter);

    void togglePause(int controllerID);

    bool stealLife(int teamID);
};

class GameListener
{
public:
    virtual ~GameListener() { }

    // Memory managment function
    // This should return true if the listener should be deleted at the end of
    // the match
    virtual bool removeOnCompletion() const = 0;

    // Called at the start of each frame
    virtual void updateListener(const std::vector<Fighter *> &fighters) = 0;
};

