#pragma once
#include "GameEntity.h"

class ProjectileAttack;

class Projectile : public GameEntity
{
public:
    Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
            const std::string &paramPrefix, const std::string &frameName);
    virtual ~Projectile();

    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;

    virtual bool canBeHit() const;
    virtual void attackCollision(const Attack *other);
    virtual void hitByAttack(const Attack *attack);
    virtual void attackConnected(GameEntity *other);

    virtual void update(float dt);
    virtual void render(float dt);

private:
    std::string paramPrefix_;
    ProjectileAttack *attack_;
    bool hit_;
    std::string frameName_;
};




class ProjectileAttack : public Attack
{
public:
    ProjectileAttack(const glm::vec2 &kb, float damage, float stun,
            const glm::vec2 &pos, const glm::vec2 &size,
            const std::string &frame);

    virtual Attack * clone() const;

    virtual Rectangle getHitbox() const;
    virtual glm::vec2 getKnockback(const Fighter *);
    virtual int getPlayerID() const;

    virtual void render(float dt);

    void setPosition(const glm::vec2 &position);

private:
    int playerID_;
};

