#include "CharlieStates.h"
#include "Attack.h"
#include "Projectile.h"
#include "AudioManager.h"

void addEntity(GameEntity *ent);

//// -------------------- UP SPECIAL STATE ------------------------------
UpSpecialState::UpSpecialState(Fighter *f, bool ground) :
    SpecialState(f, ground)
{
    pre_ = "upSpecialAttack.";
    fighter_->attack_ = fighter_->attackMap_["upSpecial"]->clone();
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();

    fighter_->push(glm::vec2(0, 2));
}

FighterState * UpSpecialState::processInput(controller_state &, float dt)
{
    if (!fighter_->attack_)
        return new AirStunnedState(fighter_, HUGE_VAL);

    // Only boost while the hitbox is active
    if (fighter_->attack_->hasHitbox())
    {
        fighter_->vel_.x = fighter_->dir_ * fighter_->param(pre_ + "xvel");
        fighter_->vel_.y = fighter_->param(pre_ + "yvel");
    }

    // Grab during attack
    return checkForLedgeGrab(true);
}

void UpSpecialState::render(float dt)
{
    /*
    printf("UP SPECIAL | || ");
    */
    assert(fighter_->attack_);
    fighter_->renderHelper(dt, fighter_->getColor());
}

FighterState* UpSpecialState::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    return SpecialState::collisionWithGround(ground, collision, platform);
}

FighterState* UpSpecialState::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

FighterState* UpSpecialState::attackConnected(GameEntity *victim)
{
    // XXX this can create problems...
    victim->push(glm::vec2(0, 20));
    return FighterState::attackConnected(victim);
}


//// ------------------- DASH SPECIAL STATE -----------------------------
DashSpecialState::DashSpecialState(Fighter *f, bool ground) :
    SpecialState(f, ground)
{
    pre_ = "dashSpecialState.";
    fighter_->attack_ = fighter_->attackMap_["dashSpecial"]->clone();
    fighter_->attack_->setFighter(f);
    fighter_->attack_->start();
}

FighterState * DashSpecialState::processInput(controller_state &, float dt)
{
    // Check for transition away
    if (!fighter_->attack_)
    {

        if (ground_)
        {
            fighter_->vel_.y = 0.f;
            // Clear x velocity
            fighter_->vel_.x = 0.f;
            return new GroundState(fighter_);
        }
        else
        {
            return new AirNormalState(fighter_);
        }
    }

    // True is for being able to grab during attack
    return checkForLedgeGrab(true);
}

void DashSpecialState::render(float dt)
{
    /*
    printf("DASH SPECIAL | ground: %d || ",
            ground_);
            */
    assert(fighter_->attack_);
    fighter_->renderHelper(dt, fighter_->getColor());
}

void DashSpecialState::update(float dt)
{
    // Only boost while the hitbox is active
    if (fighter_->attack_ && fighter_->attack_->hasHitbox())
    {
        fighter_->vel_.x = fighter_->dir_ * fighter_->param(pre_ + "xvel");
        fighter_->vel_.y = 0.f;
        // TODO figure out what to do with the yvel
        /*
        if (!ground_)
            fighter_->vel_.y = getParam(pre_ + "yvel");
            */
    }
    else if (fighter_->attack_ && !ground_)
        fighter_->vel_.x = fighter_->param(pre_ + "endxvel") * fighter_->dir_;
    else
        fighter_->vel_.x = 0.f;

    SpecialState::update(dt);
}

FighterState* DashSpecialState::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    float xvel = fighter_->vel_.x;
    FighterState *ret = SpecialState::collisionWithGround(ground, collision, platform);
    fighter_->vel_.x = xvel;
    return ret;
}

FighterState* DashSpecialState::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

FighterState* DashSpecialState::attackConnected(GameEntity *victim)
{
    return FighterState::attackConnected(victim);
}


//// --------------- CHARLIE NEUTRAL SPECIAL STATE -------------------------
CharlieNeutralSpecial::CharlieNeutralSpecial(Fighter *f, bool ground) :
    SpecialState(f, ground),
    pre_("neutralSpecialAttack."),
    t_(0.f),
    shot_(false)
{
    frameName_ = "NeutralSpecial";
}

FighterState * CharlieNeutralSpecial::processInput(controller_state &, float dt)
{
    t_ += dt;
    if (t_ > fighter_->param(pre_ + "startup") && !shot_)
    {
        shot_ = true;

        AudioManager::get()->playSound("projectile");

        Projectile *projectile =
            new Projectile(fighter_->pos_,
                    glm::vec2(fighter_->dir_, 0.f), fighter_->pre_ + pre_,
                    "Projectile", "projectilehit", fighter_->getPlayerID(),
                    fighter_->getTeamID(), fighter_->getColor());
        addEntity(projectile);
    }
    // Check for transition away
    if (t_ > fighter_->param(pre_ + "startup") +
             fighter_->param(pre_ + "cooldown"))
    {
        if (ground_)
        {
            fighter_->vel_ = glm::vec2(0.f);
            return new GroundState(fighter_);
        }
        else
        {
            return new AirNormalState(fighter_);
        }
    }

    // No Transition otherwise
    return NULL;
}

void CharlieNeutralSpecial::render(float dt)
{
    fighter_->renderHelper(dt, fighter_->getColor());
}

FighterState* CharlieNeutralSpecial::collisionWithGround(const rectangle &ground, bool collision,
        bool platform)
{
    return SpecialState::collisionWithGround(ground, collision, platform);
}

FighterState* CharlieNeutralSpecial::hitByAttack(const Attack *attack)
{
    return FighterState::calculateHitResult(attack);
}

FighterState* CharlieNeutralSpecial::attackConnected(GameEntity *victim)
{
    return FighterState::attackConnected(victim);
}
