#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>
#include "GameEntity.h"

class Fighter;
class FighterState;
class SpecialState;
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
class FighterRenderer;
typedef SpecialState* (*SpecialStateFunc)(const std::string&, Fighter*, bool);

class Fighter : public GameEntity
{
public:
    Fighter(const glm::vec3 &color, int playerID,
            int teamID, int startingLives, const std::string &username,
            const std::string &fighterName);
    ~Fighter();

    void setRespawnLocation(float x, float y);

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
    // Fighters are allowed to get the name of the current animation frame
    std::string getFrameName() const;
    const glm::vec3& getColor() const { return color_; }
    const std::string& getUsername() const { return username_; }
    // Returns the id of the player that last hit this fighter, or -1 if there
    // is none
    int getLastHitBy() const;
    // True if this player has more than 0 lives
    bool isAlive() const;

    virtual bool isDone() const { return false; }

    // collision is true if there is a collision with ground this frame, false otherwise
    void collisionWithGround(const rectangle &ground, bool collision, bool platform);
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
    // life will be removed
    void respawn(bool killed);


    // Functions for life sharing in teams
    void stealLife();
    void addLives(int lives);

    // Puts the fighter into a limp state, they have no control and can be
    // controlled by the caller.  Passed function object is called when the
    // fighter is pulled out of the limp state for whatever reason - the
    // LimpFighter is no longer valid after that.  Fighter is responsible
    // for delete callback object.
    LimpFighter* goLimp(UnlimpCallback *l);

	// a getParam() call that automatically prefixes the name of the character
	float param(const std::string &param) const;

private:
    FighterState *state_;
    std::string pre_;
    float dir_; // 1 or -1 look in xdir
    float damage_;
    float shieldHealth_;
    int lives_;
    // The last ground/platform that the fighter stepped on
    rectangle lastGround_;

    // Fighter ID members
    float respawnx_, respawny_;
    glm::vec3 color_; std::string username_;

    std::string lastFrameName_;
    FighterRenderer *renderer_;

    // Current attack members
    FighterAttack* attack_;

    // Game statistics members
    int lastHitBy_; // The id of the fighter that last hit us, or -1

    // This fighters attacks
    std::map<std::string, FighterAttack *> attackMap_;
    SpecialStateFunc specialStateFactory_;

    // Map that holds fighter specific air information, cleared on landing
    std::map<std::string, float> airData_;

    // ---- Helper functions ----
    void stateWrapper(FighterState *fs);
    void renderHelper(float dt, const glm::vec3& color, const glm::mat4 &postTrans = glm::mat4(1.f));
    // Fills in attacks based on the move set
    void fillAttacks(const std::string &moveset);
    // Loads an attack from the params using the attackName.param syntax
    template<class AttackClass>
    AttackClass* loadAttack(const std::string &attackName, const std::string &audioID,
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
    friend class SpecialState;
    friend class DashSpecialState;
    friend class CharlieNeutralSpecial;
    friend class StickmanUpSpecial;
    friend class StickmanNeutralSpecial;

    friend class MovingAttack;
    friend class DashAttack;
};

// Interface for rendering a fighter in various states
class FighterRenderer
{
public:
    virtual ~FighterRenderer() { }

    virtual void render(const glm::mat4 &transform, const glm::vec4 &color,
            const std::string &frameName, float dt) const = 0;
};

class BixelFighterRenderer : public FighterRenderer
{
public:
    BixelFighterRenderer() { }
    virtual ~BixelFighterRenderer() { }

    virtual void render(const glm::mat4 &transform, const glm::vec4 &color,
            const std::string &frameName, float dt) const;
};

class Skeleton;
class GeosmashBoneRenderer;
class SkeletonFighterRenderer : public FighterRenderer
{
public:
    SkeletonFighterRenderer();
    virtual ~SkeletonFighterRenderer();

    virtual void render(const glm::mat4 &transform, const glm::vec4 &color,
            const std::string &frameName, float dt) const;
private:
    Skeleton *skeleton_;
    GeosmashBoneRenderer *renderer_;
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
    virtual void operator() (LimpFighter *caller) = 0;
};

template<class T>
struct GenericUnlimpCallback : public UnlimpCallback
{
    GenericUnlimpCallback(T* target) :
        target_(target)
    { }

    virtual void operator() (LimpFighter *caller)
    {
        target_->disconnectCallback(caller);
    }

private:
    T* target_;
};

