#include "StickmanStates.h"
#include "ExplosionManager.h"
#include "Controller.h"
#include "Attack.h"
#include "ParamReader.h"
#include "AudioManager.h"

// --------------------------------------------
// - Up Special Attack
// --------------------------------------------

StickmanUpSpecial::StickmanUpSpecial(Fighter *f, bool ground) :
    SpecialState(f, ground),
    pre_("upSpecialAttack."),
    dir_(0.f)
{
    frameName_ = "UpSpecial";

    fighter_->attack_ = fighter_->attackMap_["upSpecial"]->clone();
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();
}

StickmanUpSpecial::~StickmanUpSpecial()
{
}

FighterState* StickmanUpSpecial::processInput(controller_state &cs, float dt)
{
    // Are we done?
    if (!fighter_->attack_)
    {
        if (ground_)
        {
            fighter_->vel_ = glm::vec2(0.f);
            return new GroundState(fighter_);
        }
        else
        {
            // Keep some momentum
            fighter_->vel_ = dir_ * fighter_->param(pre_ + "momentum");
            // After up special leave them stunned
            return new AirStunnedState(fighter_, HUGE_VAL);
        }
    }
    // Maybe the attack is active?
    else if (fighter_->hasAttack())
    {
        if (dir_ == glm::vec2(0.f))
        {
            // Calculate teleport direction, default to straight up
            glm::vec2 joypos = glm::vec2(cs.joyx, cs.joyy);
            dir_ = glm::normalize(glm::vec2(cs.joyx, cs.joyy));
            if (glm::length(joypos) < getParam("input.tiltThresh"))
                dir_ = glm::vec2(0, 1);
            // Set facing too
            if (cs.joyx > getParam("input.deadzone"))
                fighter_->dir_ = glm::sign(cs.joyx);
        }
        // Move them
        fighter_->vel_ = dir_ * fighter_->param(pre_ + "speed");
        // No more acceleration
        fighter_->accel_ = glm::vec2(0.f);
    }
    // startup or cooldown
    else
    {
        // never any cooldown, set to 0
        fighter_->vel_ = glm::vec2(0.f);
        fighter_->accel_ = glm::vec2(0.f);
    }

    // Grab during attack
    return checkForLedgeGrab(true);
}

FighterState* StickmanUpSpecial::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    if (collision)
        fighter_->attack_->cancel();
    return SpecialState::collisionWithGround(ground, collision, platform);
}

FighterState* StickmanUpSpecial::hitByAttack(const Attack *attack)
{
    return calculateHitResult(attack);
}

void StickmanUpSpecial::render(float dt)
{
    fighter_->renderHelper(dt, fighter_->color_);
}

// --------------------------------------------
// - Neutral Special Attack
// --------------------------------------------

StickmanNeutralSpecial::StickmanNeutralSpecial(Fighter *f, bool ground) :
    SpecialState(f, ground),
    pre_("neutralSpecialState.")
{
    frameName_ = "NeutralSpecial";

    // TODO set starting and hitting sfx here, maybe hitbox frame
    fighter_->attack_ = fighter_->loadAttack<FighterAttack>("neutralSpecialAttack",
            "", "NeutralSpecial");
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();
}

StickmanNeutralSpecial::~StickmanNeutralSpecial()
{
}

FighterState* StickmanNeutralSpecial::processInput(controller_state &cs, float dt)
{
    if (!fighter_->attack_)
    {
        if (ground_)
        {
            fighter_->vel_ = glm::vec2(0.f);
            fighter_->accel_ = glm::vec2(0.f);
            return new GroundState(fighter_);
        }
        else
        {
            return new AirNormalState(fighter_);
        }
    }

    // No transition
    return NULL;
}

FighterState* StickmanNeutralSpecial::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    return SpecialState::collisionWithGround(ground, collision, platform);
}

FighterState* StickmanNeutralSpecial::hitByAttack(const Attack *attack)
{
    return calculateHitResult(attack);
}

void StickmanNeutralSpecial::render(float dt)
{
    fighter_->renderHelper(dt, fighter_->color_);
}


// --------------------------------------------
// - Counter (Down) Special Attack
// --------------------------------------------

CounterState::CounterState(Fighter *f, bool ground) :
    SpecialState(f, ground), t_(0), pre_("counterSpecial."),
    playedSound_(false)
{
    frameName_ = "Counter";
}

FighterState* CounterState::processInput(controller_state & controller, float dt)
{
    t_ += dt;
    float totalT = fighter_->param(pre_ + "startup") +
        fighter_->param(pre_ + "duration") + 
        fighter_->param(pre_ + "cooldown");

    if (!fighter_->attack_ && t_ > totalT)
    {
        // Move to the next state
        if (ground_)
            return new GroundState(fighter_);
        else 
            return new AirNormalState(fighter_);
    }
    if (t_ > fighter_->param(pre_ + "startup") + fighter_->param(pre_ + "duration")
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
    if (t_ < fighter_->param(pre_ + "startup") || 
            t_ > fighter_->param(pre_ + "startup") + fighter_->param(pre_ + "duration"))
    {
        return calculateHitResult(attack);
    }

    float calcedpow = fighter_->param("counterAttack.reflectfact") *
        glm::length(attack->calcKnockback(fighter_, fighter_->getDamage()));

    // Now the other player gets screwed over for attacking us at the wrong time.
    // Otherwise create a new Fighter attack helper.
    fighter_->attack_ = new FighterAttack(fighter_->pre_ + "counterAttack", "groundhit", "CounterAttack");
    fighter_->attack_->setHitboxFrame("CounterAttackHitbox");
    fighter_->attack_->setFighter(fighter_);
    fighter_->attack_->start();
    fighter_->attack_->setBaseKnockback(calcedpow);
    fighter_->dir_ = attack->getOriginDirection(fighter_);

    return NULL;
}

FighterState* CounterState::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    return SpecialState::collisionWithGround(ground, collision, platform);
}

void CounterState::render(float dt)
{
    glm::vec3 color = fighter_->color_;
    if (t_ > fighter_->param(pre_ + "startup")
            && t_ < fighter_->param(pre_ + "startup")
                + fighter_->param(pre_ + "duration"))
                
    {
        color = muxByTime(color, t_);
        color = glm::vec3(0.8f, 0.8f, 0.8f);
    }

    fighter_->renderHelper(dt, color);
}

// --------------------------------------------
// - Side Special Attack - Cape
// --------------------------------------------

StickmanSideSpecial::StickmanSideSpecial(Fighter *f, bool ground) :
    SpecialState(f, ground),
    pre_("sideSpecialState."),
    pushed_(false)
{
    frameName_ = "SideSpecial";

    // TODO set starting and hitting sfx here, maybe hitbox frame
    fighter_->attack_ = fighter_->loadAttack<FighterAttack>("sideSpecialAttack",
            "", "SideSpecial");
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();
}

StickmanSideSpecial::~StickmanSideSpecial()
{
}

FighterState* StickmanSideSpecial::processInput(controller_state &cs, float dt)
{
    if (fighter_->hasAttack() && !pushed_)
    {
        fighter_->vel_.x = fighter_->dir_ * fighter_->param(pre_ + "velx");
        fighter_->accel_.x = fighter_->dir_ * fighter_->param(pre_ + "accelx");

        // only give vertical velocity if in air:w
        if (!ground_)
            fighter_->vel_.y = fighter_->param(pre_ + "vely");

        // Only add velocity
        pushed_ = true;
    }

    if (!fighter_->attack_)
    {
        if (ground_)
        {
            fighter_->vel_ = glm::vec2(0.f);
            fighter_->accel_ = glm::vec2(0.f);
            return new GroundState(fighter_);
        }
        else
        {
            return new AirNormalState(fighter_);
        }
    }

    // No transition
    return NULL;
}

FighterState* StickmanSideSpecial::hitByAttack(const Attack *attack)
{
    return calculateHitResult(attack);
}

void StickmanSideSpecial::render(float dt)
{
    fighter_->renderHelper(dt, fighter_->color_);
}

FighterState* StickmanSideSpecial::attackConnected(GameEntity *victim)
{
    // Turn the victim around
    victim->reflect();
    // don't hit them multiple times
    fighter_->attack_->hit(victim);

    if (victim->getType() == "Projectile")
    {
        // Take ownership of the projectile
        victim->reown(fighter_->playerID_, fighter_->teamID_);
    }
    else if (victim->getType() == Fighter::type)
    {
        Fighter *fvic = (Fighter *)victim;
        // TODO this damage is not recorded for stats, fix that!
        fvic->damage_ += fighter_->attack_->getDamage();
    }

    return NULL;
}
