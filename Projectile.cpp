#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "Fighter.h"
#include "FrameManager.h"
#include "Attack.h"
#include "ParticleManager.h"
Projectile::Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
        const std::string &paramPrefix, const std::string &frameName,
        const std::string &audioID, int playerID) :
    GameEntity(),
    attack_(NULL),
    t_(0),
    hit_(false),
    frameName_(frameName),
    audioID_(audioID)
{
    paramPrefix_ = paramPrefix + '.';
    playerID_ = playerID;

    pos_ = pos;
    vel_ = dir * getParam(paramPrefix_ + "speed");
    accel_ = glm::vec2(0.f);

    size_ = glm::vec2(getParam(paramPrefix_ + "sizex"),
            getParam(paramPrefix_ + "sizey"));

    attack_ = new SimpleAttack(
            getParam(paramPrefix_ + "knockbackpow") *
            glm::normalize(glm::vec2(
                    getParam(paramPrefix_ + "knockbackx"),
                    getParam(paramPrefix_ + "knockbacky"))),
            getParam(paramPrefix_ + "damage"),
            getParam(paramPrefix_ + "stun"),
            0.f, // 0 priority
            pos_, size_, playerID_,
            audioID_);
    attack_->setKBDirection(vel_.x > 0 ? 1 : -1);

    emitter_ = ParticleManager::get()->newEmitter();
    emitter_->setLocation(glm::vec3(pos_, 0.0f))
        ->setParticleLifetime(0.05f)
        ->setParticleLifetimeF(new lifetimeVarianceF(0.3))
        ->setParticleVelocity(20)
        ->setParticleColor(glm::vec4(0.4, 0.4, 0.8, 0.5))
        ->setParticleColorVariance(glm::vec4(0.2, 0.2, 0.2, 0.5))
        ->setParticleColorBrightness(0.8f, 0.3f)
        ->setOutputRate(2000);
    ParticleManager::get()->addEmitter(emitter_);

}

Projectile::~Projectile()
{
    
    ParticleManager::get()->quashEmitter(emitter_);
    delete attack_;
}

bool Projectile::isDone() const
{
    return hit_;
}

bool Projectile::hasAttack() const
{
    return !hit_;
}

const Attack * Projectile::getAttack() const
{
    attack_->setPosition(pos_);
    return attack_;
}

bool Projectile::canBeHit() const
{
    // Cannot be hit, only hitbox collisions
    return false;
}

void Projectile::attackCollision(const Attack *other)
{
    // For now, just make the attack go away
    hit_ = true;
}

void Projectile::hitByAttack(const Attack *attack)
{
    // Should never be hit
    assert(false);
}

void Projectile::attackConnected(GameEntity *other)
{
    // Can't hit ourself
    if (other->getPlayerID() == playerID_) return;

    hit_ = true;
}

void Projectile::collisionWithGround(const Rectangle &rect, bool collision)
{
    // XXX Ignore it for now..
}

void Projectile::update(float dt)
{
    GameEntity::update(dt);
    t_ += dt;

    if (t_ > getParam(paramPrefix_ + "lifetime"))
        hit_ = true;

    printf("PROJECTILE | t: %f  Pos: [%f %f]  Vel: [%f %f]\n",
            t_, pos_.x, pos_.y, vel_.x, vel_.y);
}

void Projectile::render(float dt)
{
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(1.f));

    emitter_->setLocation(glm::vec3(pos_, 0.0f));
    /*
    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(1, 1, 1, 0.3)),
            frameName_);
            */
}

