#pragma once
#include "GameEntity.h"
#include <string>

class SimpleAttack;

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
    SimpleAttack *attack_;

    float t_;
    bool hit_;
    std::string frameName_;
    std::string audioID_;
};

