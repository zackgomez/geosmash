#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "Fighter.h"
#include "FrameManager.h"
#include "Attack.h"
#include "PManager.h"
Projectile::Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
        const std::string &paramPrefix, const std::string &frameName,
        const std::string &audioID, int playerID, int teamID) :
    GameEntity(),
    attack_(NULL),
    t_(0),
    hit_(false),
    frameName_(frameName),
    audioID_(audioID)
{
    paramPrefix_ = paramPrefix + '.';
    playerID_ = playerID;
    teamID_ = teamID;

    pos_ = pos;
    vel_ = dir * getParam(paramPrefix_ + "speed");
    accel_ = glm::vec2(0.f);

    size_ = glm::vec2(getParam(paramPrefix_ + "sizex"),
            getParam(paramPrefix_ + "sizey"));

    glm::vec2 kbdir = glm::normalize(glm::vec2(
                getParam(paramPrefix_ + "knockbackx"),
                getParam(paramPrefix_ + "knockbacky")));
    kbdir *= glm::vec2(glm::sign(vel_.x), 1.f);

    attack_ = new SimpleAttack(
            kbdir,
            getParam(paramPrefix_ + "kbbase"),
            getParam(paramPrefix_ + "kbscaling"),
            getParam(paramPrefix_ + "damage"),
            getParam(paramPrefix_ + "stun"),
            0.f, // 0 priority
            pos_, size_, -dir.x, playerID_, teamID_,
            audioID_);

    glm::vec4 pcolors_raw[] =
    {
        glm::vec4(0.1, 0.2, 0.8, 0.4),
        glm::vec4(0.1, 0.2, 0.8, 0.8),
        glm::vec4(0.7, 0.7, 0.7, 0.4),
        glm::vec4(0.1, 0.4, 0.8, 0.4),
    };
    std::vector<glm::vec4> pcolors;
    pcolors.assign(pcolors_raw, pcolors_raw + sizeof(pcolors_raw) / sizeof(glm::vec4));
    emitter_ = ParticleManager::get()->newEmitter();
    emitter_->setLocation(glm::vec3(pos_, 0.0f))
        ->setParticleLifetimeF(new lifetimeNormalF(0.06, 0.04))
        ->setParticleVelocityF(new velocityF(20.f, 20.f, 5.f))
        ->setParticleLocationF(new locationF(4.f))
        ->setParticleColorF(new discreteColorF(pcolors))
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
    // Should never be hit, attack collision takes care of it
    assert(false);
}

void Projectile::attackConnected(GameEntity *other)
{
    // Can't hit ourself
    if (other->getPlayerID() == playerID_) return;

    // Hit the other thing
    other->hitByAttack(attack_);
    // no more hits
    hit_ = true;
}

void Projectile::collisionWithGround(const rectangle &rect, bool collision)
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
    emitter_->setLocation(glm::vec3(pos_, 0.0f));
    /*
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(1.f));

    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(1, 1, 1, 0.3)),
            frameName_);
            */
}

