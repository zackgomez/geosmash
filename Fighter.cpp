#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "ParamReader.h"

const static int AIR_NORMAL_STATE = 0;
const static int AIR_STUNNED_STATE = 1;
const static int GROUND_STATE = 2;
const static int DEAD_STATE = 3;

static sf::Music *koSound = NULL;

Fighter::Fighter(const ParamReader &params, float respawnx, float respawny, const glm::vec3& color) :
    rect_(Rectangle(0, 0, params.get("fighter.w"), params.get("fighter.h"))),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(AIR_NORMAL_STATE),
    damage_(0), lives_(params.get("fighter.lives")),
    respawnx_(respawnx), respawny_(respawny),
    color_(color),
    attack_(NULL),
    walkSpeed_(params.get("walkSpeed")),
    dashSpeed_(params.get("dashSpeed")),
    airForce_(params.get("airForce")),
    airAccel_(params.get("airAccel")),
    jumpStartupTime_(params.get("jumpStartupTime")),
    jumpSpeed_(params.get("jumpSpeed")),
    hopSpeed_(params.get("hopSpeed")),
    jumpAirSpeed_(params.get("jumpAirSpeed")),
    secondJumpSpeed_(params.get("secondJumpSpeed")),
    dashStartupTime_(params.get("dashStartupTime")),
    inputVelocityThreshold_(params.get("input.velThresh")),
    inputJumpThresh_(params.get("input.jumpThresh")),
    inputDashThresh_(params.get("input.dashThresh")),
    inputDashMin_(params.get("input.dashMin")),
    inputDeadzone_(params.get("input.deadzone"))
{
    // Load ground attacks
    dashAttack_ = loadAttack(params, "dashAttack", "sfx/neutral001.wav");
    neutralTiltAttack_ = loadAttack(params, "neutralTiltAttack", "sfx/neutral001.wav");
    sideTiltAttack_ = loadAttack(params, "sideTiltAttack", "sfx/forwardtilt001.wav");
    downTiltAttack_ = loadAttack(params, "downTiltAttack", "sfx/downtilt001.wav");
    upTiltAttack_ = loadAttack(params, "upTiltAttack", "sfx/uptilt001.wav");

    // Load air attack special as it uses a different class
    airAttack_ = AirAttack(
            params.get("airAttack.startup"),
            params.get("airAttack.duration"),
            params.get("airAttack.cooldown"),
            params.get("airAttack.damage"),
            params.get("airAttack.stun"),
            params.get("airAttack.knockbackpow"),
            Rectangle(
                0.5f * rect_.w + params.get("airAttack.hitboxx"),
                params.get("airAttack.hitboxy"),
                params.get("airAttack.hitboxw"),
                params.get("airAttack.hitboxh")));
    sf::Music *m = new sf::Music();
    m->OpenFromFile("sfx/airneutral001.wav");
    airAttack_.setSound(m);

    // Load some audio
    if (!koSound)
    {
        koSound = new sf::Music();
        koSound->OpenFromFile("sfx/ko001.wav");
    }
}

Fighter::~Fighter()
{}

int Fighter::getLives() const
{
    return lives_;
}

float Fighter::getDamage() const 
{
    return damage_;
}

float Fighter::getDirection() const
{
    return dir_;
}


void Fighter::update(const struct Controller &controller, float dt)
{
    if (state_ == DEAD_STATE)
        return;

    // Update
    if (state_ == GROUND_STATE)
    {
        dashTime_ -= dt;
        if (!attack_ && dashTime_ < 0)
        {
            // Dashing direction update
            if (dashing_)
            {
                int newdir = controller.joyx < 0 ? -1 : 1;
                if (dir_ != newdir)
                {
                    dir_ = newdir;
                    dashTime_ = dashStartupTime_;
                    xvel_ = 0;
                    // Draw a little puff
                    ExplosionManager::get()->addPuff(
                            rect_.x - rect_.w * dir_ * 0.4f, 
                            rect_.y - rect_.h * 0.45f,
                            0.3f);
                }
                else if (fabs(controller.joyx) < inputDashMin_ && fabs(controller.joyxv) < inputVelocityThreshold_)
                {
                    dashing_ = false;
                    dashTime_ = dashStartupTime_;
                }
                else
                    xvel_ = dir_ * dashSpeed_;
            }
            else // !dashing_
            {
                // Check for entry into dash
                if (!dashing_ && fabs(controller.joyx) > inputDashThresh_ && fabs(controller.joyxv) > inputVelocityThreshold_)
                {
                    dashing_ = true;
                    dashTime_ = dashStartupTime_;
                    xvel_ = 0;
                    dir_ = controller.joyx < 0 ? -1 : 1;
                    // Draw a little puff
                    ExplosionManager::get()->addPuff(
                            rect_.x - rect_.w * dir_ * 0.4f, 
                            rect_.y - rect_.h * 0.45f,
                            0.3f);
                }
                xvel_ = 0;
                // Check for walking
                if (!dashing_ && fabs(controller.joyx) > inputDeadzone_ && !attack_)
                {
                    xvel_ = controller.joyx * walkSpeed_;
                    dir_ = xvel_ < 0 ? -1 : 1;
                }
            }
            // Check for jump
            if (jumpTime_ < 0 && 
                    (controller.pressjump || (controller.joyy > inputJumpThresh_ && controller.joyyv > inputVelocityThreshold_)))
            {
                // Start the jump timer
                jumpTime_ = 0.0f;
            }
            if (jumpTime_ > jumpStartupTime_)
            {
                // If they are still "holding down" the jump button now, then full jump
                // otherwise short hop
                state_ = AIR_NORMAL_STATE;
                jumpTime_ = -1;
                // also let this character double jump if they want
                canSecondJump_ = true;
                xvel_ = fabs(controller.joyx) > inputDeadzone_ ?
                    controller.joyx * 0.5 * dashSpeed_ :
                    0.0f;
                if (controller.jumpbutton || controller.joyy > inputJumpThresh_)
                    yvel_ = jumpSpeed_;
                else
                    yvel_ = hopSpeed_;
            }
            // Check for attack
            if (dashing_ && controller.pressa)
            {
                // Do the dash attack
                dashing_ = false;
                xvel_ = dir_ * dashSpeed_;
                attack_ = new Attack(dashAttack_);
                attack_->setFighter(this);
            }
            else if (!dashing_ && controller.pressa)
            {
                // No movement during attack
                xvel_ = 0; yvel_ = 0;
                // Get direction of stick
                glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
                if (fabs(controller.joyx) > 0.4 && fabs(tiltDir.x) > fabs(tiltDir.y))
                {
                    // Do the L/R tilt
                    dir_ = controller.joyx > 0 ? 1 : -1;
                    attack_ = new Attack(sideTiltAttack_);
                }
                else if (controller.joyy < -0.4 && fabs(tiltDir.x) < fabs(tiltDir.y))
                {
                    attack_ = new Attack(downTiltAttack_);
                }
                else if (controller.joyy > 0.4 && fabs(tiltDir.x) < fabs(tiltDir.y))
                {
                    attack_ = new Attack(upTiltAttack_);
                }
                else
                {
                    // Neutral tilt attack
                    attack_ = new Attack(neutralTiltAttack_);
                }
                // Set the owner of the attack
                attack_->setFighter(this);
            }
        }
    }
    if (state_ == AIR_NORMAL_STATE)
    {
        // update for air normal state
        if (fabs(controller.joyx) > 0.2f && !attack_)
        {
            // Don't let the player increase the velocity past a certain speed
            if (xvel_ * controller.joyx <= 0 || fabs(xvel_) < jumpAirSpeed_)
                xvel_ += controller.joyx * airForce_ * dt;
            // You can always control your orientation
            dir_ = controller.joyx < 0 ? -1 : 1;
        }

        // second jump, if the character wants to.
        if ((controller.pressjump || (controller.joyy > 0.65 && 
                    controller.joyyv > 0.20f)) && canSecondJump_ && !attack_)
        {
            canSecondJump_ = false;
            jumpTime_ = 0;
        }
        if (jumpTime_ > jumpStartupTime_) 
        {
            yvel_ = secondJumpSpeed_;
            xvel_ = fabs(controller.joyx) > 0.2f ? 
                        dashSpeed_ * std::max(-1.0f, std::min(1.0f, (controller.joyx - 0.2f) / 0.6f)) :
                        0.0f;
            jumpTime_ = -1;
        }
        // Check for attack
        if (controller.pressa && !attack_)
        {
            // Start new AIR attack
            attack_ = new AirAttack(airAttack_);
            attack_->setFighter(this);
        }
        
        // gravity update (separate from controller force)
        yvel_ += airAccel_ * dt;


    }
    if (state_ == AIR_STUNNED_STATE)
    {
        yvel_ += airAccel_ * dt;
        stunTime_ += dt;
        if (stunTime_ > stunDuration_)
        {
            state_ = AIR_NORMAL_STATE;
            jumpTime_ = -1;
            canSecondJump_ = true;
        }
    }
    if (attack_)
    {
        attack_->update(dt);
        if (attack_->isDone())
        {
            delete attack_;
            attack_ = NULL;
        }
    }
    if (jumpTime_ >= 0)
        jumpTime_ += dt;

    // Update position
    rect_.x += xvel_ * dt;
    rect_.y += yvel_ * dt;
}

void Fighter::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (collision && ground.y + ground.h/2 < rect_.y + rect_.h/2)
    {
        if (state_ != GROUND_STATE)
        {
            state_ = GROUND_STATE;
            xvel_ = yvel_ = 0;
            jumpTime_ = -1;
            dashing_ = false;
            dashTime_ = -1;
            if (attack_)
                attack_->cancel();
        }
        // Make sure we're barely overlapping the ground (by 1 unit)
        rect_.y = ground.y + ground.h / 2 + rect_.h/2 - 1;
    }
    // If no ground collision and we were on the ground, now in the air
    else
    {
        if (state_ == GROUND_STATE)
        {
            state_ = AIR_NORMAL_STATE;
            jumpTime_ = -1;
            canSecondJump_ = true;
        }
    }
}

void Fighter::attackCollision()
{
    // If two attacks collide, just cancel them and go to cooldown
    assert(attack_);
    attack_->cancel();
}

void Fighter::hitByAttack(const Fighter *fighter, const Attack *inAttack)
{
    assert(inAttack);
    assert(fighter);
    // Pop up a tiny amount if they're on the ground
    if (state_ == GROUND_STATE)
        rect_.y += 2;
    // Take damage
    damage_ += inAttack->getDamage(this);
    // Go to the stunned state
    state_ = AIR_STUNNED_STATE;
    stunTime_ = 0;
    stunDuration_ = inAttack->getStun(this) * damageFunc();;

    // Calculate direction of hit
    glm::vec2 knockback = inAttack->getKnockback(this) * glm::vec2(fighter->dir_, 1.0f)
        * damageFunc();


    // Get knocked back
    xvel_ = knockback.x;
    yvel_ = knockback.y;

    // Generate a tiny explosion here
    glm::vec2 hitdir = glm::vec2(rect_.x, rect_.y)
        - glm::vec2(inAttack->getHitbox().x, inAttack->getHitbox().y);
    hitdir = glm::normalize(hitdir);
    float exx = -hitdir.x * rect_.w / 2 + rect_.x;
    float exy = -hitdir.y * rect_.h / 2 + rect_.y;
    ExplosionManager::get()->addExplosion(exx, exy, 0.2);
}

void Fighter::hitWithAttack()
{
    assert(attack_);
    attack_->hit();
}

const Rectangle& Fighter::getRectangle() const
{
    return rect_;
}

bool Fighter::hasAttack() const
{
    return attack_ && attack_->hasHitbox();
}

const Attack * Fighter::getAttack() const
{
    return attack_;
}

void Fighter::respawn(bool killed)
{
    rect_.x = respawnx_;
    rect_.y = respawny_;
    xvel_ = yvel_ = 0.0f;
    state_ = AIR_NORMAL_STATE;
    canSecondJump_ = true;
    jumpTime_ = -1;
    damage_ = 0;
    if (attack_)
    {
        delete attack_;
        attack_ = 0;
    }
    // Check for death
    if (killed)
    {
        --lives_;
        koSound->Play();
    }
    if (lives_ <= 0)
    {
        // Move them way off the map
        rect_.x = HUGE_VAL;
        rect_.y = HUGE_VAL;
        state_ = DEAD_STATE;
    }
}

bool Fighter::isAlive() const
{
    return state_ != DEAD_STATE;
}

void Fighter::render(float dt)
{
    printf("State: %d  Damage: %f  Position: [%f, %f]   Velocity: [%f, %f]  Dashing: %d  Attack: %d  Dir: %f\n", 
            state_, damage_, rect_.x, rect_.y, xvel_, yvel_, dashing_, attack_ != 0, dir_);

    if (state_ == DEAD_STATE)
        return;

    glm::vec3 color = color_;
    if (state_ == AIR_STUNNED_STATE)
	{
        // flash the player when in the stunned state...
		float opacity_factor;
		float period_scale_factor = 20.0;
		float opacity_amplitude = 3;
		opacity_factor = (1 + cos(period_scale_factor * stunTime_)) * 0.5f; 
        color *= opacity_amplitude * opacity_factor + 1;
	}
    // Draw body
    glm::mat4 transform(1.0);
    transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(rect_.w, rect_.h, 1.0));
    renderRectangle(transform, color);

    // Draw orientation tick
    float angle = 0;
    glm::mat4 ticktrans = glm::scale(
            glm::rotate(
                glm::translate(transform, glm::vec3(0.5 * dir_, 0.0, 0.0)),
                angle, glm::vec3(0.0, 0.0, -1.0)),
            glm::vec3(0.33, 0.1, 1.0));
    renderRectangle(ticktrans, color);

    // Draw hitbox if applicable
    if (attack_ && attack_->drawHitbox())
    {
        Rectangle hitbox = attack_->getHitbox();
        glm::mat4 attacktrans = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(hitbox.x, hitbox.y, 0)),
                glm::vec3(hitbox.w, hitbox.h, 1.0f));
        renderRectangle(attacktrans, glm::vec3(1,0,0));
    }
}

float Fighter::damageFunc() const
{
    return 2 * damage_ / 33;
}

Attack Fighter::loadAttack(const ParamReader &params, std::string attackName,
        std::string soundFile)
{
    attackName += '.';

    Attack ret(
            params.get(attackName + "startup"),
            params.get(attackName + "duration"),
            params.get(attackName + "cooldown"),
            params.get(attackName + "damage"),
            params.get(attackName + "stun"),
            params.get(attackName + "knockbackpow") * glm::normalize(glm::vec2(
                    params.get(attackName + "knockbackx"),
                    params.get(attackName + "knockbacky"))),
            Rectangle(
                0.5f * rect_.w + params.get(attackName + "hitboxx"),
                params.get(attackName + "hitboxy"),
                params.get(attackName + "hitboxw"),
                params.get(attackName + "hitboxh")));

    if (!soundFile.empty())
    {
        sf::Music *m = new sf::Music();
        m->OpenFromFile(soundFile);
        ret.setSound(m);
    }

    return ret;

}


Rectangle::Rectangle() :
    x(0), y(0), w(0), h(0)
{}

Rectangle::Rectangle(float xin, float yin, float win, float hin) :
    x(xin), y(yin), w(win), h(hin)
{}

bool Rectangle::overlaps(const Rectangle &rhs) const
{
    return (rhs.x + rhs.w/2) > (x - w/2) && (rhs.x - rhs.w/2) < (x + w/2) &&
        (rhs.y + rhs.h/2) > (y - h/2) && (rhs.y - rhs.h/2) < (y + h/2);
}


void Attack::playSound() 
{
    if (sound_)
        sound_->Play();
}

void Attack::setSound(sf::Music *m) 
{
    sound_ = m;
}

void Attack::setFighter(const Fighter *fighter)
{
    owner_ = fighter;
}

Rectangle Attack::getHitbox() const
{
    Rectangle ret;
    ret.x = hitbox_.x * owner_->getDirection() + owner_->getRectangle().x;
    ret.y = hitbox_.y + owner_->getRectangle().y;
    ret.h = hitbox_.h;
    ret.w = hitbox_.w;
    return ret;
}

bool Attack::hasHitbox() const
{
    return (t_ > startup_) && (t_ < startup_ + duration_) && !hasHit_;
}

bool Attack::drawHitbox() const
{
    return (t_ > startup_) && (t_ < startup_ + duration_);
}

bool Attack::isDone() const
{
    return (t_ > startup_ + duration_ + cooldown_);
}

void Attack::update(float dt)
{
    t_ += dt;
}

void Attack::cancel()
{
    t_ = startup_ + duration_;
}

void Attack::hit()
{
    hasHit_ = true;
    playSound();
}

glm::vec2 AirAttack::getKnockback(const Fighter *fighter) const
{
    // Calculate direction of knockback
    glm::vec2 dir = glm::normalize(glm::vec2(fighter->getRectangle().x - getHitbox().x,
                fighter->getRectangle().y - getHitbox().y));

    glm::vec2 kb = dir * power_;
    return kb;
}

