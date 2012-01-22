#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Controller.h"

class Controller;
class Fighter;

class Player
{
public:
    virtual ~Player();

    explicit Player(const Fighter *);

    // Gets the next state
    virtual controller_state getState() const = 0;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const = 0;
    virtual bool wantsStatsContinue() const = 0;

    // Does per frame player updates
    virtual void update(float dt) = 0;

    virtual int getPlayerID() const;
    virtual int getTeamID() const;
    virtual std::string getUsername() const;
    virtual glm::vec3 getColor() const;
protected:
    const Fighter *fighter_;

};

class AIPlayer : public Player 
{
public:
    AIPlayer(const Fighter *f);

    virtual controller_state getState() const;
    virtual bool wantsPauseToggle() const { return false; }
    virtual bool wantsStatsContinue() const { return true; }
    
    virtual void update(float);
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
    LocalPlayer(Controller *controller, const Fighter *f);
    virtual ~LocalPlayer();

    // Does per frame player updates
    virtual void update(float dt);

    // Gets the next state
    virtual controller_state getState() const;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const;
    virtual bool wantsStatsContinue() const { return false; }

private:
    Controller *controller_;
};

