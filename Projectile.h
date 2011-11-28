#pragma once
#include "GameEntity.h"
#include <string>
#include "Attack.h"

class ProjectileHelperAttack;

class Projectile : public GameEntity
{
public:
    Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
            const std::string &paramPrefix, const std::string &frameName,
            const std::string &audioID, const int playerID);
    virtual ~Projectile();

    virtual bool isDone() const;

    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;

    virtual bool canBeHit() const;
    virtual void attackCollision(const Attack *other);
    virtual void hitByAttack(const Attack *attack);
    virtual void attackConnected(GameEntity *other);

    virtual void collisionWithGround(const Rectangle &ground, bool collision);

    virtual void update(float dt);
    virtual void render(float dt);

private:
    std::string paramPrefix_;
    ProjectileHelperAttack *attack_;

    float t_;
    bool hit_;
    std::string frameName_;
    std::string audioID_;
};


class ProjectileHelperAttack : public Attack
{
public:
    ProjectileHelperAttack(const glm::vec2 &kb, float damage, float stun,
            const glm::vec2 &pos, const glm::vec2 &size, int playerID,
            const std::string &audioID);
    virtual ~ProjectileHelperAttack();

    virtual Attack * clone() const;

    virtual Rectangle getHitbox() const;
    virtual glm::vec2 getKnockback(const GameEntity *) const;
    virtual int getPlayerID() const;

    virtual void render(float dt);

    virtual std::string getAudioID() const;

    void setPosition(const glm::vec2 &position);

private:
    int playerID_;
    glm::vec2 pos_;
    glm::vec2 size_;
    glm::vec2 kb_;

    std::string audioID_;
};
