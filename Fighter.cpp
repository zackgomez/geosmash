#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"

const static int AIR_NORMAL_STATE = 0;
const static int AIR_STUNNED_STATE = 1;
const static int GROUND_STATE = 2;
const static int DEAD_STATE = 3;

Fighter::Fighter(const Rectangle &rect, float respawnx, float respawny, const glm::vec3& color) :
    rect_(rect),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(AIR_NORMAL_STATE),
    damage_(0), lives_(4),
    respawnx_(respawnx), respawny_(respawny),
    color_(color),
    attack_(NULL),
    walkSpeed_(133.3), dashSpeed_(400.0), airForce_(450.0), airAccel_(-1100.0),
    jumpStartupTime_(0.05), jumpSpeed_(600.0), hopSpeed_(200.0),
    jumpAirSpeed_(200.0), secondJumpSpeed_(500.0), dashStartupTime_(0.08)
{
    // XXX HOLY SHIT THIS SUCKS
    neutralAttack_ = Attack(0.05, 0.15, 0.1, 5, 0.07,
            40.0f * glm::normalize(glm::vec2(1, 1)),
            Rectangle(0.75f * rect_.w, 0.0f, 0.5f * rect_.w, 0.25f * rect_.h));
    /*
    attackStartup_(0.1), attackDuration_(0.3), attackCooldown_(0.2),
    attackDamage_(5), attackKnockback_(80.0), attackStun_(0.07),
    attackX_(0.5*rect.w), attackY_(-0.33*rect.h), attackW_(50), attackH_(18),
    */
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
                }
                else if (fabs(controller.joyx) < 0.6f && fabs(controller.joyxv) < 0.15f)
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
                if (!dashing_ && fabs(controller.joyx) > 0.9f && fabs(controller.joyxv) > 0.15f)
                {
                    dashing_ = true;
                    dashTime_ = dashStartupTime_;
                    xvel_ = 0;
                }
                xvel_ = 0;
                if (!dashing_ && fabs(controller.joyx) > 0.2f && !attack_)
                {
                    xvel_ = controller.joyx * walkSpeed_;
                    dir_ = xvel_ < 0 ? -1 : 1;
                }
            }
            // Check for jump
            if (jumpTime_ < 0 && 
                    (controller.pressjump || (controller.joyy > 0.65 && controller.joyyv > 0.20f)))
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
                xvel_ = fabs(controller.joyx) > 0.2f ? controller.joyx * 0.5 * dashSpeed_ : 0.0f;
                if (controller.jumpbutton || controller.joyy > 0.65f)
                    yvel_ = jumpSpeed_;
                else
                    yvel_ = hopSpeed_;
            }
            // Check for attack
            if (dashing_ && controller.pressa)
            {
                // TODO DO DASH ATTACK
            }
            else if (!dashing_ && controller.pressa)
            {
                // Do the ground neutral attack
                xvel_ = 0;
                attack_ = new Attack(neutralAttack_);
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
            // TODO Start new AIR attack
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
    if (killed) --lives_;
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


void Attack::setFighter(const Fighter *fighter)
{
    hitbox_.x = hitbox_.x * fighter->getDirection() + fighter->getRectangle().x;
    hitbox_.y += fighter->getRectangle().y;
}

Rectangle Attack::getHitbox() const
{
    return hitbox_;
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
}

