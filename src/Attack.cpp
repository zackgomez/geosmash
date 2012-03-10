#include "Attack.h"
#include "ParamReader.h"
#include "Fighter.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Engine.h"
#include "ExplosionManager.h"
#include "FrameManager.h"
#include "AudioManager.h"
#include "Projectile.h"

// XXX remove this
void addEntity(GameEntity *ent);

// ----------------------------------------------------------------------------
// SimpleAttack class methods
// ----------------------------------------------------------------------------
SimpleAttack::SimpleAttack(
        const glm::vec2 &kbdir, float kbbase, float kbscaling,
        float damage, float stun, float priority,
        const glm::vec2 &pos, const glm::vec2 &size,
        float odir,
        int playerID, int teamID, const std::string &audioID) :
    Attack(),
    playerID_(playerID), teamID_(teamID),
    damage_(damage), stun_(stun), priority_(priority),
    odir_(odir),
    pos_(pos),
    size_(size),
    kbdir_(kbdir),
    kbbase_(kbbase),
    kbscaling_(kbscaling),
    audioID_(audioID)
{
}

SimpleAttack::~SimpleAttack()
{
    /* empty */
}

rectangle SimpleAttack::getHitbox() const
{
    return rectangle(pos_.x, pos_.y, size_.x, size_.y);
}

float SimpleAttack::getPriority() const { return priority_; }
float SimpleAttack::getDamage() const { return damage_; }

float SimpleAttack::calcStun(const GameEntity *, float damage) const
{
    return stun_ * (damage * 1.2/33 + 1.5);
}

glm::vec2 SimpleAttack::calcKnockback(const GameEntity *, float damage) const
{
    return kbdir_ * (kbbase_ + damage * kbscaling_);
}

float SimpleAttack::getOriginDirection(const GameEntity *) const
{
    return odir_;
}

int SimpleAttack::getPlayerID() const
{
    return playerID_;
}

int SimpleAttack::getTeamID() const
{
    return teamID_;
}

std::string SimpleAttack::getAudioID() const
{
    return audioID_;
}

void SimpleAttack::setPosition(const glm::vec2 &position)
{
    pos_ = position;
}

void SimpleAttack::setBaseKnockback(float pow)
{
    kbbase_ = pow;
}

void SimpleAttack::setOriginDirection(float odir)
{
    odir_ = odir;
}

void SimpleAttack::setKnockbackDir(const glm::vec2 &kbdir)
{
    kbdir_ = glm::normalize(kbdir);
}

bool SimpleAttack::canHit(const GameEntity *f) const
{
    return hasHit_.find(f->getID()) == hasHit_.end();
}

void SimpleAttack::attackCollision(const Attack *other)
{
    // Do nothing
}

void SimpleAttack::hit(GameEntity *other)
{
    assert(hasHit_.find(other->getID()) == hasHit_.end());
    hasHit_.insert(other->getID());
}

void SimpleAttack::clearHit()
{
    hasHit_.clear();
}

void SimpleAttack::reown(int playerID, int teamID)
{
    playerID_ = playerID;
    teamID_ = teamID;
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
    kbdir_ = glm::normalize(glm::vec2(
            getParam(pp + "knockbackx"),
            getParam(pp + "knockbacky")));
    kbbase_ = getParam(pp + "kbbase");
    kbscaling_ = getParam(pp + "kbscaling");
    hboffset_ = glm::vec2(getParam(pp + "hitboxx"), getParam(pp + "hitboxy"));
    hbsize_ = glm::vec2(getParam(pp + "hitboxw"), getParam(pp + "hitboxh"));
    priority_ = getParam(pp + "priority");
    audioID_ = audioID;
    frameName_ = frameName;
    paramPrefix_ = pp;
    t_ = 0.0f;

    twinkle_ = false;
}

FighterAttack* FighterAttack::clone() const
{
    return new FighterAttack(*this);
}

void FighterAttack::setFighter(Fighter *fighter)
{
    owner_ = fighter;
    playerID_ = owner_->getPlayerID();
    teamID_ = owner_->getTeamID();
}

int FighterAttack::getPlayerID() const
{
    return owner_->getPlayerID();
}

void FighterAttack::start()
{
    assert(owner_);
    t_ = 0.0f;
    activeSoundPlayed_ = false;
    hasHit_.clear();
    if (!startSoundID_.empty())
        AudioManager::get()->playSound(startSoundID_);
}

void FighterAttack::finish()
{
    /* Empty */
}

float FighterAttack::getOriginDirection(const GameEntity *victim) const
{
    // Direction is based on their direction from the owner
    return victim->getPosition().x - owner_->getPosition().x > 0 ? -1 : 1;
}

rectangle FighterAttack::getHitbox() const
{
    rectangle ret;
    ret.x = hboffset_.x * owner_->getDirection() + owner_->getPosition().x;
    ret.y = hboffset_.y + owner_->getPosition().y;
    ret.w = hbsize_.x;
    ret.h = hbsize_.y;

    return ret;
}

glm::vec2 FighterAttack::calcKnockback(const GameEntity *victim, float damage) const
{
    float dir = owner_->getDirection();
    glm::vec2 kb = glm::vec2(dir, 1.f) * kbdir_;
    kb *= kbbase_ + damage * kbscaling_;
    return kb;
}

std::string FighterAttack::getFrameName() const { return frameName_; }

void FighterAttack::setTwinkle(bool twinkle) { twinkle_ = twinkle; }
void FighterAttack::setHitboxFrame(const std::string &frame) { hbframe_ = frame; }
void FighterAttack::setStartSound(const std::string &soundID) { startSoundID_ = soundID; }
void FighterAttack::setActiveSound(const std::string &soundID) { activeSoundID_ = soundID; }

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

    // Play the active sound first hitbox frame
    if (t_ > startup_ && !activeSoundPlayed_)
    {
        activeSoundPlayed_ = true;
        if (!startSoundID_.empty())
            AudioManager::get()->playSound(activeSoundID_);
    }
}

void FighterAttack::render(float dt)
{
    // Draw the hitbox if we should
    if (hasHitbox())
    {
        rectangle hitbox = getHitbox();
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
        rectangle rect = owner_->getRect();
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

void FighterAttack::kill()
{
    // Just set t to a massive value, guaranteed to be larger than total time
    t_ = HUGE_VAL;
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
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    owner_->getRect().x - owner_->getRect().w * owner_->getDirection() * 0.1f, 
                    owner_->getRect().y - owner_->getRect().h * 0.45f,
                    0.3f);
            ExplosionManager::get()->addPuff(
                    owner_->getRect().x - owner_->getRect().w * owner_->getDirection() * 0.3f, 
                    owner_->getRect().y - owner_->getRect().h * 0.45f,
                    0.3f);

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
}


void UpSpecialAttack::hit(GameEntity *victim)
{
    // Can't hit ourself
    if (victim->getPlayerID() == getPlayerID()) return;

    FighterAttack::hit(victim);
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

rectangle MovingHitboxAttack::getHitbox() const
{
    float u = std::min(duration_, std::max(t_ - startup_, 0.f)) / duration_;

    glm::vec2 pos = (1 - u) * hb0 + u * hb1;

    rectangle ret;
    ret.x = pos.x * owner_->getDirection() + owner_->getRect().x;
    ret.y = pos.y + owner_->getRect().y;
    ret.w = hbsize_.x; ret.h = hbsize_.y;

    return ret;
}

// ----------------------------------------------------------------------------
// VaryingDirectionAttack class methods
// ----------------------------------------------------------------------------
VaryingDirectionAttack::VaryingDirectionAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName)
{
}

FighterAttack *VaryingDirectionAttack::clone() const
{
    return new VaryingDirectionAttack(*this);
}

glm::vec2 VaryingDirectionAttack::calcKnockback(const GameEntity *victim,
        float damage) const
{
    glm::vec2 apos(getHitbox().x, getHitbox().y);
    glm::vec2 kbdir = glm::normalize(victim->getPosition() - apos);
    return kbdir * (kbbase_ + damage * kbscaling_);
}

// ----------------------------------------------------------------------------
// VelocityDirectionAttack class methods
// ----------------------------------------------------------------------------
VelocityDirectionAttack::VelocityDirectionAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName)
{
}

FighterAttack *VelocityDirectionAttack::clone() const
{
    return new VelocityDirectionAttack(*this);
}

glm::vec2 VelocityDirectionAttack::calcKnockback(const GameEntity *victim,
        float damage) const
{
    assert(owner_->getVelocity() != glm::vec2(0.f, 0.f));
    glm::vec2 kbdir = glm::normalize(owner_->getVelocity());
    return kbdir * (kbbase_ + damage * kbscaling_);
}

// ----------------------------------------------------------------------------
// RepeatingAttack class methods
// ----------------------------------------------------------------------------
RepeatingAttack::RepeatingAttack(const std::string &paramPrefix, const std::string &audioID,
            const std::string &frameName) :
    FighterAttack(paramPrefix, audioID, frameName)
{
}

FighterAttack * RepeatingAttack::clone() const
{
    return new RepeatingAttack(*this);
}

void RepeatingAttack::update(float dt)
{
    FighterAttack::update(dt);
    if (hasHitbox())
    {
        repeatT_ += dt;
        if (repeatT_ > getParam(paramPrefix_ + "repeatT"))
        {
            repeatT_ -= getParam(paramPrefix_ + "repeatT");
            hasHit_.clear();
        }
    }
}

