#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>
#include "FighterState.h"
#include "GameEntity.h"

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

class Fighter : public GameEntity
{
public:
    Fighter(float respawnx, float respawny, const glm::vec3 &color, int id);
    ~Fighter();

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

    friend class MovingAttack;
    friend class DashAttack;
};
