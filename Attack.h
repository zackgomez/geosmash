#pragma once
#include <string>
#include <glm/glm.hpp>
#include <set>

class Fighter;
class Rectangle;
class GameEntity;

class Attack
{
public:
    Attack() { }
    virtual ~Attack() { }

    virtual Rectangle getHitbox() const = 0;
    virtual float getPriority() const = 0;
    virtual float getDamage(const GameEntity *victim) const = 0;
    virtual float getStun(const GameEntity *victim) const = 0;
    virtual glm::vec2 getKnockback(const GameEntity *victim) const = 0;

    virtual int getPlayerID() const = 0;
    virtual std::string getAudioID() const = 0;

    virtual bool canHit(const GameEntity *f) const = 0;
    virtual void attackCollision(const Attack *other) = 0;

};

class SimpleAttack : public Attack
{
public:
    SimpleAttack() { }
    SimpleAttack(const glm::vec2 &kb, float damage, float stun, float priority,
            const glm::vec2 &pos, const glm::vec2 &size, int playerID,
            const std::string &audioID);
    virtual ~SimpleAttack();

    virtual Rectangle getHitbox() const;
    virtual float getPriority() const;
    virtual float getDamage(const GameEntity *) const;
    virtual float getStun(const GameEntity *) const;
    virtual glm::vec2 getKnockback(const GameEntity *) const;

    virtual int getPlayerID() const;
    virtual std::string getAudioID() const;

    // True if this attack can hit the passed GameEntity now
    virtual bool canHit(const GameEntity *) const;
    virtual void attackCollision(const Attack *other);


    // Non inherited functions...
    void setPosition(const glm::vec2 &position);

protected:
    int playerID_;
    
    float damage_, stun_, priority_;

    glm::vec2 pos_;
    glm::vec2 size_;
    glm::vec2 kb_;

    std::string audioID_;

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

    virtual Rectangle getHitbox() const;
    virtual glm::vec2 getKnockback(const GameEntity *fighter) const;

    void setFighter(Fighter *fighter);
    void setFrameName(const std::string &fname);
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
    // Called when the attack 'connects'
    virtual void hit(GameEntity *other);
    // Called when two attacks collide
    virtual void attackCollision(const Attack *other);

    virtual std::string getFrameName() const;
    virtual bool hasTwinkle() const;

    void setTwinkle(bool twinkle);
    void setHitboxFrame(const std::string &frame);

protected:
    glm::vec2 hboffset_;
    glm::vec2 hbsize_;

    float startup_, duration_, cooldown_;
    glm::vec2 knockbackdir_;
    float knockbackpow_;
    float t_;

    std::string frameName_;
    std::string hbframe_;
    bool twinkle_;

    Fighter *owner_;
};

class MovingAttack : public FighterAttack
{
public:
    MovingAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual FighterAttack *clone() const;
    virtual Rectangle getHitbox() const;

private:
    glm::vec2 hb0, hb1;
};


class UpSpecialAttack : public FighterAttack
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

class NeutralSpecialAttack : public FighterAttack
{
public:
    NeutralSpecialAttack(const std::string &paramPrefix,
            const std::string &frameName);

    virtual FighterAttack *clone() const;
    virtual bool hasHitbox() const;
    virtual void update(float dt);
    virtual void start();

private:
    std::string paramPrefix_;
    bool released_;
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
