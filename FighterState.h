#pragma once
#include <string>
#include "Fighter.h"

class Fighter;
class rectangle;
class controller_state;
class Attack;
class Ledge;

class FighterState
{
public:
    FighterState(Fighter *f) :
        fighter_(f), frameName_("GroundNeutral"), invincTime_(0.f)
    {}
    virtual ~FighterState() {}

    // State behavior functions
    // This function is called once every call to Fighter::processInput
    virtual FighterState* processInput(controller_state&, float dt) = 0;
    // Called once per call to Fighter::update AFTER integration
    virtual void update(float dt);
    // TODO description
    virtual void render(float dt) = 0;
    // This function is called once every call to Fighter::collisionWithGround
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision) = 0;
    // This function is called when Fighter::hitByAttack is called, before any
    // other work is done
    virtual FighterState* hitByAttack(const Attack *attack) = 0;

    // Called when Fighter::attackConnected is called
    virtual FighterState* attackConnected(GameEntity *victim);

    virtual rectangle getRect() const;

    // This function is the value returned from fighter::canBeHit
    virtual bool canBeHit() const;

protected:
    Fighter *fighter_;
    std::string frameName_;
    float invincTime_;

    FighterState* calculateHitResult(const Attack *attack);
    void collisionHelper(const rectangle &ground);
    FighterState* checkForLedgeGrab();
    // Helper for dealing with all B moves.
    FighterState* performBMove(const controller_state &, bool ground = true);
    template<typename T> 
    static T muxByTime(const T& color, float t);

    void setInvincTime(float t);
};

class GroundState : public FighterState
{
public:
    GroundState(Fighter *f, float delay = -1.0f, float invincTime = 0.f);
    virtual ~GroundState();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual rectangle getRect() const;

private:
    // Jump startup timer.  Value >= 0 implies that the fighter is starting a jump
    float jumpTime_;
    // Dash startup timer.  Value >= 0 implies that the fighter is start to dash
    float dashTime_;
    // Generic wait time, wait while value > 0
    float waitTime_;
    bool dashing_;
    bool ducking_;
};

class BlockingState : public FighterState
{
public:
    BlockingState(Fighter *f);
    virtual ~BlockingState();

    virtual FighterState* processInput(controller_state &, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    float waitTime_;
    float dazeTime_;
    float hitStunTime_;
    // time left in step dodge
    float stepTime_;
};

class AirNormalState : public FighterState
{
public:
    AirNormalState(Fighter *f);
    virtual ~AirNormalState();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);

    AirNormalState * setNoGrabTime(float t);

private:
    // True if the player has a second jump available
    bool canSecondJump_;
    // Jump startup timer.  Value > 0 implies that the fighter is starting a jump
    float jumpTime_;
    // True if this player has begun fastfalling
    bool fastFalling_;

    // when >0 cannot ledge grab
    float noGrabTime_;
};

class AirStunnedState : public FighterState
{
public:
    AirStunnedState(Fighter *f, float duration, bool groundBounce = false);
    virtual ~AirStunnedState() { }

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    float stunDuration_;
    float stunTime_;
    bool gb_;
};

class CounterState : public FighterState
{
public:
    CounterState(Fighter *f, bool ground);
    virtual ~CounterState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    // How long have we been in this state?
    float t_;
    bool ground_;
    std::string pre_;
    bool playedSound_;
};

class UpSpecialState : public FighterState
{
public:
    UpSpecialState(Fighter *f);
    virtual ~UpSpecialState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);

private:
    std::string pre_;
};

// Responsible for grabbing, holding and throwing
class GrabbingState : public FighterState
{
public:
    GrabbingState(Fighter *f);
    virtual ~GrabbingState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);
    // Override to control invincibility frames during throws
    virtual bool canBeHit() const;

    // Called by LimpFighter when disconnect is necessary, or when we release
    void disconnectCallback();

private:
    std::string pre_;
    LimpFighter *victim_;
    std::string frameName_;
    float holdTimeLeft_;
};

class DodgeState : public FighterState
{
public:
    DodgeState(Fighter *f);
    virtual ~DodgeState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    float t_;
    float dodgeTime_;
    float cooldown_;
};

class LedgeGrabState : public FighterState
{
public:
    LedgeGrabState(Fighter *f);
    virtual ~LedgeGrabState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;
    virtual rectangle getRect() const;

    void grabLedge(Ledge *ledge);

private:
    glm::vec2 hbsize_;
    float jumpTime_;
    Ledge *ledge_;
};

class LimpState : public FighterState, public LimpFighter
{
public:
    LimpState(Fighter *f, UnlimpCallback *callback);
    virtual ~LimpState();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;

    // Inherited from LimpFighter interface
    virtual void setPosition(const glm::vec2 &pos);
    virtual void setVelocity(const glm::vec2 &vel);
    virtual void setAccel(const glm::vec2 &accel);
    virtual void setDirection(float dir);
    virtual void setHitable(bool hitable);
    virtual void setFrameName(const std::string &frameName);
    virtual void hit(const Attack *attack);
    virtual void release();
    virtual const GameEntity *getEntity() const;


private:
    UnlimpCallback *unlimpCallback_;
    std::string frameName_;
    FighterState *next_;
    bool hitable_;
};

class RespawnState : public FighterState
{
public:
    RespawnState(Fighter *f);
    virtual ~RespawnState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;

private:
    float t_;
};

class DeadState : public FighterState
{
public:
    DeadState(Fighter *f);
    virtual ~DeadState() {};

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt) { };
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual bool canBeHit() const;
};

