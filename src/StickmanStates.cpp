#include "StickmanStates.h"
#include "ExplosionManager.h"
#include "Controller.h"
#include "Attack.h"

// --------------------------------------------
// - Up Special Attack
// --------------------------------------------

StickmanUpSpecial::StickmanUpSpecial(Fighter *f, bool ground) :
    SpecialState(f, ground),
    pre_("upSpecialAttack."),
    teleported_(false)
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
    if (fighter_->hasAttack() && !teleported_)
    {
        // Add a big puff at source location
        ExplosionManager::get()->addPuff(fighter_->pos_, fighter_->size_.x/2, 0.5f);

        // Calculate teleport direction
        glm::vec2 dir = glm::normalize(glm::vec2(cs.joyx, cs.joyy));
        // Teleport them
        fighter_->pos_ += dir * fighter_->param(pre_ + "dist");
        fighter_->vel_ = dir * fighter_->param(pre_ + "momentum");
        fighter_->accel_ = glm::vec2(0.f);
        fighter_->dir_ = glm::sign(dir.x);

        // Add a another big puff at destination
        ExplosionManager::get()->addPuff(fighter_->pos_, fighter_->size_.x/2, 0.4f);

        teleported_ = true;
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
    pre_("neutralSpecialAttack.")
{
    frameName_ = "NeutralSpecial";

    fighter_->attack_ = fighter_->attackMap_["neutralSpecial"]->clone();
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
        // Jump when attack is over
        glm::vec2 dir = glm::normalize(glm::vec2(cs.joyx, cs.joyy));
        fighter_->vel_ = dir * fighter_->param(pre_ + "speed");
        fighter_->dir_ = glm::sign(dir.x);

        if (ground_)
        {
            fighter_->vel_ = glm::vec2(0.f);
            return new GroundState(fighter_);
        }
        else
        {
            // After special jump leave them w/o a second jump
            return (new AirNormalState(fighter_, HUGE_VAL))->disableSecondJump();
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
