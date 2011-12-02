#include "Attack.h"
#include "ParamReader.h"
#include "Fighter.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "explosion.h"
#include "FrameManager.h"
#include "audio.h"
#include "Projectile.h"


// XXX remove this
void addEntity(GameEntity *ent);

// ----------------------------------------------------------------------------
// SimpleAttack class methods
// ----------------------------------------------------------------------------
SimpleAttack::SimpleAttack(const glm::vec2 &kb,
        float damage, float stun, float priority,
        const glm::vec2 &pos, const glm::vec2 &size,
        int playerID, const std::string &audioID) :
    Attack(),
    playerID_(playerID),
    damage_(damage), stun_(stun), priority_(priority),
    pos_(pos),
    size_(size),
    kb_(kb),
    audioID_(audioID)
{
}

SimpleAttack::~SimpleAttack()
{
    /* empty */
}

Rectangle SimpleAttack::getHitbox() const
{
    return Rectangle(pos_.x, pos_.y, size_.x, size_.y);
}

float SimpleAttack::getPriority() const { return priority_; }
float SimpleAttack::getDamage(const GameEntity *) const { return damage_; }
float SimpleAttack::getStun(const GameEntity *) const { return stun_; }

glm::vec2 SimpleAttack::getKnockback(const GameEntity *) const
{
    return kb_;
}

int SimpleAttack::getPlayerID() const
{
    return playerID_;
}

std::string SimpleAttack::getAudioID() const
{
    return audioID_;
}

void SimpleAttack::setPosition(const glm::vec2 &position)
{
    pos_ = position;
}

bool SimpleAttack::canHit(const GameEntity *f) const
{
    return hasHit_.find(f->getID()) == hasHit_.end();
}

void SimpleAttack::attackCollision(const Attack *other)
{
    // Do nothing
}


// ----------------------------------------------------------------------------
// FighterAttack class methods
// ----------------------------------------------------------------------------

FighterAttack::FighterAttack()
{
}

FighterAttack::FighterAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName)
{
    std::string pp = paramPrefix + '.';
    startup_ = getParam(pp + "startup");
    duration_ = getParam(pp + "duration");
    cooldown_ = getParam(pp + "cooldown");
    damage_ = getParam(pp + "damage");
    stun_ = getParam(pp + "stun");
    knockbackdir_ = glm::vec2(
                getParam(pp + "knockbackx"),
                getParam(pp + "knockbacky"));
    knockbackpow_ = getParam(pp + "knockbackpow");
    hboffset_ = glm::vec2(getParam(pp + "hitboxx"), getParam(pp + "hitboxy"));
    hbsize_ = glm::vec2(getParam(pp + "hitboxw"), getParam(pp + "hitboxh"));
    priority_ = getParam(pp + "priority");
    audioID_ = audioID;
    frameName_ = frameName;
    paramPrefix_ = pp;

    twinkle_ = false;

    knockbackdir_ = knockbackdir_ == glm::vec2(0, 0) ? knockbackdir_ : glm::normalize(knockbackdir_);
}

FighterAttack* FighterAttack::clone() const
{
    return new FighterAttack(*this);
}

void FighterAttack::setFighter(Fighter *fighter)
{
    owner_ = fighter;
}

int FighterAttack::getPlayerID() const
{
    return owner_->getPlayerID();
}

void FighterAttack::start()
{
    assert(owner_);
    t_ = 0.0f;
    hasHit_.clear();
    if (!startSoundID_.empty())
        AudioManager::get()->playSound(startSoundID_);
}

void FighterAttack::finish()
{
    /* Empty */
}

glm::vec2 FighterAttack::getKnockback(const GameEntity *victim) const
{
    if (knockbackdir_ == glm::vec2(0,0))
    {
        glm::vec2 apos = glm::vec2(getHitbox().x, getHitbox().y);
        glm::vec2 fpos = glm::vec2(victim->getPosition().x, victim->getPosition().y);
        glm::vec2 dir = glm::normalize(fpos - apos);
        return knockbackpow_ * dir;
    }
    else
        return knockbackdir_ * knockbackpow_ * glm::vec2(owner_->getDirection(), 1.f);

}

Rectangle FighterAttack::getHitbox() const
{
    Rectangle ret;
    ret.x = hboffset_.x * owner_->getDirection() + owner_->getPosition().x;
    ret.y = hboffset_.y + owner_->getPosition().y;
    ret.w = hbsize_.x;
    ret.h = hbsize_.y;

    return ret;
}

std::string FighterAttack::getFrameName() const { return frameName_; }

void FighterAttack::setTwinkle(bool twinkle) { twinkle_ = twinkle; }
void FighterAttack::setHitboxFrame(const std::string &frame) { hbframe_ = frame; }
void FighterAttack::setStartSound(const std::string &soundID) { startSoundID_ = soundID; }

bool FighterAttack::hasTwinkle() const
{
    return (t_ < startup_) && twinkle_;
}

bool FighterAttack::hasHitbox() const
{
    return (t_ > startup_) && (t_ < startup_ + duration_);
}

bool FighterAttack::isDone() const
{
    return (t_ > startup_ + duration_ + cooldown_);
}

void FighterAttack::update(float dt)
{
    t_ += dt;
}

void FighterAttack::render(float dt)
{
    // Draw the hitbox if we should
    if (hasHitbox())
    {
        Rectangle hitbox = getHitbox();
        glm::mat4 attacktrans =
                glm::translate(glm::mat4(1.0f), glm::vec3(hitbox.x, hitbox.y, 0));

        glm::vec4 color = glm::vec4(1,0,0,0.33);
        if (hbframe_.empty() || (getParam("debug.drawHitbox") != 0.f))
        {
            renderRectangle(glm::scale(attacktrans, glm::vec3(hitbox.w, hitbox.h, 1.f)), color);
        }
        if (!hbframe_.empty())
        {
            color = glm::vec4(owner_->getColor(), 0.33f);
            FrameManager::get()->renderFrame(glm::scale(attacktrans, glm::vec3(owner_->getDirection(), 1.f, 1.f)),
                    color, hbframe_);
        }

    }
    // Draw twinkle if applicable
    if (twinkle_ && t_ < startup_)
    {
        Rectangle rect = owner_->getRect();
        float fact = 0.5 + (t_ / startup_) * 1.5;
        glm::mat4 transform =
            glm::scale(
                    glm::rotate(
                        glm::translate(glm::mat4(1.0f),
                            glm::vec3(rect.x - owner_->getDirection() * rect.w/3, rect.y, 0.f)),
                        90 * fact,
                        glm::vec3(0,0,1)),
                    glm::vec3(fact, fact, 1.f));

        FrameManager::get()->renderFrame(transform, glm::vec4(0.6f, 0.6f, 0.8f, 0.3f),
                "StrongAttackInd");
    }
}

void FighterAttack::cancel()
{
    t_ = std::max(t_, startup_ + duration_);
}

void FighterAttack::hit(GameEntity *other)
{
    assert(hasHit_.find(other->getID()) == hasHit_.end());
    hasHit_.insert(other->getID());
}

void FighterAttack::attackCollision(const Attack *other)
{
    // Only cancel if we lose or tie priority
    if (priority_ <= other->getPriority())
        cancel();
}

// ----------------------------------------------------------------------------
// MovingAttack class methods
// ----------------------------------------------------------------------------

MovingAttack::MovingAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName), started_(false)
{
    /* Empty */
}

FighterAttack* MovingAttack::clone() const
{
    return new MovingAttack(*this);
}

void MovingAttack::update(float dt)
{
    FighterAttack::update(dt);
    // Update during duration
    if (t_ > startup_ && t_ < startup_ + duration_)
    {
        if (!started_)
        {
            owner_->vel_.x = owner_->dir_ * getParam(paramPrefix_ + "xvel");
            owner_->vel_.y = getParam(paramPrefix_ + "yvel");
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    owner_->pos_.x - owner_->size_.x * owner_->dir_ * 0.1f, 
                    owner_->pos_.y - owner_->size_.y * 0.45f,
                    0.3f);
            ExplosionManager::get()->addPuff(
                    owner_->pos_.x - owner_->size_.x * owner_->dir_ * 0.3f, 
                    owner_->pos_.y - owner_->size_.y * 0.45f,
                    0.3f);

            started_ = true;
        }

    }
    if (t_ > startup_ + duration_ && started_)
    {
        owner_->vel_ = glm::vec2(0,0); 
        started_ = false;
    }
}

void MovingAttack::start()
{
    FighterAttack::start();
    started_ = false;
}

void MovingAttack::finish()
{
    FighterAttack::finish();
}


void MovingAttack::hit(GameEntity *victim)
{
    // Can't hit ourself
    if (victim->getPlayerID() == getPlayerID()) return;

    FighterAttack::hit(victim);
}

// ----------------------------------------------------------------------------
// UpSpecialAttack class methods
// ----------------------------------------------------------------------------

UpSpecialAttack::UpSpecialAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    MovingAttack(paramPrefix, audioID, frameName)
{
    /* Empty */
}

FighterAttack* UpSpecialAttack::clone() const
{
    return new UpSpecialAttack(*this);
}

void UpSpecialAttack::update(float dt)
{
    FighterAttack::update(dt);
    // Update during duration
    if (t_ > startup_ && t_ < startup_ + duration_)
    {
        if (!started_)
        {
            owner_->vel_.x = owner_->dir_ * getParam(paramPrefix_ + "xvel");
            owner_->vel_.y = getParam(paramPrefix_ + "yvel");
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    owner_->pos_.x - owner_->size_.x * owner_->dir_ * 0.1f, 
                    owner_->pos_.y - owner_->size_.y * 0.45f,
                    0.3f);
            ExplosionManager::get()->addPuff(
                    owner_->pos_.x - owner_->size_.x * owner_->dir_ * 0.3f, 
                    owner_->pos_.y - owner_->size_.y * 0.45f,
                    0.3f);
            // Pop them up a couple pixels, and be sure they're in the air state.
            owner_->push(glm::vec2(0, 2));
            delete owner_->state_;
            owner_->state_ = new AirNormalState(owner_);

            started_ = true;
        }

       repeatTime_ += dt;
    }
    if (repeatTime_ > getParam(paramPrefix_ + "repeatInterval"))
    {
        hasHit_.clear();
        repeatTime_ -= getParam(paramPrefix_ + "repeatInterval");
    }
}

void UpSpecialAttack::start()
{
    FighterAttack::start();
    started_ = false;
    repeatTime_ = 0.0f;
}

void UpSpecialAttack::finish()
{
    FighterAttack::finish();

    // When Up Special is over, dump them in air stunned forever
    delete owner_->state_;
    owner_->state_ = new AirStunnedState(owner_, HUGE_VAL);
}


void UpSpecialAttack::hit(GameEntity *victim)
{
    // Can't hit ourself
    if (victim->getPlayerID() == getPlayerID()) return;

    FighterAttack::hit(victim);

    // XXX this can create problems...
    victim->push(glm::vec2(0, 20));
}

// ----------------------------------------------------------------------------
// DashAttack class methods
// ----------------------------------------------------------------------------

DashAttack::DashAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName)
{
    std::string pp = paramPrefix + '.';
    deceleration_ = getParam(pp + "deceleration");
    initialSpeed_ = getParam(pp + "initialSpeed");
}

FighterAttack * DashAttack::clone() const
{
    return new DashAttack(*this);
}

void DashAttack::start()
{
    FighterAttack::start();

    owner_->vel_.x = initialSpeed_ * owner_->dir_;
}

void DashAttack::finish()
{
    FighterAttack::finish();
    owner_->vel_.x = 0.f;
    owner_->accel_ = glm::vec2(0.f);
}

void DashAttack::update(float dt)
{
    FighterAttack::update(dt);

    // Deccelerate during duration
    if (t_ > startup_ && t_ < startup_ + duration_)
        owner_->accel_ = glm::vec2(deceleration_, 0.f) * owner_->dir_;
}

// ----------------------------------------------------------------------------
// MovingHitboxAttack class methods
// ----------------------------------------------------------------------------

MovingHitboxAttack::MovingHitboxAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName)
{
    std::string pp = paramPrefix + '.';
    hb0 = glm::vec2(getParam(pp + "hitboxx"), getParam(pp + "hitboxy"));
    hb1 = glm::vec2(getParam(pp + "hitboxx1"), getParam(pp + "hitboxy1"));
}

FighterAttack *MovingHitboxAttack::clone() const
{
    return new MovingHitboxAttack(*this);
}

Rectangle MovingHitboxAttack::getHitbox() const
{
    float u = std::min(duration_, std::max(t_ - startup_, 0.f)) / duration_;

    glm::vec2 pos = (1 - u) * hb0 + u * hb1;

    Rectangle ret;
    ret.x = pos.x * owner_->getDirection() + owner_->getRect().x;
    ret.y = pos.y + owner_->getRect().y;
    ret.w = hbsize_.x; ret.h = hbsize_.y;

    return ret;
}

// ----------------------------------------------------------------------------
// NeutralSpecialAttack class methods
// ----------------------------------------------------------------------------

NeutralSpecialAttack::NeutralSpecialAttack(const std::string &paramPrefix,
        const std::string &frameName) :
    FighterAttack(),
    paramPrefix_(paramPrefix),
    released_(false)
{
    std::string pp = paramPrefix + '.';
    startup_ = getParam(pp + "startup");
    // No duration
    cooldown_ = getParam(pp + "startup");
    duration_ = 0.f;

    frameName_ = frameName;
}

FighterAttack * NeutralSpecialAttack::clone() const
{
    return new NeutralSpecialAttack(*this);
}

bool NeutralSpecialAttack::hasHitbox() const
{
    // Never has a hitbox - that's the projectile
    return false;
}

void NeutralSpecialAttack::update(float dt)
{
    FighterAttack::update(dt);
    if (t_ > startup_ && !released_)
    {
        std::string pp = paramPrefix_ + '.';
        Projectile *projectile =
            new Projectile(owner_->getPosition(),
                    glm::vec2(owner_->getDirection(), 0.f), paramPrefix_,
                    "Projectile", "projectilehit", owner_->getPlayerID());
        addEntity(projectile);

        released_ = true;
    }
}

void NeutralSpecialAttack::start()
{
    released_ = false;
    FighterAttack::start();
}

