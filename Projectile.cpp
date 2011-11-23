#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "Fighter.h"
#include "FrameManager.h"

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

    attack_ = new ProjectileHelperAttack(
            getParam(paramPrefix_ + "knockbackpow") *
            glm::normalize(glm::vec2(getParam(paramPrefix_ + "knockbackx"),
                      getParam(paramPrefix_ + "knockbacky"))),
            getParam(paramPrefix_ + "damage"),
            getParam(paramPrefix_ + "stun"),
            pos_, size_, playerID_,
            audioID_);
}

Projectile::~Projectile()
{
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

    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(1, 1, 1, 0.3)),
            frameName_);
}


// ---- Projectile Attack Methods ----
ProjectileHelperAttack::ProjectileHelperAttack(const glm::vec2 &kb, float damage, float stun,
        const glm::vec2 &pos, const glm::vec2 &size, int playerID, const std::string &audioID) :
    Attack(),
    playerID_(playerID),
    pos_(pos),
    size_(size),
    kb_(kb),
    audioID_(audioID)
{
    damage_ = damage;
    stun_ = stun;
}

ProjectileHelperAttack::~ProjectileHelperAttack()
{
    /* empty */
}

Attack * ProjectileHelperAttack::clone() const
{
    return new ProjectileHelperAttack(*this);
}

Rectangle ProjectileHelperAttack::getHitbox() const
{
    return Rectangle(pos_.x, pos_.y, size_.x, size_.y);
}

glm::vec2 ProjectileHelperAttack::getKnockback(const GameEntity *) const
{
    return kb_;
}

int ProjectileHelperAttack::getPlayerID() const
{
    return playerID_;
}

void ProjectileHelperAttack::render(float dt)
{
    // Should not be called
    assert(false);
}

std::string ProjectileHelperAttack::getAudioID() const
{
    return audioID_;
}

void ProjectileHelperAttack::setPosition(const glm::vec2 &position)
{
    pos_ = position;
}