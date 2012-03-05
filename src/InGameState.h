#pragma once
#include "GameState.h"
#include <cstdlib>
#include <string>
#include <fstream>
#include "Logger.h"

class GameEntity;
class Fighter;
class Player;
class GameListener;
class GameMode;

class InGameState : public GameState
{
public:
    InGameState(const std::vector<Player *> &players,
            const std::vector<Fighter *> &fighters, bool keepStats,
            const std::string &stage, GameMode *gameMode);
    virtual ~InGameState();

    virtual GameState* processInput(const std::vector<Controller*> &joysticks,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    virtual void preFrame();
    virtual void postFrame();

    const std::vector<const Fighter*> getFighters() const;

private:
    std::vector<Player*> players_;
    std::vector<Fighter*> fighters_;
    std::vector<GameEntity *> entities_;
    GameMode *gameMode_;

    bool paused_;

    int pausingPlayer_;
    bool keepStats_;

    std::vector<GameListener *> listeners_;
    std::fstream replayStream_;

    LoggerPtr logger_;

    // Helper functions
    void integrate(float dt);
    void collisionDetection(float dt);
    void renderHUD();
    void renderPause();
    void renderArrow(const Fighter *fighter);

    void togglePause(int controllerID);

    Fighter* getFighterByID(int playerID);
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

class GameMode
{
public:
    virtual ~GameMode() { }

    virtual void update(float dt, const std::vector<Fighter *> &fighters) = 0;

    virtual bool gameOver() const = 0;
    virtual int getWinningTeam() const = 0;
};

class StockGameMode : public GameMode
{
public:
    StockGameMode();
    virtual ~StockGameMode();

    virtual void update(float dt, const std::vector<Fighter *> &fighters);
    virtual bool gameOver() const;
    virtual int getWinningTeam() const;

private:
    bool gameOver_;
    bool criticalMusic_;
    int winningTeam_;
};

// Debug game mode, game never ends, no one wins
class DebugGameMode : public GameMode
{
public:
    DebugGameMode() { }
    virtual ~DebugGameMode() { }
    virtual void update(float dt, const std::vector<Fighter *> &fighters) { }
    virtual bool gameOver() const { return false; }
    virtual int getWinningTeam() const { return -1; }
};
