#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "Fighter.h"

Projectile::Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
        const std::string &paramPrefix, const std::string &frameName,
        int playerID) :
    GameEntity(),
    attack_(NULL),
    hit_(false),
    frameName_(frameName)
{
    paramPrefix_ = paramPrefix + '.';
    playerID_ = playerID;

    pos_ = pos;
    vel_ = dir * getParam(paramPrefix + "speed");
    accel_ = glm::vec2(0.f);

    size_ = glm::vec2(getParam(paramPrefix + "sizex"),
            getParam(paramPrefix + "sizey"));

    attack_ = new ProjectileAttack(
            glm::vec2(getParam(paramPrefix + "knockbackx"),
                      getParam(paramPrefix + "knockbacky")),
            getParam(paramPrefix + "damage"),
            getParam(paramPrefix + "stun"),
            pos_, size_, playerID_);
}

Projectile::~Projectile()
{
    delete attack_;
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
    // TODO have some sort of life amount that this attack has...

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
    hit_ = true;
}

void Projectile::update(float dt)
{
    GameEntity::update(dt);

    // TODO have some max lifetime here
}

void Projectile::render(float dt)
{
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(size_, 1.f));

    renderRectangle(transform, glm::vec4(glm::vec4(1, 1, 1, 1)));
}


// ---- Projectile Attack Methods ----
ProjectileAttack::ProjectileAttack(const glm::vec2 &kb, float damage, float stun,
        const glm::vec2 &pos, const glm::vec2 &size, int playerID) :
    Attack(),
    playerID_(playerID),
    pos_(pos),
    size_(size),
    kb_(kb)
{
    damage_ = damage;
    stun_ = stun;
}

ProjectileAttack::~ProjectileAttack()
{
    /* empty */
}

Attack * ProjectileAttack::clone() const
{
    return new ProjectileAttack(*this);
}

Rectangle ProjectileAttack::getHitbox() const
{
    return Rectangle(pos_.x, pos_.y, size_.x, size_.y);
}

glm::vec2 ProjectileAttack::getKnockback(const Fighter *)
{
    return kb_;
}

int ProjectileAttack::getPlayerID() const
{
    return playerID_;
}

void ProjectileAttack::render(float dt)
{
    // Should not be called
    assert(false);
}

void ProjectileAttack::setPosition(const glm::vec2 &position)
{
    pos_ = position;
}
