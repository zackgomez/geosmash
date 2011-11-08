#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "ParamReader.h"
#include "audio.h"

static anim_frame *frame = 0;

Fighter::Fighter(float respawnx, float respawny, const glm::vec3& color, int id) :
    rect_(Rectangle(0, 0, getParam("fighter.w"), getParam("fighter.h"))),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(0),
    damage_(0), lives_(getParam("fighter.lives")),
    respawnx_(respawnx), respawny_(respawny),
    color_(color), id_(id),
    attack_(NULL),
    lastHitBy_(-1), lastHitExpireTime_(0.0f)
{
    // Load ground attacks
    std::string g = "groundhit";
    std::string a = "airhit";
    dashAttack_ = loadAttack<Attack>("dashAttack", g);
    neutralTiltAttack_ = loadAttack<Attack>("neutralTiltAttack", g);
    sideTiltAttack_ = loadAttack<Attack>("sideTiltAttack", g);
    downTiltAttack_ = loadAttack<Attack>("downTiltAttack", g);
    upTiltAttack_ = loadAttack<Attack>("upTiltAttack", g);

    // Load air attack special as it uses a different class
    airNeutralAttack_ = loadAttack<Attack>("airNeutralAttack", a);
    airSideAttack_ = loadAttack<Attack>("airSideAttack", a);
    airDownAttack_ = loadAttack<Attack>("airDownAttack", a);
    airUpAttack_ = loadAttack<Attack>("airUpAttack", a);

    upSpecialAttack_ = loadAttack<UpSpecialAttack>("upSpecialAttack", a);

    state_ = 0;
    
    if (!frame)
        frame = loadAnimFrame("frames/ground.running.frame");
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

int Fighter::getLastHitBy() const
{
    return lastHitBy_;
}

void Fighter::update(const struct Controller &controller, float dt)
{
    // Check for state transition
    if (state_->hasTransition())
    {
        FighterState *next = state_->nextState();
        delete state_;
        state_ = next;
    }

    // Update the attack
    if (attack_)
    {
        attack_->update(dt);
        if (attack_->isDone())
        {
            attack_->finish();
            delete attack_;
            attack_ = NULL;
        }
    }

    // Update last hit by
    lastHitExpireTime_ += dt;
    if (lastHitExpireTime_ > getParam("stats.hitExpireTime"))
            lastHitBy_ = -1;

    // Update state
    state_->update(controller, dt);

    // Update position
    rect_.x += xvel_ * dt;
    rect_.y += yvel_ * dt;
}

void Fighter::collisionWithGround(const Rectangle &ground, bool collision)
{
    state_->collisionWithGround(ground, collision);
}

void Fighter::attackCollision(const Attack *inAttack)
{
    // If two attacks collide, just cancel them and go to cooldown
    assert(attack_);
    assert(inAttack);
    attack_->attackCollision(inAttack);
}

void Fighter::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    assert(attack && attacker);
    state_->hitByAttack(attacker, attack);

    lastHitBy_ = attacker->id_;
    lastHitExpireTime_ = 0;

    // Play a sound
    std::string fname = "lvl";
    // we should freak out if damage is negative
    fname += '1' + floor(std::min(damage_ / 100, 2.0f));
    fname += attack->getAudioID();
    AudioManager::get()->playSound(fname);
}

void Fighter::hitWithAttack(Fighter *victim)
{
    assert(attack_);
    attack_->hit(victim);
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
    // Reset vars
    rect_.x = respawnx_;
    rect_.y = respawny_;
    xvel_ = yvel_ = 0.0f;
    damage_ = 0;
    lastHitBy_ = -1;
    // Set state to air normal
    delete state_;
    state_ = new AirNormalState(this);
    // Remove any attacks
    if (attack_)
    {
        delete attack_;
        attack_ = 0;
    }
    // If we died remove a life and play a sound
    if (killed)
    {
        --lives_;
        AudioManager::get()->playSound("ko");
    }
    // Check for death
    if (lives_ <= 0)
    {
        delete state_;
        state_ = new DeadState(this);
    }
}

bool Fighter::isAlive() const
{
    return lives_ > 0;
}

void Fighter::render(float dt)
{
    state_->render(dt);
}

void Fighter::renderHelper(float dt, const glm::vec3 &color)
{
    printf("ID: %d  Damage: %.1f  Position: [%.2f, %.2f]   Velocity: [%.2f, %.2f]  Attack: %d  Dir: %.1f  LastHitBy: %d\n",
            id_, damage_, rect_.x, rect_.y, xvel_, yvel_, attack_ != 0, dir_, lastHitBy_);

    // Draw body
    glm::mat4 transform(1.0);
    /*
    transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(dir_ * rect_.w, rect_.h, 1.0));
            */
    transform = glm::scale(
            glm::translate(transform, glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(dir_, 1.0f, 1.0f));
    renderFrame(transform, color, frame);

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
    return (damage_) / 33 + 1;
}

template<class AttackClass>
AttackClass Fighter::loadAttack(std::string attackName, const std::string &audioID)
{
    attackName += '.';

    AttackClass ret(
            getParam(attackName + "startup"),
            getParam(attackName + "duration"),
            getParam(attackName + "cooldown"),
            getParam(attackName + "damage"),
            getParam(attackName + "stun"),
            getParam(attackName + "knockbackpow") * glm::normalize(glm::vec2(
                    getParam(attackName + "knockbackx"),
                    getParam(attackName + "knockbacky"))),
            Rectangle(
                getParam(attackName + "hitboxx"),
                getParam(attackName + "hitboxy"),
                getParam(attackName + "hitboxw"),
                getParam(attackName + "hitboxh")),
            getParam(attackName + "priority"),
            audioID);

    return ret;

}

// ----------------------------------------------------------------------------
// FighterState class methods
// ----------------------------------------------------------------------------

void FighterState::calculateHitResult(const Fighter *attacker, const Attack *attack)
{
    // Cancel any current attack
    if (fighter_->attack_)
    {
        delete fighter_->attack_;
        fighter_->attack_ = 0;
    }
    // Take damage
    fighter_->damage_ += attack->getDamage(fighter_);

    // Calculate direction of hit
    glm::vec2 knockback = attack->getKnockback(fighter_) * glm::vec2(attacker->dir_, 1.0f)
        * fighter_->damageFunc();

    // Get knocked back
    fighter_->xvel_ = knockback.x;
    fighter_->yvel_ = knockback.y;

    // Generate a tiny explosion here
    glm::vec2 hitdir = glm::vec2(fighter_->rect_.x, fighter_->rect_.y)
        - glm::vec2(attack->getHitbox().x, attack->getHitbox().y);
    hitdir = glm::normalize(hitdir);
    float exx = -hitdir.x * fighter_->rect_.w / 2 + fighter_->rect_.x;
    float exy = -hitdir.y * fighter_->rect_.h / 2 + fighter_->rect_.y;
    ExplosionManager::get()->addExplosion(exx, exy, 0.2);

    // Go to the stunned state
    float stunDuration = attack->getStun(fighter_) * fighter_->damageFunc();
    next_ = new AirStunnedState(fighter_, stunDuration);
}

//// ---------------------- AIR STUNNED STATE -----------------------
AirStunnedState::AirStunnedState(Fighter *f, float duration) : 
    FighterState(f), stunDuration_(duration), stunTime_(0)
{
}

AirStunnedState::~AirStunnedState()
{
}

void AirStunnedState::update(const Controller&, float dt)
{
    // Gravity
    fighter_->yvel_ += getParam("airAccel") * dt;

    // Check for completetion
    if ((stunTime_ += dt) > stunDuration_)
        next_ = new AirNormalState(fighter_);
}

void AirStunnedState::render(float dt)
{
    printf("AIR STUNNED | StunTime: %.3f  StunDuration: %.3f || ",
            stunTime_, stunDuration_);
    // flash the player 
    float period_scale_factor = 20.0;
    float opacity_amplitude = 3;
    float opacity_factor = (1 + cos(period_scale_factor * stunTime_)) * 0.5f; 
    glm::vec3 color = fighter_->color_ * (opacity_amplitude * opacity_factor + 1);

    fighter_->renderHelper(dt, color);
}

void AirStunnedState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return;
    fighter_->xvel_ = 0;
    fighter_->yvel_ = 0;
    // If we're completely below the ground, no 'real' collision
    if (fighter_->rect_.y + fighter_->rect_.h/2 < ground.y + ground.h/2)
        return;
    // Overlap the ground by just one unit, only if some part of us is above
    fighter_->rect_.y = ground.y + ground.h/2 + fighter_->rect_.h/2 - 1;
    // Transition to the ground state
    if (next_) delete next_;
    next_ = new GroundState(fighter_);
}

void AirStunnedState::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    FighterState::calculateHitResult(attacker, attack);
}

//// ------------------------ GROUND STATE -------------------------
GroundState::GroundState(Fighter *f, float delay) :
    FighterState(f), jumpTime_(-1), dashTime_(-1), waitTime_(delay),
    dashing_(false)
{
}

GroundState::~GroundState()
{ /* Empty */ }

void GroundState::update(const Controller &controller, float dt)
{
    // Update running timers
    if (jumpTime_ >= 0) jumpTime_ += dt;
    if (dashTime_ >= 0) dashTime_ += dt;
    if (waitTime_ >= 0) waitTime_ -= dt;
    // If the fighter is currently attacking, do nothing else
    if (fighter_->attack_) return;
    // Do nothing during jump startup
    if (jumpTime_ > 0 && jumpTime_ < getParam("jumpStartupTime"))
        return;
    // Do nothing during dash startup
    if (dashTime_ > 0 && dashTime_ < getParam("dashStartupTime"))
        return;
    if (waitTime_ > 0)
        return;

    // -- Deal with starting an attack
    if (controller.pressa && jumpTime_ < 0)
    {
        // Check for dash attack
        if (dashing_)
        {
            dashing_ = false;
            fighter_->xvel_ = fighter_->dir_ * 1.25 * getParam("dashSpeed");
            fighter_->attack_ = fighter_->dashAttack_.clone();
        }
        // Not dashing- use a tilt
        else
        {
            // No movement during attack
            fighter_->xvel_ = 0; fighter_->yvel_ = 0;
            // Get direction of stick
            glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
            if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
            {
                // Do the L/R tilt
                fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
                fighter_->attack_ = fighter_->sideTiltAttack_.clone();
            }
            else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->downTiltAttack_.clone();
            }
            else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->upTiltAttack_.clone();
            }
            else
            {
                // Neutral tilt attack
                fighter_->attack_ = fighter_->neutralTiltAttack_.clone();
            }
        }
        // Do per attack stuff
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return;
    }
    else if (controller.pressb)
    {
        // Any B press is up B
        fighter_->attack_ = new UpSpecialAttack(fighter_->upSpecialAttack_);
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return;
    }

    // --- Deal with dashing movement ---
    if (dashing_ )
    {
        int newdir = controller.joyx < 0 ? -1 : 1;
        // Check for change of dash direction
        if (fighter_->dir_ != newdir && fabs(controller.joyxv) > getParam("input.velThresh") && fabs(controller.joyx) > getParam("input.dashMin"))
        {
            fighter_->dir_ = newdir;
            fighter_->xvel_ = 0;
            waitTime_ = getParam("dashChangeTime");
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    fighter_->rect_.x - fighter_->rect_.w * fighter_->dir_ * 0.4f, 
                    fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                    0.3f);
        }
        // Check for drop out of dash
        else if (fabs(controller.joyx) < getParam("input.dashMin") && fabs(controller.joyxv) < getParam("input.velThresh"))
        {
            dashing_ = false;
            waitTime_ = getParam("dashChangeTime");
            fighter_->xvel_ = 0;
            ExplosionManager::get()->addPuff(
                    fighter_->rect_.x + fighter_->rect_.w * fighter_->dir_ * 0.4f, 
                    fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                    0.3f);
        }
        // Otherwise just set the velocity
        else
        {
            fighter_->xvel_ = fighter_->dir_ * getParam("dashSpeed");
            // TODO add puffs every x amount of time
        }
    }
    // --- Deal with normal ground movement ---
    else
    {
        // Just move around a bit based on the controller
        if (fabs(controller.joyx) > getParam("input.deadzone"))
        {
            fighter_->xvel_ = controller.joyx * getParam("walkSpeed");
            fighter_->dir_ = fighter_->xvel_ < 0 ? -1 : 1;
        }
        // Only move when controller is held
        else
            fighter_->xvel_ = 0;

        // --- Check for dashing ---
        if (dashTime_ > getParam("dashStartupTime"))
        {
            dashing_ = true;
            dashTime_ = -1;
        }
        else if (fabs(controller.joyx) > getParam("input.dashThresh")
                && fabs(controller.joyxv) > getParam("input.velThresh"))
        {
            dashTime_ = 0;
            fighter_->xvel_ = 0;
            fighter_->dir_ = controller.joyx < 0 ? -1 : 1;
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    fighter_->rect_.x - fighter_->rect_.w * fighter_->dir_ * 0.4f, 
                    fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                    0.3f);
        }
    }


    // --- Deal with jumping ---
    if (jumpTime_ > getParam("jumpStartupTime"))
    {
        // Jump; transition to Air Normal
        next_ = new AirNormalState(fighter_);
        // Set the xvelocity of the jump
        fighter_->xvel_ = fabs(controller.joyx) > getParam("input.deadzone") ?
            controller.joyx * 0.5 * getParam("dashSpeed") :
            0.0f;
        // If they are still "holding down" the jump button now, then full jump
        // otherwise short hop
        if (controller.jumpbutton || controller.joyy > getParam("input.jumpThresh"))
            fighter_->yvel_ = getParam("jumpSpeed");
        else
            fighter_->yvel_ = getParam("hopSpeed");
        // Draw a little puff
        ExplosionManager::get()->addPuff(
                fighter_->rect_.x - fighter_->rect_.w * fighter_->dir_ * 0.1f, 
                fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                0.3f);
    }
    else if (controller.pressjump ||
            (controller.joyy > getParam("input.jumpThresh")
             && controller.joyyv > getParam("input.velThresh")))
    {
        // Start the jump timer
        jumpTime_ = 0.0f;
    }

}

void GroundState::render(float dt)
{
    printf("GROUND | JumpTime: %.3f  DashTime: %.3f  WaitTime: %.3f || ",
            jumpTime_, dashTime_, waitTime_);
    fighter_->renderHelper(dt, fighter_->color_);
}

void GroundState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision)
        next_ = new AirNormalState(fighter_);

    // If there is a collision, we don't need to do anything, because we're
    // already in the GroundState
}

void GroundState::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    // Pop up a bit so that we're not overlapping the ground
    fighter_->rect_.x += 2;
    // Then do the normal stuff
    FighterState::calculateHitResult(attacker, attack);
}

//// -------------------- AIR NORMAL STATE -----------------------------
AirNormalState::AirNormalState(Fighter *f) :
    FighterState(f), canSecondJump_(true), jumpTime_(-1)
{
}

AirNormalState::~AirNormalState()
{ /* Empty */ }

void AirNormalState::update(const Controller &controller, float dt)
{
    // Gravity
    fighter_->yvel_ += getParam("airAccel") * dt;

    // Update running timers
    if (jumpTime_ >= 0) jumpTime_ += dt;
    // If the fighter is currently attacking, do nothing else
    if (fighter_->attack_) return;

    // Let them control the character slightly
    if (fabs(controller.joyx) > getParam("input.deadzone"))
    {
        // Don't let the player increase the velocity past a certain speed
        if (fighter_->xvel_ * controller.joyx <= 0 || fabs(fighter_->xvel_) < getParam("jumpAirSpeed"))
            fighter_->xvel_ += controller.joyx * getParam("airForce") * dt;
        // You can always control your orientation
        fighter_->dir_ = controller.joyx < 0 ? -1 : 1;
    }

    // --- Check for attack ---
    if (controller.pressa)
    {
        // Get direction of stick
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
        if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
        {
            // Do the L/R tilt
            fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
            fighter_->attack_ = fighter_->airSideAttack_.clone();
        }
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->airDownAttack_.clone();
        }
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->airUpAttack_.clone();
        }
        else
        {
            // Neutral tilt attack
            fighter_->attack_ = new Attack(fighter_->airNeutralAttack_);
        }
        // Do stuff to all attacks
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }
    else if (controller.pressb)
    {
        // Any B press is up B
        fighter_->attack_ = new UpSpecialAttack(fighter_->upSpecialAttack_);
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }

    // If we just added an attack, lets not jump too
    if (fighter_->attack_)
    {
        jumpTime_ = -1;
        return;
    }

    // --- Check for jump ---
    if ((controller.pressjump || (controller.joyy > getParam("input.jumpThresh") && 
                    controller.joyyv > getParam("input.velThresh"))) && canSecondJump_)
    {
        jumpTime_ = 0;
    }
    if (jumpTime_ > getParam("jumpStartupTime"))
    {
        fighter_->yvel_ = getParam("secondJumpSpeed");
        fighter_->xvel_ = fabs(controller.joyx) > getParam("input.deadzone") ?
            getParam("dashSpeed") * std::max(-1.0f, std::min(1.0f, (controller.joyx - 0.2f) / 0.6f)) :
            0.0f;
        jumpTime_ = -1;
        canSecondJump_ = false;
        // Draw a puff
        ExplosionManager::get()->addPuff(
                fighter_->rect_.x - fighter_->rect_.w * fighter_->dir_ * 0.1f, 
                fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                0.3f);
    }
}

void AirNormalState::render(float dt)
{
    printf("AIR NORMAL | JumpTime: %.3f  Can2ndJump: %d || ",
            jumpTime_, canSecondJump_);
    fighter_->renderHelper(dt, fighter_->color_);
}

void AirNormalState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return;
    // If we're completely below the ground, no 'real' collision
    if (fighter_->rect_.y + fighter_->rect_.h/2 < ground.y + ground.h/2)
        return;
    // Overlap the ground by just one unit, only if some part of us is above
    fighter_->rect_.y = ground.y + ground.h/2 + fighter_->rect_.h/2 - 1;
    fighter_->xvel_ = 0;
    fighter_->yvel_ = 0;

    // Cancel any attack if it exists
    if (fighter_->attack_)
        fighter_->attack_->cancel();
    // Transition to the ground state, with a small delay for landing
    if (next_) delete next_;
    next_ = new GroundState(fighter_, getParam("landingCooldownTime"));

    // Draw some puffs for landing
    ExplosionManager::get()->addPuff(
            fighter_->rect_.x - fighter_->rect_.w * fighter_->dir_ * 0.4f, 
            fighter_->rect_.y - fighter_->rect_.h * 0.45f,
            0.3f);
    ExplosionManager::get()->addPuff(
            fighter_->rect_.x + fighter_->rect_.w * fighter_->dir_ * 0.4f, 
            fighter_->rect_.y - fighter_->rect_.h * 0.45f,
            0.3f);

}

void AirNormalState::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    FighterState::calculateHitResult(attacker, attack);
}


// ----------------------------------------------------------------------------
// Rectangle class methods
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Attack class methods
// ----------------------------------------------------------------------------

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
    t_ = 0.0f;
    hasHit_ = false;
}

void Attack::finish()
{
    /* Empty */
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

void Attack::hit(Fighter *other)
{
    hasHit_ = true;
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

            // Go to air normal state
            delete owner_->state_;
            owner_->state_ = new AirNormalState(owner_);
            started_ = true;
        }

        repeatTime_ += dt;
    }
    if (repeatTime_ > getParam("upSpecialAttack.repeatInterval"))
    {
        hasHit_ = false;
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
