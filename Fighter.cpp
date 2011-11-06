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

Fighter::Fighter(float respawnx, float respawny, const glm::vec3& color) :
    rect_(Rectangle(0, 0, getParam("fighter.w"), getParam("fighter.h"))),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(AIR_NORMAL_STATE),
    damage_(0), lives_(getParam("fighter.lives")),
    respawnx_(respawnx), respawny_(respawny),
    color_(color),
    attack_(NULL)
{
    std::cout << "RESPAWN: " << respawnx_ << ' ' << respawny_ << '\n';
    // Load ground attacks
    dashAttack_ = loadAttack("dashAttack", "sfx/neutral001.wav");
    neutralTiltAttack_ = loadAttack("neutralTiltAttack", "sfx/neutral001.wav");
    sideTiltAttack_ = loadAttack("sideTiltAttack", "sfx/forwardtilt001.wav");
    downTiltAttack_ = loadAttack("downTiltAttack", "sfx/downtilt001.wav");
    upTiltAttack_ = loadAttack("upTiltAttack", "sfx/uptilt001.wav");

    // Load air attack special as it uses a different class
    airNeutralAttack_ = loadAttack("airNeutralAttack", "sfx/uptilt001.wav");
    airSideAttack_ = loadAttack("airSideAttack", "sfx/uptilt001.wav");
    airDownAttack_ = loadAttack("airDownAttack", "sfx/uptilt001.wav");
    airUpAttack_ = loadAttack("airUpAttack", "sfx/uptilt001.wav");

    state_ = 0;

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
            delete attack_;
            attack_ = NULL;
        }
    }

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
    state_->hitByAttack(fighter, inAttack);
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
    // Reset vars
    rect_.x = respawnx_;
    rect_.y = respawny_;
    xvel_ = yvel_ = 0.0f;
    damage_ = 0;
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
        koSound->Play();
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
    printf("Damage: %.2f  Position: [%.4f, %.4f]   Velocity: [%.4f, %.4f]  Attack: %d  Dir: %.1f\n", 
            damage_, rect_.x, rect_.y, xvel_, yvel_, attack_ != 0, dir_);

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
    return (damage_) / 33 + 1;
}

Attack Fighter::loadAttack(std::string attackName, std::string soundFile)
{
    attackName += '.';

    Attack ret(
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
                getParam(attackName + "hitboxh")));

    if (!soundFile.empty())
    {
        sf::Music *m = new sf::Music();
        m->OpenFromFile(soundFile);
        ret.setSound(m);
    }

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

    // -- Deal with starting an attack
    if (controller.pressa && jumpTime_ < 0)
    {
        // Check for dash attack
        if (dashing_)
        {
            dashing_ = false;
            fighter_->xvel_ = fighter_->dir_ * getParam("dashSpeed");
            fighter_->attack_ = new Attack(fighter_->dashAttack_);
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
                fighter_->attack_ = new Attack(fighter_->sideTiltAttack_);
            }
            else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = new Attack(fighter_->downTiltAttack_);
            }
            else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = new Attack(fighter_->upTiltAttack_);
            }
            else
            {
                // Neutral tilt attack
                fighter_->attack_ = new Attack(fighter_->neutralTiltAttack_);
            }
        }
        // Set the owner of the attack
        fighter_->attack_->setFighter(fighter_);
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
    if (f->attack_)
        f->attack_->cancel();
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
    // --- Check for attack ---
    if (controller.pressa)
    {
        // Get direction of stick
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
        if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
        {
            // Do the L/R tilt
            fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
            fighter_->attack_ = new Attack(fighter_->airSideAttack_);
        }
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = new Attack(fighter_->airDownAttack_);
        }
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = new Attack(fighter_->airUpAttack_);
        }
        else
        {
            // Neutral tilt attack
            fighter_->attack_ = new Attack(fighter_->airNeutralAttack_);
        }
        // Set the owner of the attack
        fighter_->attack_->setFighter(fighter_);
        jumpTime_ = -1;
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

