#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>

const static int AIR_NORMAL_STATE = 0;
const static int AIR_STUNNED_STATE = 1;
const static int GROUND_STATE = 2;
const static int DEAD_STATE = 3;

Fighter::Fighter(const Rectangle &rect, float respawnx, float respawny, const glm::vec3& color) :
    rect_(rect),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(AIR_NORMAL_STATE),
    stunTime_(0), stunDuration_(0),
    damage_(0), lives_(4),
    respawnx_(respawnx), respawny_(respawny),
    color_(color),
    attackTime_(-1),
    attackStartup_(0.1), attackDuration_(0.3), attackCooldown_(0.2),
    attackDamage_(5), attackKnockback_(60.0), attackStun_(0.02),
    attackX_(0.5*rect.w), attackY_(-0.33*rect.h), attackW_(50), attackH_(18),
    walkSpeed_(300.0), airForce_(450.0), airAccel_(-1100.0),
    jumpStartupTime_(0.05), jumpSpeed_(600.0), hopSpeed_(200.0),
    jumpAirSpeed_(200.0), secondJumpSpeed_(500.0)
{}

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


void Fighter::update(const struct Controller &controller, float dt)
{
    if (state_ == DEAD_STATE)
        return;

    // Update
    if (state_ == GROUND_STATE)
    {
        // Check for jump
        if (jumpTime_ < 0 && attackTime_ < 0 &&
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
            xvel_ = fabs(controller.joyx) > 0.2f ? controller.joyx * 0.5 * walkSpeed_ : 0.0f;
            if (controller.jumpbutton || controller.joyy > 0.65f)
                yvel_ = jumpSpeed_;
            else
                yvel_ = hopSpeed_;
        }
        else
        {
            // If no jump update 
            if (fabs(controller.joyx) > 0.2f && attackTime_ < 0)
            {
                xvel_ = controller.joyx * walkSpeed_;
                dir_ = xvel_ < 0 ? -1 : 1;
            }
            else if (attackTime_ < 0)
                xvel_ = 0.0f;
        }
    }
    if (state_ == AIR_NORMAL_STATE)
    {
        // update for air normal state
        if (fabs(controller.joyx) > 0.2f && attackTime_ < 0)
        {
            // Don't let the player increase the velocity past a certain speed
            if (xvel_ * controller.joyx <= 0 || fabs(xvel_) < jumpAirSpeed_)
                xvel_ += controller.joyx * airForce_ * dt;
            // You can always control your orientation
            dir_ = controller.joyx < 0 ? -1 : 1;
        }

        // second jump, if the character wants to.
        if ((controller.pressjump || (controller.joyy > 0.65 && 
                    controller.joyyv > 0.20f)) && canSecondJump_ && attackTime_ < 0) 
        {
            canSecondJump_ = false;
            jumpTime_ = 0;
        }
        if (jumpTime_ > jumpStartupTime_) 
        {
            yvel_ = secondJumpSpeed_;
            xvel_ = fabs(controller.joyx) > 0.2f ? 
                        controller.joyx  * walkSpeed_ : 
                        0.0f;
            jumpTime_ = -1;
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
    if (state_ != AIR_STUNNED_STATE)
    {
        // Check for attack
        if (controller.pressa && attackTime_ < 0)
        {
            // Start new attack
            attackTime_ = 0;
            attackHit_ = false;
        }
    }
    if (attackTime_ >= 0)
    {
        attackTime_ += dt;
        if (attackTime_ > attackStartup_ + attackDuration_ + attackCooldown_)
            attackTime_ = -1;
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
            attackTime_ = -1;
            jumpTime_ = -1;
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
    std::cout << "Attack Collision\n";
    // If two attacks collide, just cancel them and go to cooldown
    attackHit_ = true;

    // TODO Probably generate a tiny explosion here
}

void Fighter::hitByAttack(const Rectangle &hitbox)
{
    std::cout << "Hit by attack\n";
    // Pop up a tiny amount if they're on the ground
    if (state_ == GROUND_STATE)
        rect_.y += 2;
    // Take damage
    damage_ += attackDamage_;
    // Go to the stunned state
    state_ = AIR_STUNNED_STATE;
    stunTime_ = 0;
    stunDuration_ = attackStun_ * damageFunc();;
    // Calculate direction of hit
    glm::vec2 hitdir = glm::vec2(rect_.x, rect_.y) - glm::vec2(hitbox.x, hitbox.y);
    hitdir = glm::normalize(hitdir);
    hitdir *= attackKnockback_ * damageFunc();

    // Get knocked back
    xvel_ = hitdir.x;
    yvel_ = hitdir.y;

    // TODO Probably generate a tiny explosion here
}

void Fighter::hitWithAttack()
{
    attackHit_ = true;
}

const Rectangle& Fighter::getRectangle() const
{
    return rect_;
}

bool Fighter::hasAttack() const
{
    return inAttackAnimation() && !attackHit_;
}

bool Fighter::inAttackAnimation() const
{
    return attackTime_ > attackStartup_
        && attackTime_ < attackStartup_ + attackDuration_;
}

Rectangle Fighter::getAttackBox() const
{
    return Rectangle(rect_.x + dir_ * attackX_, rect_.y + attackY_,
            attackW_, attackH_);
}

void Fighter::respawn(bool killed)
{
    rect_.x = respawnx_;
    rect_.y = respawny_;
    xvel_ = yvel_ = 0.0f;
    state_ = AIR_NORMAL_STATE;
    canSecondJump_ = true;
    attackTime_ = -1;
    jumpTime_ = -1;
    damage_ = 0;
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
    printf("State: %d  Lives: %d  Damage: %f  Position: [%f, %f]   Velocity: [%f, %f] JumpTime: %f\n", 
            state_, lives_, damage_, rect_.x, rect_.y, xvel_, yvel_, jumpTime_);

    if (state_ == DEAD_STATE)
        return;

    glm::vec3 color = color_;
    if (state_ == AIR_STUNNED_STATE)
        color *= 4;
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
    if (inAttackAnimation())
    {
        Rectangle hitbox = getAttackBox();
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
