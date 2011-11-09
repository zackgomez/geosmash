#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <SFML/Audio.hpp>
#include <map>

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
    bool contains(const Rectangle &rhs) const;

    float x, y, w, h;
};

class Attack
{
public:
    Attack() {}
    Attack(float startup, float duration, float cooldown, float damage, float stun,
            const glm::vec2& knockback, const Rectangle &hitbox, float priority,
            const std::string &audioFileprefix = "groundhit") :
        hitbox_(hitbox),
        startup_(startup), duration_(duration), cooldown_(cooldown),
        damage_(damage), stun_(stun), priority_(priority), knockback_(knockback), 
        hasHit_(), t_(0.0f), audioID_(audioFileprefix), frameName_("GroundUptilt"),
        owner_(NULL)
    {}

    virtual ~Attack() {}

    virtual Attack* clone() const;

    virtual Rectangle getHitbox() const;
    virtual float getDamage(const Fighter *fighter) const { return damage_; }
    virtual float getStun(const Fighter *fighter) const { return stun_; }
    virtual glm::vec2 getKnockback(const Fighter *fighter) const { return knockback_; }

    void setFighter(Fighter *fighter);
    void setFrameName(const std::string &fname) { frameName_ = fname; }
    const Fighter *getOwner() const;

    // Called when the move is started
    virtual void start();
    // Called when the move is finished and going to be removed
    virtual void finish();

    // If hitbox can hit another player
    virtual bool hasHitbox() const;
    // If hitbox should be drawn
    virtual bool drawHitbox() const;
    // If this attack is over
    virtual bool isDone() const;
    // True if this attack can hit the fighter right now
    bool canHit(const Fighter *f) const;

    // Updates internal timer
    virtual void update(float dt);
    // Sends to cooldown time
    virtual void cancel();
    // Called when the attack 'connects'
    virtual void hit(Fighter *other);
    // Called when two attacks collide
    virtual void attackCollision(const Attack *other);

    virtual std::string getAudioID() const { return audioID_; }
    virtual std::string getFrameName() const { return frameName_; }

protected:
    Rectangle hitbox_;
    float startup_, duration_, cooldown_;
    float damage_, stun_;
    float priority_;
    glm::vec2 knockback_;
    bool hasHit_[4];
    float t_;

    std::string audioID_;
    std::string frameName_;

    Fighter *owner_;
};

class UpSpecialAttack : public Attack
{
public:
    UpSpecialAttack() : Attack() {};

    UpSpecialAttack(float startup, float duration, float cooldown, float damage, float stun,
            const glm::vec2& knockback, const Rectangle &hitbox, float priority,
            const std::string &audioFileprefix = "groundhit") :
        Attack(startup, duration, cooldown, damage, stun, knockback, hitbox, priority),
        repeatTime_(0.0f)
        {
        }

    virtual Attack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);
    virtual void hit(Fighter *victim);

private:
    float repeatTime_;
    bool started_;
};

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
    std::string frameName_;

    void calculateHitResult(const Fighter *fighter, const Attack *attack);
    void collisionHelper(const Rectangle &ground);
};


class Fighter
{
public:
    Fighter(float respawnx, float respawny, const glm::vec3 &color, int id);
    ~Fighter();

    void update(const Controller&, float dt);
    void render(float dt);

    int getID() const { return id_; }
    int getLives() const;
    float getDamage() const;
    float getDirection() const; // returns -1 or 1
    // Returns the id of the player that last hit this fighter, or -1 if there
    // is none
    int getLastHitBy() const;
    // True if this player has more than 0 lives
    bool isAlive() const;

    // collision is true if there is a collision with ground this frame, false otherwise
    void collisionWithGround(const Rectangle &ground, bool collision);
    void attackCollision(const Attack *attack); // Called when two attacks collide, with other attack
    void hitByAttack(const Fighter *fighter, const Attack* attack);  // Called when hit by an attack
    void hitWithAttack(Fighter *victim); // Called when you hit with an attack
    // Gets the fighter's hitbox
    const Rectangle& getRectangle() const;

    // Returns true if this Fighter is currently attacking and has an attack hitbox
    bool hasAttack() const;
    const Attack * getAttack() const;

    // Respawns the fighter at its respawn location.  If killed is true, a
    // life be removed
    void respawn(bool killed);

private:
    // Game state members
    Rectangle rect_;
    float xvel_, yvel_;
    float dir_; // 1 or -1 look in xdir
    FighterState *state_;
    float damage_;
    int lives_;
    // the lastHitBy_ variable should be reset

    // Fighter ID members
    float respawnx_, respawny_;
    glm::vec3 color_;
    int id_;

    // Current attack members
    Attack* attack_;

    // Game statistics members
    int lastHitBy_; // The id of the fighter that last hit us, or -1

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

    UpSpecialAttack upSpecialAttack_;

    // ---- Helper functions ----
    float damageFunc() const; // Returns a scaling factor based on damage
    // Loads an attack from the params using the attackName.param syntax
    void renderHelper(float dt, const std::string &frameName, const glm::vec3& color);
    template<class AttackClass>
    AttackClass loadAttack(std::string attackName, const std::string &audioID,
            const std::string &frameName);

    friend class FighterState;
    friend class GroundState;
    friend class DashState;
    friend class AirNormalState;
    friend class AirStunnedState;
    friend class DeadState;
    friend class RespawnState;
    friend class UpSpecialAttack;
};

class GroundState : public FighterState
{
public:
    GroundState(Fighter *f, float delay = -1.0f);
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
    // Generic wait time, wait while value > 0
    float waitTime_;
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

class RespawnState : public FighterState
{
public:
    RespawnState(Fighter *f);
    virtual ~RespawnState() {}

    virtual void update(const Controller&, float dt);
    virtual void render(float dt);
    virtual void collisionWithGround(const Rectangle &ground, bool collision);
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack);

private:
    float t_;
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
    virtual void render(float dt) { };
    virtual void collisionWithGround(const Rectangle &ground, bool collision) { assert(false); }
    virtual void hitByAttack(const Fighter *attacker, const Attack *attack) { assert(false); }
};
