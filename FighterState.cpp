#include <glm/gtc/matrix_transform.hpp>
#include "FighterState.h"
#include "Fighter.h"
#include "Attack.h"
#include "ParamReader.h"
#include "explosion.h"
#include "Projectile.h"
#include "glutils.h"
#include "audio.h"
#include "StatsManager.h"
#include "StageManager.h"

int getTeamID(int);

// ----------------------------------------------------------------------------
// FighterState class methods
// ----------------------------------------------------------------------------

bool FighterState::canBeHit() const
{
    return invincTime_ <= 0.f;
}

Rectangle FighterState::getRect() const
{
    return Rectangle(fighter_->pos_.x, fighter_->pos_.y,
            fighter_->size_.x, fighter_->size_.y);
}

void FighterState::update(float dt)
{
    invincTime_ -= dt;
}

FighterState* FighterState::calculateHitResult(const Attack *attack)
{
    // Cancel any current attack
    if (fighter_->attack_)
    {
        fighter_->attack_->finish();
        delete fighter_->attack_;
        fighter_->attack_ = 0;
    }

    // Take damage
    float dd = attack->getDamage(fighter_);
    fighter_->damage_ += dd;
    // Record damage given/taken
    StatsManager::get()->addStat(StatsManager::getStatPrefix(fighter_->playerID_) + "damageTaken", dd);
    if (getTeamID(attack->getPlayerID()) == getTeamID(fighter_->playerID_))
        StatsManager::get()->addStat(StatsManager::getStatPrefix(attack->getPlayerID()) + "teamDamageGiven", dd);
    else
        StatsManager::get()->addStat(StatsManager::getStatPrefix(attack->getPlayerID()) + "damageGiven", dd);


    // Calculate direction of hit
    glm::vec2 knockback = attack->getKnockback(fighter_) * fighter_->damageFunc();

    // Get knocked back
    fighter_->vel_ = knockback;

    // Generate a tiny explosion here
    glm::vec2 hitdir = glm::vec2(fighter_->pos_.x, fighter_->pos_.y)
        - glm::vec2(attack->getHitbox().x, attack->getHitbox().y);
    hitdir = glm::normalize(hitdir);
    float exx = -hitdir.x * fighter_->size_.x / 2 + fighter_->pos_.x;
    float exy = -hitdir.y * fighter_->size_.y / 2 + fighter_->pos_.y;
    ExplosionManager::get()->addExplosion(exx, exy, 0.2);

    // Play a sound
    AudioManager::get()->playSound(attack->getAudioID(), fighter_->pos_, fighter_->damage_);

    // Pop up a bit so that we're not overlapping the ground
    fighter_->pos_.y += 4;

    // Go to the stunned state
    float stunDuration = attack->getStun(fighter_) * fighter_->damageFunc();
    std::cout << "StunDuration: " << stunDuration << '\n';
    return new AirStunnedState(fighter_, stunDuration, fighter_->vel_.y < 0);
}

void FighterState::collisionHelper(const Rectangle &ground)
{
    // If the player is entirely inside of the rectangle
    // just move them on top of it, minus 1 pixel
    if (ground.contains(fighter_->getRect()))
        fighter_->pos_.y = ground.y + ground.h/2 + fighter_->size_.y/2 - 1;
    // If the character's center is above the ground then move them on top
    // minus one pixel to ensure they continue to be in the GroundState
    else if (fighter_->pos_.y > ground.y + ground.h/2)
        fighter_->pos_.y = ground.y + ground.h/2 + fighter_->size_.y/2 - 1;
    // If the character is not above the ground, and not overlapping, then
    // if their center is within the ground, move them down so they are not
    // covering the rectangle
    else if (fighter_->pos_.x < ground.x + ground.w/2
            && fighter_->pos_.x > ground.x - ground.w/2)
        fighter_->pos_.y = ground.y - ground.h/2 - fighter_->size_.y/2;
    // If the character is not above the ground, and their center is not
    // in the ground, move them so they don't overlap in x/y
    else if (fighter_->pos_.y < ground.y + ground.h/2
            && (fighter_->pos_.x > ground.x + ground.w/2 || fighter_->pos_.x < ground.x - ground.w/2))
    {
        float dist = ground.x - fighter_->pos_.x;
        float dir = dist / fabs(dist);
        fighter_->pos_.x += dir * (fabs(dist) - (ground.w/2 + fighter_->size_.x/2));
    }
}

FighterState* FighterState::checkForLedgeGrab()
{
    // You must be /*facing the ledge*/ and be completely below the ledge and
    // be within ledge grabbing distance of the ledge and not attacking
    const glm::vec2 &fpos = fighter_->pos_;
    Ledge *ledge = StageManager::get()->getPossibleLedge(fpos);

    // We should either get no ledge or a ledge that's unoccupied
    assert(!ledge || !ledge->occupied);
    if (ledge
        && !fighter_->attack_
        && (ledge->pos.y > (fpos.y + fighter_->getRect().h / 2)) 
        && glm::length(fpos - ledge->pos) <= getParam("ledgeGrab.dist")
        && ledge->dir * (fpos.x - ledge->pos.x) >= 1)
    {
        AudioManager::get()->playSound("ledgegrab");
        LedgeGrabState *lgs = new LedgeGrabState(fighter_);
        lgs->grabLedge(ledge);
        return lgs;
    }

    // No ledge grab
    return NULL;
}

FighterState* FighterState::performBMove(const Controller &controller, bool ground)
{
    assert(controller.pressb);
    assert(!fighter_->attack_);
    FighterState *next = NULL;

    glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
    // Check for up B
    if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
    {
        next = new UpSpecialState(fighter_);
    }
    // Check for down B
    else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
    {
        next = new CounterState(fighter_, ground);
    }
    // Check for side B
    else if (fabs(controller.joyx) > getParam("input.tiltThresh") 
                && fabs(tiltDir.x) > fabs(tiltDir.y))
    {
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
        fighter_->attack_ = fighter_->attackMap_["sideSpecial"]->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }
    // Otherwise neutral B
    else
    {
        fighter_->attack_ = fighter_->attackMap_["neutralSpecial"]->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }

    return next;
}


//// ---------------------- AIR STUNNED STATE -----------------------
AirStunnedState::AirStunnedState(Fighter *f, float duration, bool gb) : 
    FighterState(f), stunDuration_(duration), stunTime_(0), gb_(gb)
{
    frameName_ = "AirStunned";
}

FighterState* AirStunnedState::processInput(Controller &controller, float dt)
{
    // Gravity
    fighter_->accel_ = glm::vec2(0.f, getParam("airAccel"));

    // Let them control the character slightly
    if (fabs(controller.joyx) > getParam("input.deadzone"))
    {
        // Don't let the player increase the velocity past a certain speed
        if (fighter_->vel_.x * controller.joyx <= 0 || fabs(fighter_->vel_.x) < getParam("jumpAirSpeed"))
            fighter_->vel_.x += controller.joyx * getParam("airDI") * dt;
    }

    // Check for completetion
    if ((stunTime_ += dt) > stunDuration_)
        return new AirNormalState(fighter_);
    // Only check for ledge grab after up B
    if (stunDuration_ == HUGE_VAL)
        return checkForLedgeGrab();

    // No state change
    return NULL;
}

void AirStunnedState::render(float dt)
{
    printf("AIR STUNNED | StunTime: %.3f  StunDuration: %.3f || ",
            stunTime_, stunDuration_);
    // flash the player 
    glm::vec3 color = muxByTime(fighter_->color_, stunTime_);

    std::string fname = frameName_;
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    fighter_->renderHelper(dt, fname, color);
}

FighterState* AirStunnedState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return NULL;

    FighterState::collisionHelper(ground);
    // If we're not overlapping the ground anymore, no collision
    if (!ground.overlaps(fighter_->getRect()))
        return NULL;

    // Check for ground bounce
    if (fighter_->vel_.y < -getParam("fighter.gbThresh")
            && gb_)
    {
        // reflect and dampen yvel and stun
        fighter_->vel_.y *= -getParam("fighter.gbVelDamping");
        fighter_->vel_.x *= getParam("fighter.gbVelDamping");
        stunTime_ *= getParam("fighter.gbStunDamping");
        // Cannot be bounced again unless hit again
        gb_ = false;
    }
    else
    {
        fighter_->vel_ = glm::vec2(0.f);
        fighter_->accel_ = glm::vec2(0.f);
        // Transition to the ground state
        return new GroundState(fighter_);
    }
    // No state change
    return NULL;
}

FighterState* AirStunnedState::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

//// ------------------------ GROUND STATE -------------------------
GroundState::GroundState(Fighter *f, float delay, float invincTime) :
    FighterState(f), jumpTime_(-1), dashTime_(-1), waitTime_(delay),
    dashing_(false), ducking_(false)
{
    frameName_ = "GroundNormal";
    fighter_->lastHitBy_ = -1;
    invincTime_ = invincTime;
}

GroundState::~GroundState()
{ /* Empty */ }

FighterState* GroundState::processInput(Controller &controller, float dt)
{
    // 'unduck' every frame

    ducking_ = false;
    // Update running timers
    if (jumpTime_ >= 0) jumpTime_ += dt;
    if (waitTime_ >= 0) waitTime_ -= dt;
    if (dashTime_ >= 0) dashTime_ += dt;
    else dashTime_ -= dt;
    // If the fighter is currently attacking, do nothing else
    if (fighter_->attack_) return NULL;
    // Do nothing during jump startup
    if (jumpTime_ > 0 && jumpTime_ < getParam("jumpStartupTime"))
        return NULL;
    // Do nothing during dash startup
    if (dashTime_ > 0 && dashTime_ < getParam("dashStartupTime"))
        return NULL;
    if (waitTime_ > 0)
        return NULL;

    // --- Deal with jump transition ---
    if (jumpTime_ > getParam("jumpStartupTime"))
    {
        // Set the xvelocity of the jump
        fighter_->vel_.x = fabs(controller.joyx) > getParam("input.deadzone") ?
            controller.joyx * 0.5 * getParam("dashSpeed") :
            0.0f;
        // If they are still "holding down" the jump button now, then full jump
        // otherwise short hop
        if (controller.jumpbutton || controller.joyy > getParam("input.jumpThresh"))
            fighter_->vel_.y = getParam("jumpSpeed");
        else
            fighter_->vel_.y = getParam("hopSpeed");
        // Draw a little puff
        ExplosionManager::get()->addPuff(
                fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.1f, 
                fighter_->pos_.y - fighter_->size_.y * 0.45f,
                0.3f);
        // Jump; transition to Air Normal
        return new AirNormalState(fighter_);
    }


    // -- Deal with starting an attack
    if (controller.pressa)
    {
        // Check for dash attack
        if (dashing_)
        {
            dashing_ = false;
            fighter_->vel_.x = fighter_->dir_ * getParam("dashSpeed");
            fighter_->attack_ = fighter_->attackMap_["dash"]->clone();
        }
        // Not dashing- use a tilt
        else
        {
            // No movement during attack
            fighter_->vel_ = glm::vec2(0.f);
            // Get direction of stick
            glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
            if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
            {
                // Do the L/R tilt
                fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
                fighter_->attack_ = fighter_->attackMap_["sideTilt"]->clone();
            }
            else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->attackMap_["downTilt"]->clone();
            }
            else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            {
                fighter_->attack_ = fighter_->attackMap_["upTilt"]->clone();
            }
            else
            {
                // Neutral tilt attack
                fighter_->attack_ = fighter_->attackMap_["neutralTilt"]->clone();
            }
        }
        // Do per attack stuff
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return NULL;
    }
    // Check for dodge
    else if ((controller.rbumper || controller.rtrigger < -getParam("input.trigThresh") || controller.ltrigger < -getParam("input.trigThresh"))
            && fabs(controller.joyx) > getParam("input.dodgeThresh")
            && fabs(controller.joyxv) > getParam("input.velThresh"))
    {
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
        ExplosionManager::get()->addPuff(
                fighter_->pos_.x + fighter_->size_.x * fighter_->dir_ * 0.4f, 
                fighter_->pos_.y - fighter_->size_.y * 0.45f,
                0.3f);
        return new DodgeState(fighter_);
    }
    // Check for block
    else if ((controller.rtrigger < -getParam("input.trigThresh")
           || controller.ltrigger < -getParam("input.trigThresh")))
    {
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
        fighter_->vel_ = glm::vec2(0.f);
        fighter_->accel_ = glm::vec2(0.f);

        AudioManager::get()->playSound("shieldon");
        return new BlockingState(fighter_);
    }
    // Check for B moves
    else if (controller.pressb)
    {
        FighterState *next = performBMove(controller);
        // In the ground state, make sure the player stops moving before
        // ANY B move
        fighter_->vel_.x = 0.f;
        dashing_ = false;
        return next;
    }
    // Check for taunt
    else if (controller.dpadu && !dashing_)
    {
        fighter_->attack_ = fighter_->attackMap_["taunt"]->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return NULL;
    }
    // Check for smash attacks
    else if (controller.pressc && !dashing_)
    {
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));
        // No movement during smash attacks
        fighter_->vel_ = glm::vec2(0.f);

        if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
        {
            fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
            fighter_->attack_ = fighter_->attackMap_["sideSmash"]->clone();
        }
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            fighter_->attack_ = fighter_->attackMap_["downSmash"]->clone();
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
            fighter_->attack_ = fighter_->attackMap_["upSmash"]->clone();
        else
            fighter_->attack_ = fighter_->attackMap_["neutralSmash"]->clone();

        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        return NULL;
    }

    // --- Deal with dashing movement ---
    if (dashing_ )
    {
        int newdir = controller.joyx < 0 ? -1 : 1;
        // Check for change of dash direction
        if (fighter_->dir_ != newdir && fabs(controller.joyxv) > getParam("input.velThresh") && fabs(controller.joyx) > getParam("input.dashMin"))
        {
            fighter_->dir_ = newdir;
            fighter_->vel_.x = 0;
            waitTime_ = getParam("dashChangeTime");
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.4f, 
                    fighter_->pos_.y - fighter_->size_.y * 0.45f,
                    0.3f);
            // Reset min dash time
            dashTime_ = -1 + std::min(getParam("dashChangeTime"), 0.99f);
        }
        // Check for drop out of dash
        else if (fabs(controller.joyx) < getParam("input.dashMin")
                && fabs(controller.joyxv) < getParam("input.velThresh")
                && dashTime_ < -1 - getParam("fighter.minDashTime"))
        {
            
            dashing_ = false;
            waitTime_ = getParam("dashChangeTime");
            fighter_->vel_.x = 0;
            ExplosionManager::get()->addPuff(
                    fighter_->pos_.x + fighter_->size_.x * fighter_->dir_ * 0.4f, 
                    fighter_->pos_.y - fighter_->size_.y * 0.45f,
                    0.3f);
        }
        // Otherwise just set the velocity
        else
        {
            fighter_->vel_.x = fighter_->dir_ * getParam("dashSpeed");
            // TODO add puffs every x amount of time
        }
    }
    // --- Deal with normal ground movement ---
    else
    {
        // Just move around a bit based on the controller
        if (fabs(controller.joyx) > getParam("input.deadzone"))
        {
            fighter_->vel_.x = controller.joyx * getParam("walkSpeed");
            fighter_->dir_ = controller.joyx < 0 ? -1 : 1;
        }
        // Only move when controller is held
        else
            fighter_->vel_ = glm::vec2(0.f);

        // --- Check for dashing startup complete ---
        if (dashTime_ > getParam("dashStartupTime"))
        {
            dashing_ = true;
            dashTime_ = -1;
        }
        // --- Check for dashing transition start
        else if (fabs(controller.joyx) > getParam("input.dashThresh")
                && fabs(controller.joyxv) > getParam("input.velThresh"))
        {
            dashTime_ = 0;
            fighter_->vel_.x = 0;
            fighter_->dir_ = controller.joyx < 0 ? -1 : 1;
            // Draw a little puff
            ExplosionManager::get()->addPuff(
                    fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.4f, 
                    fighter_->pos_.y - fighter_->size_.y * 0.45f,
                    0.3f);
        }

        if (controller.joyy < getParam("input.duckThresh"))
            ducking_ = true;
    }

    // --- Check to see if they want to start a jump ---
    if (controller.pressjump ||
            (controller.joyy > getParam("input.jumpThresh")
             && controller.joyyv > getParam("input.velThresh")))
    {
        // Start the jump timer
        jumpTime_ = 0.0f;
    }
    // No state change
    return NULL;
}

void GroundState::render(float dt)
{
    printf("GROUND | JumpT: %.3f  DashT: %.3f  WaitT: %.3f Duck: %d || ",
            jumpTime_, dashTime_, waitTime_, ducking_);

    std::string fname = frameName_;
    if (dashing_)
        fname = "GroundRunning";
    if (dashing_ && waitTime_ > 0.f)
        fname = "DashChange";
    if (ducking_)
        fname = "Ducking";
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    glm::vec3 color = fighter_->color_;
    if (invincTime_ > 0.f)
        color = muxByTime(color, invincTime_);
    fighter_->renderHelper(dt, fname, color);
}

FighterState* GroundState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision)
    {
        if (fighter_->attack_)
        {
            fighter_->attack_->cancel();
            dashing_ = false;
            float dir = (ground.x - fighter_->pos_.x) > 0 ? 1 : -1;
            fighter_->pos_.x += dir * (fabs(ground.x - fighter_->pos_.x) - ground.w/2 - fighter_->size_.x/2 + 2);
        }
        else
        {
            return new AirNormalState(fighter_);
        }
    }

    // If there is a collision, we don't need to do anything, because we're
    // already in the GroundState
    return NULL;
}

FighterState* GroundState::hitByAttack(const Attack *attack)
{
    // Then do the normal stuff
    return FighterState::calculateHitResult(attack);
}

Rectangle GroundState::getRect() const
{
    Rectangle r = FighterState::getRect();
    if (ducking_)
    {
        r.y -= r.h/4;
        r.h /= 2;
    }
    return r;
}

//// -------------------- AIR NORMAL STATE -----------------------------
BlockingState::BlockingState(Fighter *f) :
    FighterState(f), waitTime_(0.f), dazeTime_(0.f), hitStunTime_(0.f)
{
    frameName_ = "Blocking";
    waitTime_ = getParam("shield.startup");
}

BlockingState::~BlockingState()
{
    /* Empty */
}

FighterState* BlockingState::processInput(Controller &controller, float dt)
{
    // Update timers
    waitTime_ -= dt;
    dazeTime_ -= dt;
    hitStunTime_ -= dt;

    // Don't do anything while waiting or dazed or stunned
    if (waitTime_ > 0.f) return NULL;
    if (dazeTime_ > 0.f) return NULL;
    if (hitStunTime_ > 0.f) return NULL;

    // Check for drop out of blocking state
    if (controller.rtrigger > -getParam("input.trigThresh")
     && controller.ltrigger > -getParam("input.trigThresh"))
    {
        return new GroundState(fighter_, getParam("shield.cooldown"));
    }
    // Check for dodge
    else if (fabs(controller.joyx) > getParam("input.dodgeThresh")
          && fabs(controller.joyxv) > getParam("input.velThresh"))
    {
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
        ExplosionManager::get()->addPuff(
                fighter_->pos_.x + fighter_->size_.x * fighter_->dir_ * 0.4f, 
                fighter_->pos_.y - fighter_->size_.y * 0.45f,
                0.3f);
        return new DodgeState(fighter_);
    }

    // Eat the degeneration
    fighter_->shieldHealth_ += getParam("shield.degen") * dt;

    // Check to see if they are out of shield
    if (fighter_->shieldHealth_ < 0.f)
    {
        dazeTime_ = getParam("shield.dazeTime");
        fighter_->shieldHealth_ = 0.f;
        AudioManager::get()->playSound("shieldshatter");
    }

    return NULL;
}

void BlockingState::render(float dt)
{
    printf("BLOCKING | waitT: %.3f dazeT: %.3f health: %.3f || ",
            waitTime_, dazeTime_, fighter_->shieldHealth_);

    glm::vec3 color = fighter_->color_;
    std::string fname = frameName_;
    if (dazeTime_ > 0.f)
    {
        fname = "Dazed";
        color = muxByTime(color, dazeTime_);
    }
    fighter_->renderHelper(dt, fname, color);


    // Draw shield
    if (!(waitTime_ > 0.f || dazeTime_ > 0.f))
    {
        glm::vec4 color(0.4f, 0.0f, 0.33f, 0.4f);
        float sizeFact = std::max(fighter_->shieldHealth_ / getParam("shield.maxHealth"), 0.f);
        glm::mat4 transform = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(fighter_->pos_, 0.1f)),
                sizeFact * glm::vec3(getParam("shield.size"), getParam("shield.size"), 1.f));

        if (hitStunTime_ > 0.f)
            color = muxByTime(color, hitStunTime_);
        renderRectangle(transform, color);
    }
}

template<typename T>
T FighterState::muxByTime(const T& color, float t)
{
    float period_scale_factor = 20.0;
    float opacity_amplitude = 3;
    float opacity_factor = (1 + cos(period_scale_factor * t)) * 0.5f; 
    return color * (opacity_amplitude * opacity_factor + 1);
}


FighterState* BlockingState::collisionWithGround(const Rectangle &ground, bool collision)
{
    return NULL;
}

FighterState* BlockingState::hitByAttack(const Attack *attack)
{
    // Draw an explosion no matter what
    glm::vec2 hitdir = glm::vec2(fighter_->pos_.x, fighter_->pos_.y)
        - glm::vec2(attack->getHitbox().x, attack->getHitbox().y);
    hitdir = glm::normalize(hitdir);
    float exx = -hitdir.x * fighter_->size_.x / 2 + fighter_->pos_.x;
    float exy = -hitdir.y * fighter_->size_.y / 2 + fighter_->pos_.y;
    ExplosionManager::get()->addExplosion(exx, exy, 0.2);

    // Only block if we're actively blocking right now
    if (waitTime_ > 0.f || dazeTime_ > 0.f)
    {
        // Get hit
        fighter_->pos_.y += 4;
        return FighterState::calculateHitResult(attack);
    }
    else
    {
        // block it and take shield damage
        fighter_->shieldHealth_ -= attack->getDamage(fighter_);
        // Experience some hit stun
        hitStunTime_ = getParam("shield.stunFactor")
            * glm::length(attack->getKnockback(fighter_));
        AudioManager::get()->playSound("shieldhit");
    }

    return NULL;
}


//// -------------------- AIR NORMAL STATE -----------------------------
AirNormalState::AirNormalState(Fighter *f) :
    FighterState(f), canSecondJump_(true), jumpTime_(-1), noGrabTime_(0)
{
    frameName_ = "AirNormal";
}

AirNormalState::~AirNormalState()
{ /* Empty */ }

FighterState* AirNormalState::processInput(Controller &controller, float dt)
{
    // Gravity
    fighter_->accel_ = glm::vec2(0.f, getParam("airAccel"));

    // Update running timers
    if (jumpTime_ >= 0) jumpTime_ += dt;
    noGrabTime_ -= dt;
    // If the fighter is currently attacking, do nothing else
    if (fighter_->attack_) return NULL;

    // Let them control the character slightly
    if (fabs(controller.joyx) > getParam("input.deadzone"))
    {
        // Don't let the player increase the velocity past a certain speed
        if (fighter_->vel_.x * controller.joyx <= 0 || fabs(fighter_->vel_.x) < getParam("jumpAirSpeed"))
            fighter_->vel_.x += controller.joyx * getParam("airForce") * dt;
        fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
    }
    // Fast falling
    if (fighter_->vel_.y < 0 && fighter_->vel_.y > getParam("fastFallMaxSpeed") && controller.joyy < getParam("input.fallThresh"))
    {
        if (!fastFalling_ && fighter_->vel_.y > getParam("fastFallMaxInitialSpeed"))
            fighter_->vel_.y += getParam("fastFallInitialSpeed");
        else
            fighter_->accel_ += glm::vec2(0.f, getParam("fastFallAccel"));
        fastFalling_ = true;
    }

    // --- Check for attack ---
    if (controller.pressa)
    {
        // Get direction of stick
        glm::vec2 tiltDir = glm::normalize(glm::vec2(controller.joyx, controller.joyy));

        if (fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
        {
            // side aerial
            fighter_->dir_ = controller.joyx > 0 ? 1 : -1;
            fighter_->attack_ = fighter_->attackMap_["airFront"]->clone();
        }
        /*
        else if (tiltDir.x * fighter_->dir_ < 0 && fabs(controller.joyx) > getParam("input.tiltThresh") && fabs(tiltDir.x) > fabs(tiltDir.y))
            // Do the back
            fighter_->attack_ = fighter_->attackMap_["airBack"]->clone();
        */
        else if (controller.joyy < -getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->attackMap_["airDown"]->clone();
        }
        else if (controller.joyy > getParam("input.tiltThresh") && fabs(tiltDir.x) < fabs(tiltDir.y))
        {
            fighter_->attack_ = fighter_->attackMap_["airUp"]->clone();
        }
        else
        {
            // Neutral tilt attack
            fighter_->attack_ = fighter_->attackMap_["airNeutral"]->clone();
        }
        // Do stuff to all attacks
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
    }
    else if (controller.pressb)
    {
        // No additional processing is required here.
        return performBMove(controller, true);
    }

    // If we just added an attack, lets not jump too
    if (fighter_->attack_)
    {
        jumpTime_ = -1;
        return NULL;
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
        fighter_->vel_.x = fabs(controller.joyx) > getParam("input.deadzone") ?
            0.8f * getParam("dashSpeed") * std::max(-1.0f, std::min(1.0f, (controller.joyx - 0.2f) / 0.6f)) :
            0.0f;
        fighter_->vel_.y = getParam("secondJumpSpeed");
        jumpTime_ = -1;
        canSecondJump_ = false;
        // Draw a puff
        ExplosionManager::get()->addPuff(
                fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.1f, 
                fighter_->pos_.y - fighter_->size_.y * 0.45f,
                0.3f);
    }
    // --- Check for ledge grab ---
    if (noGrabTime_ <= 0.f)
    {
        FighterState* next = checkForLedgeGrab();
        if (next) return next;
    }

    return NULL;
}

void AirNormalState::update(float dt)
{
    FighterState::update(dt);
}

void AirNormalState::render(float dt)
{
    printf("AIR NORMAL | JumpT: %.3f  Can2ndJump: %d || ",
            jumpTime_, canSecondJump_);
    std::string fname = frameName_;
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();
    fighter_->renderHelper(dt, fname, fighter_->color_);
}

FighterState* AirNormalState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // If no collision, we don't care
    if (!collision)
        return NULL;
    FighterState::collisionHelper(ground);
    // If we're not overlapping the ground anymore, no collision
    if (!ground.overlaps(fighter_->getRect()))
        return NULL;

    // Reset vel/accel
    fighter_->vel_ = glm::vec2(0.f, 0.f);
    fighter_->accel_ = glm::vec2(0.f);

    // Cancel any attack if it exists
    if (fighter_->attack_)
        fighter_->attack_->cancel();

    // Draw some puffs for landing
    ExplosionManager::get()->addPuff(
            fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.4f, 
            fighter_->pos_.y - fighter_->size_.y * 0.45f,
            0.3f);
    ExplosionManager::get()->addPuff(
            fighter_->pos_.x + fighter_->size_.x * fighter_->dir_ * 0.4f, 
            fighter_->pos_.y - fighter_->size_.y * 0.45f,
            0.3f);

    // Transition to the ground state, with a small delay for landing
    return new GroundState(fighter_, getParam("landingCooldownTime"));
}

FighterState* AirNormalState::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

AirNormalState * AirNormalState::setNoGrabTime(float t)
{
    noGrabTime_ = t;
    return this;
}


//// ----------------------- COUNTER STATE --------------------------------

CounterState::CounterState(Fighter *f, bool ground) :
    FighterState(f), t_(0), ground_(ground), pre_("counterSpecial."),
    playedSound_(false)
{
    frameName_ = "Counter";
}

FighterState* CounterState::processInput(Controller & controller, float dt)
{
    t_ += dt;
    float totalT = getParam(pre_ + "startup") +
        getParam(pre_ + "duration") + 
        getParam(pre_ + "cooldown");

    if (!fighter_->attack_ && t_ > totalT)
    {
        // Move to the next state
        if (ground_)
            return new GroundState(fighter_);
        else 
            return new AirNormalState(fighter_);
    }
    if (t_ > getParam(pre_ + "startup") + getParam(pre_ + "duration")
            && !playedSound_)
    {
        playedSound_ = true;
        AudioManager::get()->playSound("counterhit");
    }

    return NULL;
}

FighterState* CounterState::hitByAttack(const Attack* attack)
{
    // Invincibility during the counter!
    if (fighter_->attack_)
        return NULL;

    // If counter isn't ready yet (or it's too late), eat the attack
    if (t_ < getParam(pre_ + "startup") || 
            t_ > getParam(pre_ + "startup") + getParam(pre_ + "duration"))
    {
        return calculateHitResult(attack);
    }

    // Now the other player gets screwed over for attacking us at the wrong time.
    // Otherwise create a new Fighter attack helper.
    fighter_->attack_ = new FighterAttack("counterAttack", "groundhit", "CounterAttack");
    fighter_->attack_->setHitboxFrame("CounterAttackHitbox");
    fighter_->attack_->setFighter(fighter_);
    fighter_->attack_->start();
    fighter_->dir_ = attack->getHitbox().x - fighter_->pos_.x > 0 ? 1 : -1;

    return NULL;
}

FighterState* CounterState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (collision)
    {
        collisionHelper(ground);
        fighter_->vel_ =  glm::vec2(0.f);
        fighter_->accel_ = glm::vec2(0.f);
        ground_ = true;
    }
    else
    {
        // No collision, fall
        ground_ = false;
        fighter_->accel_ = glm::vec2(0.f, getParam("airAccel"));
    }
    // No state change
    return NULL;
}

void CounterState::render(float dt)
{
    printf("COUNTER | t: %.3f  ground: %d || ",
            t_, ground_);

    std::string fname = frameName_;
    if (fighter_->attack_)
        fname = fighter_->attack_->getFrameName();

    glm::vec3 color = fighter_->color_;
    if (t_ > getParam(pre_ + "startup")
            && t_ < getParam(pre_ + "startup")
                + getParam(pre_ + "duration"))
                
    {
        color = muxByTime(color, t_);
        color = glm::vec3(0.8f, 0.8f, 0.8f);
    }

    fighter_->renderHelper(dt, fname, color);
}

//// -------------------- UP SPECIAL STATE ------------------------------
UpSpecialState::UpSpecialState(Fighter *f) :
    FighterState(f)
{
    pre_ = "upSpecialAttack.";
    fighter_->attack_ = fighter_->attackMap_["upSpecial"]->clone();
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();

    fighter_->push(glm::vec2(0, 2));
}

FighterState * UpSpecialState::processInput(Controller &, float dt)
{
    if (!fighter_->attack_)
        return new AirStunnedState(fighter_, HUGE_VAL);

    if (fighter_->attack_->hasHitbox())
    {
        fighter_->vel_.x = fighter_->dir_ * getParam(pre_ + "xvel");
        fighter_->vel_.y = getParam(pre_ + "yvel");
    }

    return NULL;
}

void UpSpecialState::render(float dt)
{
    printf("UP SPECIAL | || ");
    assert(fighter_->attack_);
    fighter_->renderHelper(dt, fighter_->attack_->getFrameName(), fighter_->getColor());
}

FighterState* UpSpecialState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // XXX not sure if this is right
    return NULL;
}

FighterState* UpSpecialState::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

//// ----------------------- DODGE STATE --------------------------------
DodgeState::DodgeState(Fighter *f) :
    FighterState(f), t_(0.f), dodgeTime_(getParam("dodge.duration")),
    cooldown_(getParam("dodge.cooldown"))
{
    invincTime_ = getParam("dodge.invincTime");
    frameName_ = "GroundRoll";
}

FighterState* DodgeState::processInput(Controller &controller, float dt)
{
    t_ += dt;
    fighter_->vel_.x = fighter_->dir_ * getParam("dodge.speed");
    
    if (t_ > dodgeTime_)
        fighter_->vel_.x = 0;

    if (t_ > dodgeTime_ && t_ - dt < dodgeTime_)
        fighter_->dir_ = -fighter_->dir_;

    if (t_ > dodgeTime_ + cooldown_)
        return new GroundState(fighter_);
    // No state change
    return NULL;
}

void DodgeState::render(float dt)
{
    glm::vec3 color = fighter_->color_;

    float angle = t_ < dodgeTime_ ? t_ / dodgeTime_ * 360 : 0.f;

    if (invincTime_ > 0.f)
        color = muxByTime(fighter_->color_, t_);

    printf("DODGE | t: %.3f  invincT: %.3f  dodgeT: %.3f || ",
            t_, invincTime_, dodgeTime_);
    // Just render the fighter, but flashing
    fighter_->renderHelper(dt, frameName_, color,
            glm::rotate(glm::mat4(1.f), -angle, glm::vec3(0,0,1)));
}

FighterState* DodgeState::hitByAttack(const Attack *attack)
{
    // Then do the normal stuff
    return FighterState::calculateHitResult(attack);
}

FighterState* DodgeState::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision)
    {
        float dir = (ground.x - fighter_->pos_.x) > 0 ? 1 : -1;
        fighter_->pos_.x += dir * (fabs(ground.x - fighter_->pos_.x) - ground.w/2 - fighter_->size_.x/2 + 2);
        fighter_->vel_.x = 0.0f;
        t_ = std::max(t_, invincTime_);
    }

    return NULL;
}

//// ---------------------- LEDGE GRAB STATE ------------------------------
LedgeGrabState::LedgeGrabState(Fighter *f) :
    FighterState(f),
    hbsize_(getParam("ledgeGrab.hbwidth"), getParam("ledgeGrab.hbheight")),
    
    jumpTime_(HUGE_VAL),
    ledge_(NULL)
{
    invincTime_ = getParam("ledgeGrab.grabInvincTime");
}

FighterState* LedgeGrabState::processInput(Controller &controller, float dt)
{
    if (jumpTime_ > 0 && jumpTime_ != HUGE_VAL) return NULL;

    // Check for jump transition
    if (jumpTime_ <= 0.f)
    {
        fighter_->vel_ = glm::vec2(0.f,
                controller.jumpbutton ? getParam("jumpSpeed") : getParam("hopSpeed"));
        // Unoccupy
        ledge_->occupied = false;

        // Draw a little puff
        ExplosionManager::get()->addPuff(
                fighter_->pos_.x - fighter_->size_.x * fighter_->dir_ * 0.1f, 
                fighter_->pos_.y - fighter_->size_.y * 0.45f,
                0.3f);
        // Transition
        return new AirNormalState(fighter_);
    }

    // Check for jump input
    if (controller.pressjump)
    {
        // Start startup time countdown
        jumpTime_ = getParam("jumpStartupTime");
    }
    // Check for attack input
    else if (controller.pressa)
    {
        // TODO add a real ledge grab attack
        ledge_->occupied = false;
        // move on top of ground
        fighter_->push(glm::vec2(
                    fighter_->getDirection() * hbsize_.x/2,
                    fighter_->size_.y - 1));
        // start ledge attack XXX: just use down tilt for now
        fighter_->attack_ = fighter_->attackMap_["downTilt"]->clone();
        fighter_->attack_->setFighter(fighter_);
        fighter_->attack_->start();
        // transition to groundnormal state with some invincibility
        return new GroundState(fighter_, -1.f, getParam("ledgeGrab.attackInvinc"));
    }
    // Check for fall
    else if (controller.joyy < getParam("input.dropThresh"))
    {
        ledge_->occupied = false;
        // transition to air normal state with some no grab time
        return (new AirNormalState(fighter_))
            ->setNoGrabTime(getParam("ledgeGrab.dropTime"));
    }
    // Check for dodge
    else if (controller.rtrigger < -getParam("input.trigThresh") ||
             controller.ltrigger < -getParam("input.trigThresh"))
    {
        ledge_->occupied = false;
        // move on top of ground
        fighter_->push(glm::vec2(
                    fighter_->getDirection() * hbsize_.x/2,
                    fighter_->size_.y - 1));
        // put in dodge state with some delay
        return new DodgeState(fighter_);
    }
    // Check for plain stand up (up or forward on the joystick)
    else if ((controller.joyx * fighter_->getDirection() > getParam("input.tiltThresh") 
                && controller.joyxv * fighter_->getDirection() > getParam("input.velThresh"))
            || (controller.joyy > getParam("input.tiltThresh")
                && controller.joyyv > getParam("input.velThresh")))
    {
        ledge_->occupied = false;
        // move on top of ground
        fighter_->push(glm::vec2(
                    fighter_->getDirection() * hbsize_.x/2,
                    fighter_->size_.y - 1));
        // put in GroundNormal state
        return new GroundState(fighter_, getParam("ledgeGrab.wakeupTime"),
                getParam("ledgeGrab.wakeUpInvinc"));
    }

    // No state change
    return NULL;
}

void LedgeGrabState::update(float dt)
{
    FighterState::update(dt);
    // Just update timers
    jumpTime_ -= dt;
}

void LedgeGrabState::render(float dt)
{
    printf("LEDGE | jumpT: %.3f invincT: %.3f || ",
            jumpTime_, invincTime_);
    glm::vec3 color = fighter_->color_;

    // flash when invincible
    if (invincTime_ > 0)
        color = muxByTime(color, invincTime_);

    fighter_->renderHelper(dt, "LedgeGrab", color);
}

FighterState* LedgeGrabState::collisionWithGround(const Rectangle &ground, bool collision)
{
    // Empty
    return NULL;
}

FighterState* LedgeGrabState::hitByAttack(const Attack *attack)
{
    assert(invincTime_ <= 0.f);
    ledge_->occupied = false;
    return FighterState::calculateHitResult(attack);
}

bool LedgeGrabState::canBeHit() const
{
    return invincTime_ <= 0.f;
}

Rectangle LedgeGrabState::getRect() const
{
    return Rectangle(fighter_->pos_.x, fighter_->pos_.y,
            hbsize_.x, hbsize_.y);
}

void LedgeGrabState::grabLedge(Ledge *l)
{
    ledge_ = l;
    ledge_->occupied = true;

    float xdir = ledge_->pos.x > fighter_->pos_.x ? 1 : -1;
    // Move our top corner to their top corner
    fighter_->pos_.y = ledge_->pos.y - hbsize_.y / 2;
    fighter_->pos_.x = ledge_->pos.x - xdir * hbsize_.x / 2;

    fighter_->vel_ = glm::vec2(0.f);
    fighter_->accel_ = glm::vec2(0.f);

    fighter_->dir_ = xdir;

    std::cout << "Pos: " << fighter_->pos_.x << ' ' << fighter_->pos_.y
        << " Size: " << hbsize_.x << ' ' << hbsize_.y
        << " LedgePos: " << l->pos.x << ' ' << l->pos.y << '\n';
}



//// ----------------------- RESPAWN STATE --------------------------------
RespawnState::RespawnState(Fighter *f) :
    FighterState(f), t_(0.f)
{
}

FighterState* RespawnState::processInput(Controller &controller, float dt)
{
    t_ += dt;
    if (t_ > getParam("fighter.respawnTime"))
        return new AirNormalState(fighter_);
    return NULL;
}

void RespawnState::render(float dt)
{
    printf("RESPAWN | t: %f || ", t_);
    // Just render the fighter, but slightly lighter
    fighter_->renderHelper(dt, frameName_, 1.6f * fighter_->color_);
}

FighterState* RespawnState::hitByAttack(const Attack *)
{
    // Do nothing, we're invincible bitch!
    return NULL;
}

bool RespawnState::canBeHit() const
{
    return false;
}

FighterState* RespawnState::collisionWithGround(const Rectangle &ground, bool collision)
{
    assert(!collision);
    return NULL;
}

//// ------------------------- DEAD STATE ----------------------------------
DeadState::DeadState(Fighter *f) :
    FighterState(f)
{
    f->pos_.x = HUGE_VAL;
    f->pos_.y = HUGE_VAL;
}

FighterState* DeadState::collisionWithGround(const Rectangle &, bool)
{
    return NULL;
}

FighterState* DeadState::hitByAttack(const Attack *)
{
    assert(false);
    return NULL;
}

bool DeadState::canBeHit() const
{
    return false;
}

FighterState* DeadState::processInput(Controller &controller, float dt)
{
    // NOTHING
    return NULL;
}

