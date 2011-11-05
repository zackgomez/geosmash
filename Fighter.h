#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <SFML/Audio.hpp>

class ParamReader;
class Fighter;

struct Controller
{
    // The positions [-1, 1] of the main analog stick
    float joyx, joyy;
    // The velocities of the main analog stick over some time period
    float joyxv, joyyv;
    // nonzero if the button is pressed
    int buttona, buttonb, buttonc, jumpbutton;
    // nonzero if the button was pressed this frame
    int pressa, pressb, pressc, pressjump;
};

class Rectangle
{
public:
    Rectangle();
    Rectangle(float x, float y, float w, float h);

    bool overlaps(const Rectangle &rhs) const;

    float x, y, w, h;
};

class Attack
{
public:
    Attack() {}
    Attack(float startup, float duration, float cooldown, float damage, float stun,
            const glm::vec2& knockback, const Rectangle &hitbox) :
        hitbox_(hitbox),
        startup_(startup), duration_(duration), cooldown_(cooldown),
        damage_(damage), stun_(stun), knockback_(knockback),
        hasHit_(false), t_(0.0f), owner_(NULL), sound_(NULL)
    {}

    virtual ~Attack() {}

    virtual Rectangle getHitbox() const;
    virtual float getDamage(const Fighter *fighter) const { return damage_; }
    virtual float getStun(const Fighter *fighter) const { return stun_; }
    virtual glm::vec2 getKnockback(const Fighter *fighter) const { return knockback_; }

    void setFighter(const Fighter *fighter);

    // If hitbox can hit another player
    virtual bool hasHitbox() const;
    // If hitbox should be drawn
    virtual bool drawHitbox() const;
    // If this attack is over
    virtual bool isDone() const;

    // Updates internal timer
    virtual void update(float dt);
    // Sends to cooldown time
    virtual void cancel();
    // Called when the attack 'connects'
    virtual void hit();
    void playSound();
    void setSound(sf::Music *);

private:
    Rectangle hitbox_;
    float startup_, duration_, cooldown_;
    float damage_, stun_;
    glm::vec2 knockback_;
    bool hasHit_;
    float t_;

    const Fighter *owner_;
    sf::Music *sound_;
};

class FighterState
{
public:
    FighterState(Fighter *f) :
        fighter_(f), next_(NULL)
    {}
    virtual ~FighterState() {}

    // Returns true if this state should transition to a new one
    bool hasTransition() const { return next_ != NULL; }
    // Returns the next state to transition to, only valid if needsTransition()
    // returns true.
    FighterState* nextState() const { return next_; }

    // State behavior functions
    // This function is called once every call to Fighter::update
    virtual void update(const Controller&, float dt) = 0;
    // TODO description
    virtual void render(float dt) = 0;
    // This function is called once every call to Fighter::collisionWithGround
    virtual void collisionWithGround(const Rectangle &ground, bool collision) = 0;
    // This function is called when Fighter::hitByAttack is called, before any
    // other work is done
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack) = 0;

protected:
    Fighter *fighter_;
    FighterState *next_;

    void calculateHitResult(const Fighter *fighter, const Attack *attack);
};


class Fighter
{
public:
    Fighter(float respawnx, float respawny, const glm::vec3 &color);
    ~Fighter();

    void update(const Controller&, float dt);
    void render(float dt);

    int getLives() const;
    float getDamage() const;
    float getDirection() const; // returns -1 or 1

    // collision is true if there is a collision with ground this frame, false otherwise
    void collisionWithGround(const Rectangle &ground, bool collision);
    void attackCollision(); // Called when two attacks collide
    void hitByAttack(const Fighter *fighter, const Attack* attack);  // Called when hit by an attack
    void hitWithAttack(); // Called when you hit with an attack
    // Gets the fighter's hitbox
    const Rectangle& getRectangle() const;

    // Returns true if this Fighter is currently attacking and has an attack hitbox
    bool hasAttack() const;
    const Attack * getAttack() const;

    // Respawns the fighter at its respawn location.  If killed is true, a
    // life be removed
    void respawn(bool killed);
    // True if this player has more than 0 lives
    bool isAlive() const;

private:
    // Game state members
    Rectangle rect_;
    float xvel_, yvel_;
    float dir_; // 1 or -1 look in xdir
    FighterState *state_;
    float damage_;
    int lives_;

    // Fighter ID members
    float respawnx_, respawny_;
    glm::vec3 color_;

    // Current attack members
    Attack* attack_;

    // Available reference attacks
    Attack dashAttack_;
    Attack neutralTiltAttack_;
    Attack sideTiltAttack_;
    Attack downTiltAttack_;
    Attack upTiltAttack_;

    Attack airNeutralAttack_;
    Attack airSideAttack_;
    Attack airDownAttack_;
    Attack airUpAttack_;

    // Fighter stats
    const float walkSpeed_; // maximum walking speed
    const float dashSpeed_; // Dashing Speed
    const float jumpStartupTime_; // Delay before jump begins, also short hop/full jump control time
    const float dashStartupTime_; // Time from starting dash to first movement
    const float jumpSpeed_; // Speed of a full jump
    const float hopSpeed_; // Speed of a short hop

    const float airForce_; // Force applied to allow player air control
    const float airAccel_; // "Gravity"
    const float jumpAirSpeed_; // The maximum x speed for jumping (only for player control)
    const float secondJumpSpeed_; // Speed of the second jump

    // Input response parameters
    const float inputVelocityThresh_;
    const float inputJumpThresh_;
    const float inputDashThresh_;
    const float inputDashMin_;
    const float inputDeadzone_;
    const float inputTiltThresh_;

    // ---- Helper functions ----
    float damageFunc() const; // Returns a scaling factor based on damage
    // Loads an attack from the params using the attackName.param syntax
    Attack loadAttack(std::string attackName,
            std::string soundFile = "");
    void renderHelper(float dt, const glm::vec3& color);

    friend class FighterState;
    friend class GroundState;
    friend class DashState;
    friend class AirNormalState;
    friend class AirStunnedState;
    friend class DeadState;
};

class GroundState : public FighterState
{
public:
    GroundState(Fighter *f);
    virtual ~GroundState();

    virtual void update(const Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack);

private:
    // Jump startup timer.  Value >= 0 implies that the fighter is starting a jump
    float jumpTime_;
    // Dash startup timer.  Value >= 0 implies that the fighter is start to dash
    float dashTime_;
    // Dash change direction timer
    float dashChangeTime_;
    bool dashing_;
};

class AirNormalState : public FighterState
{
public:
    AirNormalState(Fighter *f);
    virtual ~AirNormalState();

    virtual void update(const Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack);

private:
    // True if the player has a second jump available
    bool canSecondJump_;
    // Jump startup timer.  Value > 0 implies that the fighter is starting a jump
    float jumpTime_;
};

class AirStunnedState : public FighterState
{
public:
    AirStunnedState(Fighter *f, float duration);
    virtual ~AirStunnedState();

    virtual void update(const Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack);

private:
    float stunDuration_;
    float stunTime_;
};

class DeadState : public FighterState
{
public:
    DeadState(Fighter *f) : FighterState(f)
    {
        f->rect_.x = HUGE_VAL;
        f->rect_.y = HUGE_VAL;
    };
    virtual ~DeadState() {};

    virtual void update(const Controller&, float dt) { }
    virtual void render(float dt) { }
    virtual void collisionWithGround(const Rectangle &ground, bool collision) { assert(false); }
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack) { assert(false); }
};
