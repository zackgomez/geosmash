#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "audio.h"

void pause(int playerID);
Fighter *getPartner(int playerID);

Fighter::Fighter(float respawnx, float respawny, const glm::vec3& color, int id) :
    rect_(Rectangle(0, 0, getParam("fighter.w"), getParam("fighter.h"))),
    xvel_(0), yvel_(0),
    dir_(-1),
    state_(0),
    damage_(0), lives_(getParam("fighter.lives")),
    respawnx_(respawnx), respawny_(respawny),
    color_(color), id_(id),
    attack_(NULL),
    lastHitBy_(-1)
{
    // Load ground attacks
    std::string g = "groundhit";
    std::string a = "airhit";

    dashAttack_ = loadAttack<DashAttack>("dashAttack", g, "DashAttack");

    neutralTiltAttack_ = loadAttack<Attack>("neutralTiltAttack", g, "GroundNeutral");
    sideTiltAttack_ = loadAttack<Attack>("sideTiltAttack", g, "GroundSidetilt");
    downTiltAttack_ = loadAttack<Attack>("downTiltAttack", g, "GroundDowntilt");
    upTiltAttack_ = loadAttack<Attack>("upTiltAttack", g, "GroundUptilt");

    // Load air attack special as it uses a different class
    airNeutralAttack_ = loadAttack<Attack>("airNeutralAttack", a, "AirNeutral");
    airFrontAttack_ = loadAttack<Attack>("airFrontAttack", a, "AirFronttilt");
    airBackAttack_ = loadAttack<Attack>("airBackAttack", a, "AirBacktilt");
    airDownAttack_ = loadAttack<Attack>("airDownAttack", a, "AirDowntilt");
    airUpAttack_ = loadAttack<Attack>("airUpAttack", a, "AirUptilt");

    upSpecialAttack_ = loadAttack<UpSpecialAttack>("upSpecialAttack", a, "UpSpecial");

    tauntAttack_ = loadAttack<Attack>("tauntAttack", a, "TauntAttack");

    neutralSmashAttack_ = loadAttack<Attack>("neutralSmashAttack", g, "NeutralSmash");
    neutralSmashAttack_->setTwinkle(true);
    //neutralSmashAttack_->setHitboxFrame("NeutralSmashHitbox");
    sideSmashAttack_ = loadAttack<Attack>("sideSmashAttack", g, "SideSmash");
    sideSmashAttack_->setTwinkle(true);
    sideSmashAttack_->setHitboxFrame("SideSmashHitbox");
    downSmashAttack_ = loadAttack<Attack>("downSmashAttack", g, "DownSmash");
    downSmashAttack_->setTwinkle(true);
    downSmashAttack_->setHitboxFrame("DownSmashHitbox");
    upSmashAttack_ = loadAttack<MovingAttack>("upSmashAttack", g, "UpSmash");
    upSmashAttack_->setTwinkle(true);
    upSmashAttack_->setHitboxFrame("UpSmashHitbox");

    // Set up the twinkle moves
    airFrontAttack_->setTwinkle(true);

    state_ = 0;
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

void Fighter::update(Controller &controller, float dt)
{
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

    // Check for state transition
    if (state_->hasTransition())
    {
        FighterState *next = state_->nextState();
        delete state_;
        state_ = next;
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
    assert(attack->canHit(this));

    state_->hitByAttack(attacker, attack);

    lastHitBy_ = attacker->id_;

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
    assert(attack_->canHit(victim));

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

bool Fighter::canBeHit() const
{
    return state_->canBeHit();
}

void Fighter::respawn(bool killed)
{
    // Remove any attacks
    if (attack_)
    {
        attack_->finish();
        delete attack_;
        attack_ = 0;
    }
    // Reset vars
    rect_.x = respawnx_;
    rect_.y = respawny_;
    xvel_ = yvel_ = 0.0f;
    damage_ = 0;
    lastHitBy_ = -1;
    // If we died remove a life and play a sound
    if (killed)
    {
        --lives_;
        AudioManager::get()->playSound("ko");
    }
    delete state_;
    // Set the new state, either respawn or dead
    if (lives_ <= 0)
        state_ = new DeadState(this);
    else
        // Set state to respawn state
        state_ = new RespawnState(this);
}

bool Fighter::isAlive() const
{
    return lives_ > 0;
}

void Fighter::render(float dt)
{
    state_->render(dt);
}

void Fighter::renderHelper(float dt, const std::string &frameName, const glm::vec3 &color,
        const glm::mat4 &postTrans)
{
    printf("ID: %d  Damage: %.1f  Position: [%.2f, %.2f]   Velocity: [%.2f, %.2f]  Attack: %d  Dir: %.1f  LastHitBy: %d\n",
            id_, damage_, rect_.x, rect_.y, xvel_, yvel_, attack_ != 0, dir_, lastHitBy_);

    // Draw body
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(dir_, 1.0f, 1.0f));

    FrameManager::get()->renderFrame(transform * postTrans, glm::vec4(color, 0.25f), frameName);

    if (attack_)
        attack_->render(dt);
}

float Fighter::damageFunc() const
{
    return 1.5*(damage_) / 33 + 1.5;
}

template<class AttackClass>
AttackClass* Fighter::loadAttack(std::string attackName, const std::string &audioID,
        const std::string &fname)
{
    /*
    attackName += '.';

    AttackClass* ret = new AttackClass(
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
    ret->setFrameName(fname);
    */

    AttackClass *ret = new AttackClass(attackName, audioID, fname);

    return ret;

}

// ----------------------------------------------------------------------------
// FighterState class methods
// ----------------------------------------------------------------------------

bool FighterState::canBeHit() const
{
    return true;
}

void FighterState::calculateHitResult(const Fighter *attacker, const Attack *attack)
{
    // Cancel any current attack
    if (fighter_->attack_)
    {
        fighter_->attack_->finish();
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
    std::cout << "StunDuration: " << stunDuration << '\n';
    next_ = new AirStunnedState(fighter_, stunDuration);
}

void FighterState::collisionHelper(const Rectangle &ground)
{
    // If the player is entirely inside of the rectangle
    // just move them on top of it, minus 1 pixel
    if (ground.contains(fighter_->rect_))
        fighter_->rect_.y = ground.y + ground.h/2 + fighter_->rect_.h/2 - 1;
    // If the character has some part above the ground wihen overlap, let
    // them 'ledge grab'
    else if (fighter_->rect_.y + fighter_->rect_.h/2 > ground.y + ground.h/2)
        fighter_->rect_.y = ground.y + ground.h/2 + fighter_->rect_.h/2 - 1;
    // If the character is not above the ground, and not overlapping, then
    // if their center is within the ground, move them down so they are not
    // covering the rectangle
    else if (fighter_->rect_.x < ground.x + ground.w/2
            && fighter_->rect_.x > ground.x - ground.w/2)
        fighter_->rect_.y = ground.y - ground.h/2 - fighter_->rect_.h/2;
    // If the character is not above the ground, and their center is not
    // in the ground, move them so they don't overlap in x/y
    else if (fighter_->rect_.y + fighter_->rect_.h/2 < ground.y + ground.h/2
            && (fighter_->rect_.x > ground.x + ground.w/2 || fighter_->rect_.x < ground.x - ground.w/2))
    {
        float dist = ground.x - fighter_->rect_.x;
        float dir = dist / fabs(dist);
        fighter_->rect_.x += dir * (fabs(dist) - (ground.w/2 + fighter_->rect_.w/2));
    }

}

//// ---------------------- AIR STUNNED STATE -----------------------
AirStunnedState::AirStunnedState(Fighter *f, float duration) : 
    FighterState(f), stunDuration_(duration), stunTime_(0)
{
    frameName_ = "AirStunned";

    std::cout << "AIR STUNNED CONSTRUCTED.\n";
}

AirStunnedState::~AirStunnedState()
{
}

void AirStunnedState::update(Controller &controller, float dt)
{
    // Check for pause
    if (controller.pressstart)
        pause(fighter_->id_);

    // Gravity
    fighter_->yvel_ += getParam("airAccel") * dt;

    // Let them control the character slightly
    if (fabs(controller.joyx) > getParam("input.deadzone"))
    {
        // Don't let the player increase the velocity past a certain speed
        if (fighter_->xvel_ * controller.joyx <= 0 || fabs(fighter_->xvel_) < getParam("jumpAirSpeed"))
            fighter_->xvel_ += controller.joyx * getParam("airDI") * dt;
    }

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

    std::string fname = frameName_;
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    fighter_->renderHelper(dt, fname, color);
}

void AirStunnedState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return;

    std::cout << "Air Stun collision\n";
    FighterState::collisionHelper(ground);
    // If we're not overlapping the ground anymore, no collision
    if (!ground.overlaps(fighter_->rect_))
        return;
    fighter_->xvel_ = 0;
    fighter_->yvel_ = 0;
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
    frameName_ = "GroundNormal";
    fighter_->lastHitBy_ = -1;
}

GroundState::~GroundState()
{ /* Empty */ }

void GroundState::update(Controller &controller, float dt)
{
    // Check for pause
    if (controller.pressstart)
        pause(fighter_->id_);

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

    // --- Deal with jump transition ---
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
        return;
    }


    // -- Deal with starting an attack
    if (controller.pressa)
    {
        // Check for dash attack
        if (dashing_)
        {
            dashing_ = false;
            fighter_->xvel_ = fighter_->dir_ * getParam("dashSpeed");
            fighter_->attack_ = fighter_->dashAttack_->clone();
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
                fighter_->attack_ = fighter_->sideTiltAttack_->clone();
            }
            else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->downTiltAttack_->clone();
            }
            else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->upTiltAttack_->clone();
            }
            else
            {
                // Neutral tilt attack
                fighter_->attack_ = fighter_->neutralTiltAttack_->clone();
            }
        }
        // Do per attack stuff
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return;
    }
    // Check for dodge
    else if ((controller.rbumper || controller.rtrigger < -getParam("input.trigThresh") || controller.ltrigger < -getParam("input.trigThresh"))
            && fabs(controller.joyx) > getParam("input.dodgeThresh")
            && fabs(controller.joyxv) > getParam("input.velThresh"))
    {
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
        next_ = new DodgeState(fighter_);
        ExplosionManager::get()->addPuff(
                fighter_->rect_.x + fighter_->rect_.w * fighter_->dir_ * 0.4f, 
                fighter_->rect_.y - fighter_->rect_.h * 0.45f,
                0.3f);
        return;
    }
    // Check for B moves
    else if (controller.pressb)
    {
        // Any B press is up B
        fighter_->attack_ = fighter_->upSpecialAttack_->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return;
    }
    // Check for taunt
    else if (controller.dpadu && !dashing_)
    {
        fighter_->attack_ = fighter_->tauntAttack_->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return;
    }
    // Check for smash attacks
    else if (controller.pressc && !dashing_)
    {
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
        // No movement during smash attacks
        fighter_->xvel_ = 0; fighter_->yvel_ = 0;

        if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
        {
            fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
            fighter_->attack_ = fighter_->sideSmashAttack_->clone();
        }
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            fighter_->attack_ = fighter_->downSmashAttack_->clone();
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            fighter_->attack_ = fighter_->upSmashAttack_->clone();
        else
            fighter_->attack_ = fighter_->neutralSmashAttack_->clone();

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

    // --- Check to see if they want to start a jump ---
    if (controller.pressjump ||
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

    std::string fname = frameName_;
    if (dashing_)
        fname = "GroundRunning";
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    fighter_->renderHelper(dt, fname, fighter_->color_);
}

void GroundState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (next_)
        return;
    if (!collision)
    {
        if (fighter_->attack_)
        {
            fighter_->attack_->cancel();
            dashing_ = false;
            float dir = (ground.x - fighter_->rect_.x) > 0 ? 1 : -1;
            fighter_->rect_.x += dir * (fabs(ground.x - fighter_->rect_.x) - ground.w/2 - fighter_->rect_.w/2 + 2);
        }
        else
        {
            next_ = new AirNormalState(fighter_);
        }
    }

    // If there is a collision, we don't need to do anything, because we're
    // already in the GroundState
}

void GroundState::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    // Pop up a bit so that we're not overlapping the ground
    fighter_->rect_.y += 4;
    // Then do the normal stuff
    FighterState::calculateHitResult(attacker, attack);
}

//// -------------------- AIR NORMAL STATE -----------------------------
AirNormalState::AirNormalState(Fighter *f) :
    FighterState(f), canSecondJump_(true), jumpTime_(-1)
{
    frameName_ = "AirNormal";
}

AirNormalState::~AirNormalState()
{ /* Empty */ }

void AirNormalState::update(Controller &controller, float dt)
{
    // Gravity
    fighter_->yvel_ += getParam("airAccel") * dt;

    // Check for pause
    if (controller.pressstart)
        pause(fighter_->id_);

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
    }
    // Fast falling
    if (fighter_->yvel_ < 0 && fighter_->yvel_ > getParam("fastFallMaxSpeed") && controller.joyy < -getParam("input.fallThresh"))
    {
        if (!fastFalling_ && fighter_->yvel_ > getParam("fastFallMaxInitialSpeed"))
            fighter_->yvel_ += getParam("fastFallInitialSpeed");
        else
            fighter_->yvel_ += getParam("fastFallAccel") * dt;
        fastFalling_ = true;
    }

    // --- Check for attack ---
    if (controller.pressa)
    {
        // Get direction of stick
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));

        if (tiltDir.x * fighter_->dir_ > 0 && fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
            // front tilt
            fighter_->attack_ = fighter_->airFrontAttack_->clone();
        else if (tiltDir.x * fighter_->dir_ < 0 && fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
            // Do the back
            fighter_->attack_ = fighter_->airBackAttack_->clone();
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->airDownAttack_->clone();
        }
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->airUpAttack_->clone();
        }
        else
        {
            // Neutral tilt attack
            fighter_->attack_ = fighter_->airNeutralAttack_->clone();
        }
        // Do stuff to all attacks
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }
    else if (controller.pressb)
    {
        fighter_->dir_ = controller.joyx < 0 ? -1 : 1;
        // Any B press is up B
        fighter_->attack_ = fighter_->upSpecialAttack_->clone();
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
    // 
    if (jumpTime_ > getParam("jumpStartupTime"))
    {
        fighter_->yvel_ = getParam("secondJumpSpeed");
        fighter_->xvel_ = fabs(controller.joyx) > getParam("input.deadzone") ?
            0.8f * getParam("dashSpeed") * std::max(-1.0f, std::min(1.0f, (controller.joyx - 0.2f) / 0.6f)) :
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
    std::string fname = frameName_;
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    fighter_->renderHelper(dt, fname, fighter_->color_);
}

void AirNormalState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return;
    FighterState::collisionHelper(ground);
    // If we're not overlapping the ground anymore, no collision
    if (!ground.overlaps(fighter_->rect_))
        return;
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

//// ----------------------- DODGE STATE --------------------------------
DodgeState::DodgeState(Fighter *f) :
    FighterState(f), t_(0.f), dodgeTime_(getParam("dodge.duration")),
    invincTime_(getParam("dodge.invincTime")), cooldown_(getParam("dodge.cooldown"))
{
    frameName_ = "GroundRoll";
}

void DodgeState::update(Controller &controller, float dt)
{
    // Check for pause
    if (controller.pressstart)
        pause(fighter_->id_);

    t_ += dt;
    fighter_->xvel_ = fighter_->dir_ * getParam("dodge.speed");
    
    if (t_ > dodgeTime_)
        fighter_->xvel_ = 0;

    if (t_ > dodgeTime_ && t_ - dt < dodgeTime_)
        fighter_->dir_ = -fighter_->dir_;

    if (t_ > dodgeTime_ + cooldown_)
        next_ = new GroundState(fighter_);

}

void DodgeState::render(float dt)
{
    float period_scale_factor = 20.0;
    float opacity_amplitude = 3;
    float opacity_factor = (1 + cos(period_scale_factor * t_)) * 0.5f; 
    glm::vec3 color = fighter_->color_ * (opacity_amplitude * opacity_factor + 1);

    float angle = t_ < dodgeTime_ ? t_ / dodgeTime_ * 360 : 0.f;

    if (t_ > invincTime_)
        color = fighter_->color_;

    printf("DODGE | t: %.3f  invincTime: %.3f  dodgeTime: %.3f || ",
            t_, invincTime_, dodgeTime_);
    // Just render the fighter, but flashing
    fighter_->renderHelper(dt, frameName_, color,
            glm::rotate(glm::mat4(1.f), -angle, glm::vec3(0,0,1)));
}

void DodgeState::hitByAttack(const Fighter *attacker, const Attack *attack)
{
    // Pop up a bit so that we're not overlapping the ground
    fighter_->rect_.y += 2;
    // Then do the normal stuff
    FighterState::calculateHitResult(attacker, attack);
}

void DodgeState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision && !next_)
    {
        float dir = (ground.x - fighter_->rect_.x) > 0 ? 1 : -1;
        fighter_->rect_.x += dir * (fabs(ground.x - fighter_->rect_.x) - ground.w/2 - fighter_->rect_.w/2 + 2);
        fighter_->xvel_ = 0.0f;
        t_ = std::max(t_, invincTime_);
    }
}

bool DodgeState::canBeHit() const
{
    return t_ > invincTime_;
}

//// ----------------------- RESPAWN STATE --------------------------------
RespawnState::RespawnState(Fighter *f) :
    FighterState(f), t_(0.f)
{
}

void RespawnState::update(Controller &controller, float dt)
{
    // Check for pause
    if (controller.pressstart)
        pause(fighter_->id_);

    t_ += dt;
    if (t_ > getParam("fighter.respawnTime"))
        next_ = new AirNormalState(fighter_);
}

void RespawnState::render(float dt)
{
    // Just render the fighter, but slightly lighter
    fighter_->renderHelper(dt, frameName_, 1.6f * fighter_->color_);
}

void RespawnState::hitByAttack(const Fighter *, const Attack *)
{
    // Do nothing, we're invincible bitch!
}

void RespawnState::collisionWithGround(const Rectangle &ground, bool collision)
{
    assert(!collision);
}

//// ------------------------- DEAD STATE ----------------------------------
void DeadState::update(Controller &controller, float dt)
{
    if (controller.pressstart)
    {
        Fighter *partner = getPartner(fighter_->id_);
        if (partner && partner->lives_ > 1)
        {
            partner->lives_--;
            fighter_->lives_++;
            fighter_->respawn(false);
        }
        // Consume press
        controller.pressstart = false;
    }
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

bool Rectangle::contains(const Rectangle &rhs) const
{
    return (rhs.x - rhs.w/2) > (x - w/2) && (rhs.x + rhs.w/2) < (x + w/2) &&
        (rhs.y - rhs.h/2) > (y - h/2) && (rhs.y + rhs.h/2) < (y + h/2);
}

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
    hitbox_ = Rectangle(
            getParam(pp + "hitboxx"),
            getParam(pp + "hitboxy"),
            getParam(pp + "hitboxw"),
            getParam(pp + "hitboxh"));
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
    ret.x = hitbox_.x * owner_->getDirection() + owner_->getRectangle().x;
    ret.y = hitbox_.y + owner_->getRectangle().y;
    ret.h = hitbox_.h;
    ret.w = hitbox_.w;

    return ret;
}

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

Attack * DashAttack::clone() const
{
    return new DashAttack(*this);
}

void DashAttack::start()
{
    Attack::start();

    owner_->xvel_ = getParam("dashAttack.initialSpeed") * owner_->dir_;
    // Calculate acceleration needed to completely decelerate over duration
    accel_ = - getParam("dashAttack.deceleration") * owner_->dir_;
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
        owner_->xvel_ += accel_ * dt;
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
    ret.w = hitbox_.w; ret.h = hitbox_.h;

    return ret;
}
