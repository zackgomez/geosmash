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
    Attack();

    Attack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual ~Attack() {}

    virtual Attack* clone() const;

    virtual bool hasTwinkle() const;
    virtual Rectangle getHitbox() const;
    virtual float getDamage(const Fighter *fighter) const;
    virtual float getStun(const Fighter *fighter) const;
    virtual glm::vec2 getKnockback(const Fighter *fighter) const;

    void setFighter(Fighter *fighter);
    void setFrameName(const std::string &fname);
    const GameEntity *getOwner() const;

    // Called when the move is started
    virtual void start();
    // Called when the move is finished and going to be removed
    virtual void finish();

    // If hitbox can hit another player
    virtual bool hasHitbox() const;
    // If this attack is over
    virtual bool isDone() const;
    // True if this attack can hit the fighter right now
    bool canHit(const GameEntity *f) const;

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

    virtual std::string getAudioID() const;
    virtual std::string getFrameName() const;

    void setTwinkle(bool twinkle);
    void setHitboxFrame(const std::string &frame);

protected:
    glm::vec2 hboffset_;
    glm::vec2 hbsize_;

    float startup_, duration_, cooldown_;
    float damage_, stun_;
    float priority_;
    glm::vec2 knockbackdir_;
    float knockbackpow_;
    std::set<int> hasHit_;
    float t_;

    std::string audioID_;
    std::string frameName_;
    std::string hbframe_;
    bool twinkle_;

    Fighter *owner_;
};

class MovingAttack : public Attack
{
public:
    MovingAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual Attack *clone() const;
    virtual Rectangle getHitbox() const;

private:
    glm::vec2 hb0, hb1;
};


class UpSpecialAttack : public Attack
{
public:
    UpSpecialAttack() : Attack() {};
    UpSpecialAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);


    virtual Attack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);
    virtual void hit(GameEntity *victim);

private:
    float repeatTime_;
    bool started_;
};

class DashAttack : public Attack
{
public:
    DashAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName);

    virtual Attack* clone() const;
    virtual void start();
    virtual void finish();
    virtual void update(float dt);

private:
    float deceleration_;
    float initialSpeed_;
};

