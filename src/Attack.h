#pragma once
#include <string>
#include <glm/glm.hpp>
#include <set>

class Fighter;
class rectangle;
class GameEntity;

// Attack base class consisting entirely of pure virtual functions.
// Just defines the essential duties of any attack.
class Attack
{
public:
    Attack() { }
    virtual ~Attack() { }

    // Simple accessors
    virtual rectangle getHitbox() const = 0;
    virtual float getPriority() const = 0;
    virtual float getDamage() const = 0;

    // For use in calculating results
    virtual float calcStun(const GameEntity *victim, float damage) const = 0;
    virtual glm::vec2 calcKnockback(const GameEntity *victim, float damage) const = 0;
    virtual float getOriginDirection(const GameEntity *victim) const = 0;

    virtual int getPlayerID() const = 0;
    virtual int getTeamID() const = 0;
    virtual std::string getAudioID() const = 0;

    virtual bool canHit(const GameEntity *f) const = 0;
    virtual void attackCollision(const Attack *other) = 0;
    virtual void hit(GameEntity *other) = 0;
};

class SimpleAttack : public Attack
{
public:
    SimpleAttack() { }
    SimpleAttack(const glm::vec2 &kbdir, float kbbase, float kbscaling,
            float damage, float stun, float priority,
            const glm::vec2 &pos, const glm::vec2 &size, float odir,
            int playerID, int teamID,
            const std::string &audioID);
    virtual ~SimpleAttack();

    virtual rectangle getHitbox() const;
    virtual float getPriority() const;
    virtual float getDamage() const;

    virtual float calcStun(const GameEntity *victim, float damage) const;
    virtual glm::vec2 calcKnockback(const GameEntity *victim, float damage) const;
    virtual float getOriginDirection(const GameEntity *victim) const;

    virtual int getPlayerID() const;
    virtual int getTeamID() const;
    virtual std::string getAudioID() const;

    // True if this attack can hit the passed GameEntity now
    virtual bool canHit(const GameEntity *) const;
    virtual void attackCollision(const Attack *other);
    virtual void hit(GameEntity *other);

    // Allows all Entities to be hit by this attack
    virtual void clearHit();

    // Non inherited functions...
    void setPosition(const glm::vec2 &position);
    //void setKBDirection(float dir);
    void setOriginDirection(float odir);
    void setBaseKnockback(float pow);
    void reown(int playerID, int teamID);

protected:
    int playerID_, teamID_;
    
    float damage_, stun_, priority_;
    float odir_;

    glm::vec2 pos_;
    glm::vec2 size_;
    glm::vec2 kbdir_;
    float kbbase_;
    float kbscaling_;
    
    float dir_;

    std::string audioID_;
    std::string paramPrefix_;

    std::set<int> hasHit_;
};

class FighterAttack : public SimpleAttack
{
public:
    FighterAttack();

    FighterAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual ~FighterAttack() {}

    virtual FighterAttack* clone() const;

    virtual rectangle getHitbox() const;
    virtual float getOriginDirection(const GameEntity *victim) const;
    virtual glm::vec2 calcKnockback(const GameEntity *victim, float damage) const;

    void setFighter(Fighter *fighter);
    virtual int getPlayerID() const;

    // Called when the move is started
    virtual void start();
    // Called when the move is finished and going to be removed
    virtual void finish();

    // If hitbox can hit another player
    virtual bool hasHitbox() const;
    // If this attack is over
    virtual bool isDone() const;

    // Updates internal timer
    virtual void update(float dt);
    // Renders any attack specific stuff (in this case, the hitbox)
    virtual void render(float dt);
    // Sends to cooldown time
    virtual void cancel();
    // Ends attack altogether (will be removed)
    virtual void kill();
    // Called when two attacks collide
    virtual void attackCollision(const Attack *other);

    virtual std::string getFrameName() const;
    virtual bool hasTwinkle() const;

    void setTwinkle(bool twinkle);
    void setFrameName(const std::string &fname);
    void setHitboxFrame(const std::string &frame);
    // Played during start up
    void setStartSound(const std::string &soundID_);
    // Played when the hitbox first becomes active
    void setActiveSound(const std::string &soundID_);


protected:
    glm::vec2 hboffset_;
    glm::vec2 hbsize_;

    float startup_, duration_, cooldown_;
    float t_;

    std::string frameName_;
    std::string hbframe_;
    bool twinkle_;

    std::string startSoundID_;
    std::string activeSoundID_;
    bool activeSoundPlayed_;

    Fighter *owner_;
};

// An attack with a hitbox that moves, like the current Up Smash
class MovingHitboxAttack : public FighterAttack
{
public:
    MovingHitboxAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack *clone() const;
    virtual rectangle getHitbox() const;

private:
    glm::vec2 hb0, hb1;
};

// kbdir is based on direction of victim from hitbox center
class VaryingDirectionAttack : public FighterAttack
{
public:
    VaryingDirectionAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack *clone() const;
    virtual glm::vec2 calcKnockback(const GameEntity * victim, float damage) const;
};


// An attack that is responsible for moving the character.
// (Current Up - B 0.7)
class MovingAttack : public FighterAttack
{
public:
    MovingAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);


    virtual FighterAttack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);
    virtual void hit(GameEntity *victim);

protected:
    void setVelocity(const glm::vec2 &);
    bool started_;
};

// The Up-B attack class. 
class UpSpecialAttack : public MovingAttack
{
public:
    UpSpecialAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);
    virtual void hit(GameEntity *victim);

private:
    float repeatTime_;
    bool started_;
};

class RepeatingAttack : public FighterAttack
{
public:
    RepeatingAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack* clone() const;
    virtual void update(float dt);

private:
    float repeatT_;
};

class DashAttack : public FighterAttack
{
public:
    DashAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);

private:
    float deceleration_;
    float initialSpeed_;
};
