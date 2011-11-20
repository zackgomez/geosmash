#include "Attack.h"
#include "ParamReader.h"
#include "Fighter.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include "explosion.h"
#include "FrameManager.h"
#include "audio.h"


// ----------------------------------------------------------------------------
// Attack class methods
// ----------------------------------------------------------------------------

Attack::Attack(const std::string &paramPrefix, const std::string &audioID,
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

    twinkle_ = false;

    knockbackdir_ = knockbackdir_ == glm::vec2(0, 0) ? knockbackdir_ : glm::normalize(knockbackdir_);
}

Attack* Attack::clone() const
{
    return new Attack(*this);
}

void Attack::setFighter(Fighter *fighter)
{
    owner_ = fighter;
}

void Attack::start()
{
    assert(owner_);
    t_ = 0.0f;
    hasHit_[0] = hasHit_[1] = hasHit_[2] = hasHit_[3] = false;
}

void Attack::finish()
{
    /* Empty */
}

glm::vec2 Attack::getKnockback(const Fighter *fighter) const
{
    if (knockbackdir_ == glm::vec2(0,0))
    {
        glm::vec2 apos = glm::vec2(getHitbox().x, getHitbox().y);
        glm::vec2 fpos = glm::vec2(fighter->getRectangle().x, fighter->getRectangle().y);
        glm::vec2 dir = glm::normalize(fpos - apos);
        std::cout << "fpos: " << fpos.x << ' ' << fpos.y << "   apos: " << apos.x << ' ' << apos.y << '\n';
        return glm::vec2(owner_->getDirection(), 1.f) * knockbackpow_ * dir;
    }
    else
        return knockbackdir_ * knockbackpow_;

}

Rectangle Attack::getHitbox() const
{
    Rectangle ret;
    ret.x = hboffset_.x * owner_->getDirection() + owner_->getRectangle().x;
    ret.y = hboffset_.y + owner_->getRectangle().y;
    ret.w = hbsize_.x;
    ret.h = hbsize_.y;

    return ret;
}

float Attack::getDamage(const Fighter *) const { return damage_; }
float Attack::getStun(const Fighter *) const { return stun_; }
std::string Attack::getAudioID() const { return audioID_; }
std::string Attack::getFrameName() const { return frameName_; }

void Attack::setTwinkle(bool twinkle) { twinkle_ = twinkle; }
void Attack::setHitboxFrame(const std::string &frame) { hbframe_ = frame; }

bool Attack::hasTwinkle() const
{
    return (t_ < startup_) && twinkle_;
}

bool Attack::hasHitbox() const
{
    return (t_ > startup_) && (t_ < startup_ + duration_);
}

bool Attack::isDone() const
{
    return (t_ > startup_ + duration_ + cooldown_);
}

bool Attack::canHit(const Fighter *f) const
{
    return !hasHit_[f->getID()];
}

void Attack::update(float dt)
{
    t_ += dt;
}

void Attack::render(float dt)
{
    // Draw the hitbox if we should
    if (hasHitbox())
    {
        Rectangle hitbox = getHitbox();
        glm::mat4 attacktrans =
                glm::translate(glm::mat4(1.0f), glm::vec3(hitbox.x, hitbox.y, 0));

        glm::vec4 color = glm::vec4(1,0,0,0.33);
        if (hbframe_.empty())
            renderRectangle(glm::scale(attacktrans, glm::vec3(hitbox.w, hitbox.h, 1.f)), color);
        else
            FrameManager::get()->renderFrame(glm::scale(attacktrans, glm::vec3(owner_->getDirection(), 1.f, 1.f)),
                    glm::vec4(owner_->getColor(), 0.33f), hbframe_);

    }
    // Draw twinkle if applicable
    if (twinkle_ && t_ < startup_)
    {
        Rectangle rect = owner_->getRectangle();
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

void Attack::cancel()
{
    t_ = std::max(t_, startup_ + duration_);
}

void Attack::hit(Fighter *other)
{
    assert(!hasHit_[other->getID()]);
    hasHit_[other->getID()] = true;
}

void Attack::attackCollision(const Attack *other)
{
    // Only cancel if we lose or tie priority
    if (priority_ <= other->priority_)
        cancel();
}

// ----------------------------------------------------------------------------
// UpSpecialAttack class methods
// ----------------------------------------------------------------------------

UpSpecialAttack::UpSpecialAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    Attack(paramPrefix, audioID, frameName)
{
    /* Empty */
}

Attack* UpSpecialAttack::clone() const
{
    return new UpSpecialAttack(*this);
}

void UpSpecialAttack::update(float dt)
{
    Attack::update(dt);
    // Update during duration
    if (t_ > startup_ && t_ < startup_ + duration_)
    {
        if (!started_)
        {
            // Move slightly up to avoid the ground, if applicable
            owner_->rect_.y += 2;
            owner_->xvel_ = owner_->dir_ * getParam("upSpecialAttack.xvel");
            owner_->yvel_ = getParam("upSpecialAttack.yvel");
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    owner_->rect_.x - owner_->rect_.w * owner_->dir_ * 0.1f, 
                    owner_->rect_.y - owner_->rect_.h * 0.45f,
                    0.3f);
            ExplosionManager::get()->addPuff(
                    owner_->rect_.x - owner_->rect_.w * owner_->dir_ * 0.3f, 
                    owner_->rect_.y - owner_->rect_.h * 0.45f,
                    0.3f);
            // Play the UP Special sound
            AudioManager::get()->playSound("upspecial001-loud");

            // Go to air normal state
            delete owner_->state_;
            owner_->state_ = new AirNormalState(owner_);
            started_ = true;
        }

        repeatTime_ += dt;
    }
    if (repeatTime_ > getParam("upSpecialAttack.repeatInterval"))
    {
        hasHit_[0] = hasHit_[1] = hasHit_[2] = hasHit_[3] = false;
        repeatTime_ -= getParam("upSpecialAttack.repeatInterval");
    }
}

void UpSpecialAttack::start()
{
    Attack::start();
    started_ = false;
    repeatTime_ = 0.0f;
}

void UpSpecialAttack::finish()
{
    Attack::finish();

    // When Up Special is over, dump them in air stunned forever
    delete owner_->state_;
    owner_->state_ = new AirStunnedState(owner_, HUGE_VAL);
}
 void UpSpecialAttack::hit(Fighter *victim)
{
    Attack::hit(victim);

    victim->rect_.y += 20;
}

// ----------------------------------------------------------------------------
// DashAttack class methods
// ----------------------------------------------------------------------------

DashAttack::DashAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    Attack(paramPrefix, audioID, frameName)
{
    std::string pp = paramPrefix + '.';
    deceleration_ = getParam(pp + "deceleration");
    initialSpeed_ = getParam(pp + "initialSpeed");
}

Attack * DashAttack::clone() const
{
    return new DashAttack(*this);
}

void DashAttack::start()
{
    Attack::start();

    owner_->xvel_ = initialSpeed_ * owner_->dir_;
}

void DashAttack::finish()
{
    Attack::finish();
    owner_->xvel_ = 0.f;
}

void DashAttack::update(float dt)
{
    Attack::update(dt);

    // Deccelerate during duration
    if (t_ > startup_ && t_ < startup_ + duration_)
        owner_->xvel_ += deceleration_ * owner_->dir_ * dt;
}

// ----------------------------------------------------------------------------
// MovingAttack class methods
// ----------------------------------------------------------------------------

MovingAttack::MovingAttack(const std::string &paramPrefix, const std::string &audioID,
        const std::string &frameName) :
    Attack(paramPrefix, audioID, frameName)
{
    std::string pp = paramPrefix + '.';
    hb0 = glm::vec2(getParam(pp + "hitboxx"), getParam(pp + "hitboxy"));
    hb1 = glm::vec2(getParam(pp + "hitboxx1"), getParam(pp + "hitboxy1"));
}

Attack *MovingAttack::clone() const
{
    return new MovingAttack(*this);
}

Rectangle MovingAttack::getHitbox() const
{
    float u = std::min(duration_, std::max(t_ - startup_, 0.f)) / duration_;
    std::cout << "yay: " << u << '\n';

    glm::vec2 pos = (1 - u) * hb0 + u * hb1;

    Rectangle ret;
    ret.x = pos.x * owner_->getDirection() + owner_->getRectangle().x;
    ret.y = pos.y + owner_->getRectangle().y;
    ret.w = hbsize_.x; ret.h = hbsize_.y;

    return ret;
}
