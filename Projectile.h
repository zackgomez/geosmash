#pragma once
#include "GameEntity.h"
#include <string>

class Emitter;
class SimpleAttack;

class Projectile : public GameEntity
{
public:
    Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
            const std::string &paramPrefix, const std::string &frameName,
            const std::string &audioID, int playerID, int teamID);
    virtual ~Projectile();

    virtual std::string getType() const { return "ProjectileEntity"; }

    virtual bool isDone() const;

    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;

    virtual bool canBeHit() const;
    virtual void attackCollision(const Attack *other);
    virtual void hitByAttack(const Attack *attack);
    virtual void attackConnected(GameEntity *other);

    virtual void collisionWithGround(const rectangle &ground, bool collision);

    virtual void update(float dt);
    virtual void render(float dt);

private:
    std::string paramPrefix_;
    SimpleAttack *attack_;

    float t_;
    bool hit_;
    std::string frameName_;
    std::string audioID_;
    Emitter *emitter_;
};

