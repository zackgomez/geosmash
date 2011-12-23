#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>
#include "GameEntity.h"

class FighterState;
class Attack;
class FighterAttack;
struct controller_state;

class rectangle
{
public:
    rectangle();
    rectangle(float x, float y, float w, float h);

    bool overlaps(const rectangle &rhs) const;
    bool contains(const rectangle &rhs) const;

    float x, y, w, h;
};

class LimpFighter;
struct UnlimpCallback;

class Fighter : public GameEntity
{
public:
    Fighter(float respawnx, float respawny, const glm::vec3 &color, int id);
    ~Fighter();

    static const std::string type;
    // required for subclassing GameEntity
    virtual std::string getType() const { return type; }

    // dt is time from last call to processInput
    void processInput(controller_state &, float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    virtual rectangle getRect() const;

    int getLives() const;
    float getDamage() const;
    float getDirection() const; // returns -1 or 1
    const glm::vec3& getColor() const { return color_; }
    // Returns the id of the player that last hit this fighter, or -1 if there
    // is none
    int getLastHitBy() const;
    // True if this player has more than 0 lives
    bool isAlive() const;

    virtual bool isDone() const { return false; }

    // collision is true if there is a collision with ground this frame, false otherwise
    void collisionWithGround(const rectangle &ground, bool collision);
    virtual void attackCollision(const Attack *other);
    virtual void hitByAttack(const Attack* attack);
    virtual void attackConnected(GameEntity *victim);

    // Returns true if this Fighter is currently attacking and has an attack hitbox
    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;

    // Returns true if this fighter is not invincible and can be hit by the
    // passed in attack
    virtual bool canBeHit() const;

    // Respawns the fighter at its respawn location.  If killed is true, a
    // life be removed
    void respawn(bool killed);

    // Puts the fighter into a limp state, they have no control and can be
    // controlled by the caller.  Passed function object is called when the
    // fighter is pulled out of the limp state for whatever reason - the
    // LimpFighter is no longer valid after that.  Fighter is responsible
    // for delete callback object.
    LimpFighter* goLimp(UnlimpCallback *l);

private:
    float dir_; // 1 or -1 look in xdir
    FighterState *state_;
    float damage_;
    float shieldHealth_;
    int lives_;

    // Fighter ID members
    float respawnx_, respawny_;
    glm::vec3 color_;

    // Current attack members
    FighterAttack* attack_;

    // Game statistics members
    int lastHitBy_; // The id of the fighter that last hit us, or -1

    // This fighters attacks
    std::map<std::string, FighterAttack *> attackMap_;

    // ---- Helper functions ----
    void stateWrapper(FighterState *fs);
    float damageFunc() const; // Returns a scaling factor based on damage
    void renderHelper(float dt, const std::string &frameName, const glm::vec3& color, const glm::mat4 &postTrans = glm::mat4(1.f));
    // Loads an attack from the params using the attackName.param syntax
    template<class AttackClass>
    AttackClass* loadAttack(std::string attackName, const std::string &audioID,
            const std::string &frameName);

    friend class FighterState;
    friend class GroundState;
    friend class DashState;
    friend class AirNormalState;
    friend class AirStunnedState;
    friend class DodgeState;
    friend class DeadState;
    friend class RespawnState;
    friend class BlockingState;
    friend class LedgeGrabState;
    friend class CounterState;
    friend class UpSpecialState;
    friend class GrabbingState;
    friend class LimpState;

    friend class MovingAttack;
    friend class DashAttack;
};

// Interface exposed when a fighter goes limp.  Allows direct control of the
// fighter.
class LimpFighter
{
public:
    virtual ~LimpFighter() { }

    // Set the position, velocity and acceleration of the fighter
    virtual void setPosition(const glm::vec2 &pos) = 0;
    virtual void setVelocity(const glm::vec2 &vel) = 0;
    virtual void setAccel(const glm::vec2 &accel) = 0;

    virtual void setDirection(float dir) = 0;
    // Sets whether or not the limp fighter can be hit by other attacks
    // DEFAULTS TO FALSE (cannot be hit)
    virtual void setHitable(bool canHit) = 0;

    // Sets the fighters display frame
    virtual void setFrameName(const std::string &frameName) = 0;
    virtual void setPreTransform(const glm::mat4 &pretrans) = 0;

    // Hits the fighter with the attack-- will trigger disconnect event
    virtual void hit(const Attack *attack) = 0;

    // Releases the fighter from the hold.  Triggers callback, interface
    // is no longer valid after call.  Puts the fighter in the AirNormal state.
    virtual void release() = 0;

    // Returns the corresponding GameEntity object for this limp fighter
    virtual const GameEntity *getEntity() const = 0;
};

struct UnlimpCallback
{
public:
    virtual ~UnlimpCallback() { }
    virtual void operator() () = 0;
};

template<class T>
struct GenericUnlimpCallback : public UnlimpCallback
{
    GenericUnlimpCallback(T* target) :
        target_(target)
    { }

    virtual void operator() ()
    {
        target_->disconnectCallback();
    }

private:
    T* target_;
};

