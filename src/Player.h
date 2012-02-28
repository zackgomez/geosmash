#pragma once
#include <string>
#include <glm/glm.hpp>
#include <set>
#include <map>
#include "Controller.h"
#include "InGameState.h"
#include "Logger.h"

class Controller;
class Fighter;

class Player : public GameListener
{
public:
    virtual ~Player();

    explicit Player(const Fighter *);

    // Gets the next state
    virtual controller_state getState() const = 0;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const = 0;
    // Returns true if this player would like to steal a life from the teammate
    // with the most lives remaining.
    virtual bool wantsLifeSteal() const = 0;
    // This function bypasses the controller in StatsGameState
    // It's mainly used for non local players, as they have a controller
    // Eventually this function could be removed by the players knowing what 
    // the game state current is, so they returned controller_state shows
    // that the (non local) player wants to continue.
    virtual bool wantsStatsContinue() const = 0;

    // Does per frame player updates
    virtual void update(float dt) = 0;

    // Inherited from GameListener
    virtual void updateListener(const std::vector<Fighter *> &fighters) = 0;
    virtual bool removeOnCompletion() const { return false; }

    // Accessors for fighter variables
    virtual int getPlayerID() const;
    virtual int getTeamID() const;
    virtual std::string getUsername() const;
    virtual glm::vec3 getColor() const;
    virtual std::string getFighterName() const;

protected:
    const Fighter *fighter_;
};

class AIPlayer : public Player 
{
public:
    explicit AIPlayer(const Fighter *f);

    virtual controller_state getState() const;
    virtual bool wantsPauseToggle() const { return false; }
    virtual bool wantsStatsContinue() const { return true; }
    virtual bool wantsLifeSteal() const { return false; }
    
    virtual void update(float);
    virtual void updateListener(const std::vector<Fighter *> &fighters);

protected:
    void performGetBack();
    glm::vec2 pos;
    bool danger;
    void senseDanger();
    void setTargetPos();
    void performAttack();
    glm::vec2 targetPos;
private:
    controller_state cs_;

};

class LocalPlayer : public Player
{
public:
    explicit LocalPlayer(Controller *controller, const Fighter *f);
    virtual ~LocalPlayer();

    // Does per frame player updates
    virtual void update(float dt);

    // Gets the next state
    virtual controller_state getState() const;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const;
    virtual bool wantsStatsContinue() const { return false; }
    virtual bool wantsLifeSteal() const;

    virtual void updateListener(const std::vector<Fighter *> &fighters);

private:
    Controller *controller_;
};

struct CaseGameState
{
    glm::vec2 abspos, relpos, relvel;
    std::string myframe;
    bool facing;

    float enemydamage;
    bool enemyhitbox;
    bool enemyvulnerable;

    bool operator<(const CaseGameState &rhs) const;
};

struct CasePlayerState
{
    int playerid;
    glm::vec2 pos, vel;
    std::string fname;
    float damage;
    bool hasAttack;
    float dir;
};

struct CaseAction
{
    std::string target;
    float dir;
};

std::ostream& operator<<(std::ostream &os, const CaseGameState &cgs);

class GhostAIPlayer : public Player
{
public:
    explicit GhostAIPlayer(const Fighter *f);
    virtual ~GhostAIPlayer();

    virtual controller_state getState() const;
    virtual bool wantsPauseToggle() const { return false; }
    virtual bool wantsStatsContinue() const { return true; }
    virtual bool wantsLifeSteal() const { return false; }
    
    virtual void update(float);
    virtual void updateListener(const std::vector<Fighter *> &fighters);

private:
    // Helper functions
    void readCaseBase(std::istream &is);
    static CasePlayerState readCPS(const std::string &line);
    void fillActionMap();

    static CasePlayerState fighter2cps(const Fighter *f);
    static CaseGameState cps2cgs(const CasePlayerState &me, const CasePlayerState &enemy);

    // Returns an empty string if the player should do nothing/continue doing
    // what they're doing
    CaseAction getNextAction() const;
    static float caseHeuristic(const CaseGameState &cur, const CaseGameState &ref);
    static bool isSynonymState(const std::string &a, const std::string &b);

    // Data Members
    controller_state cs_;

    std::set<std::string> actionFrames_;
    std::map<CaseGameState, CaseAction> caseBase_;
    std::map<std::string, controller_state> actionMap_;
    CaseGameState cgs_;

    LoggerPtr logger_;
};
