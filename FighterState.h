#pragma once
#include <string>

class Fighter;
class Rectangle;
class Controller;
class Attack;

class FighterState
{
public:
    FighterState(Fighter *f) :
        fighter_(f), next_(NULL), frameName_("GroundNeutral")
    {}
    virtual ~FighterState() {}

    // Returns true if this state should transition to a new one
    bool hasTransition() const { return next_ != NULL; }
    // Returns the next state to transition to, only valid if needsTransition()
    // returns true.
    FighterState* nextState() const { return next_; }

    // State behavior functions
    // This function is called once every call to Fighter::processInput
    virtual void processInput(Controller&, float dt) = 0;
    // TODO description
    virtual void render(float dt) = 0;
    // This function is called once every call to Fighter::collisionWithGround
    virtual void collisionWithGround(const Rectangle &ground, bool collision) = 0;
    // This function is called when Fighter::hitByAttack is called, before any
    // other work is done
    virtual void hitByAttack(const Attack *attack) = 0;

    virtual Rectangle getRect() const;

    // This function is the value returned from fighter::canBeHit
    virtual bool canBeHit() const;

protected:
    Fighter *fighter_;
    FighterState *next_;
    std::string frameName_;

    void calculateHitResult(const Attack *attack);
    void collisionHelper(const Rectangle &ground);
};





class GroundState : public FighterState
{
public:
    GroundState(Fighter *f, float delay = -1.0f);
    virtual ~GroundState();

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);

private:
    // Jump startup timer.  Value >= 0 implies that the fighter is starting a jump
    float jumpTime_;
    // Dash startup timer.  Value >= 0 implies that the fighter is start to dash
    float dashTime_;
    // Generic wait time, wait while value > 0
    float waitTime_;
    bool dashing_;
};

class AirNormalState : public FighterState
{
public:
    AirNormalState(Fighter *f);
    virtual ~AirNormalState();

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);

private:
    // True if the player has a second jump available
    bool canSecondJump_;
    // Jump startup timer.  Value > 0 implies that the fighter is starting a jump
    float jumpTime_;
    // True if this player has begun fastfalling
    bool fastFalling_;
};

class AirStunnedState : public FighterState
{
public:
    AirStunnedState(Fighter *f, float duration);
    virtual ~AirStunnedState() { }

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);

private:
    float stunDuration_;
    float stunTime_;
};

class DodgeState : public FighterState
{
public:
    DodgeState(Fighter *f);
    virtual ~DodgeState() {}

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;

private:
    float t_;
    float dodgeTime_;
    float invincTime_;
    float cooldown_;
};

class DuckingState : public FighterState
{
public:
    DuckingState(Fighter *f);
    virtual ~DuckingState() {}

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);
    virtual Rectangle getRect() const;

private:
    glm::vec2 size_;
};


class RespawnState : public FighterState
{
public:
    RespawnState(Fighter *f);
    virtual ~RespawnState() {}

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;

private:
    float t_;
};

class DeadState : public FighterState
{
public:
    DeadState(Fighter *f);
    virtual ~DeadState() {};

    virtual void processInput(Controller&, float dt);
    virtual void render(float dt) { };
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;
};
