#include "StickmanStates.h"
#include "ExplosionManager.h"
#include "Controller.h"
#include "Attack.h"
#include "ParamReader.h"

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
    if (fighter_->hasAttack())
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

    // Grab during attack
    return checkForLedgeGrab(true);
}

FighterState* StickmanUpSpecial::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
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

    // TODO set sfx here, maybe hitbox frame
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
    if (fighter_->hasAttack())
        return NULL;

    if (!fighter_->attack_)
    {
        // Jump when attack is over, if we can
        if (!fighter_->airData_["specialJumped"])
        {
            // Set direction to be based on joystick, straight up if no input
            glm::vec2 dir = glm::normalize(glm::vec2(cs.joyx, 1));
            if (fabs(cs.joyx) < getParam("input.tiltThresh"))
                dir = glm::vec2(0, 1);
            fighter_->vel_ = dir * fighter_->param(pre_ + "speed");
            if (cs.joyx > getParam("input.deadzone"))
                fighter_->dir_ = glm::sign(cs.joyx);
            // Can only special jump once
            fighter_->airData_["specialJumped"] = 1;
        }

        // After special jump leave them w/o a second jump
        return (new AirNormalState(fighter_));
    }

    // No transition
    return NULL;
}

FighterState* StickmanNeutralSpecial::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    if (collision)
        fighter_->attack_->cancel();
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
