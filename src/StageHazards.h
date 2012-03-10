#pragma once
#include <glm/glm.hpp>
#include "GameEntity.h"

class Emitter;
class SimpleAttack;

class VolcanoHazard : public GameEntity
{
public:
    VolcanoHazard(const glm::vec2 &pos);
    virtual ~VolcanoHazard();

    virtual std::string getType() const { return "VolcanoHazard"; }

    virtual bool isDone() const;
    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;
    virtual bool canBeHit() const { return false; }

    virtual void update(float dt);

    // GameEntity overrides
    virtual void render(float dt);
    virtual void attackCollision(const Attack*);
    virtual void attackConnected(GameEntity*);
    virtual void collisionWithGround(const rectangle&, bool, bool);
    virtual void hitByAttack(const Attack*);

private:
    SimpleAttack *attack_;
    std::string pre_;
    float t_;
    bool active_;

    Emitter *emitter_;
};

