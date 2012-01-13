#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include "ExplosionManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "AudioManager.h"
#include "Attack.h"
#include "Controller.h"
#include "FighterState.h"
#include "StatsManager.h"

const std::string Fighter::type = "FighterEntity";

Fighter::Fighter(const glm::vec3& color, int playerID,
        int teamID, int startingLives, const std::string &username) :
    dir_(-1),
    state_(0),
    damage_(0),
    shieldHealth_(getParam("shield.maxHealth")),
    lives_(startingLives),
    respawnx_(0), respawny_(0),
    color_(color),
    username_(username),
    attack_(NULL),
    lastHitBy_(-1)
{
    // Set GameEntity members
    pos_ = vel_ = accel_ = glm::vec2(0.f, 0.f);
    size_ = glm::vec2(getParam("fighter.w"), getParam("fighter.h"));
    playerID_ = playerID;
    teamID_ = teamID;

    // Load ground attacks
    std::string g = "groundhit";
    std::string a = "airhit";
    std::string s = "smashhit";

    attackMap_["dash"] = loadAttack<DashAttack>("dashAttack", g, "DashAttack");
    attackMap_["dash"]->setHitboxFrame("DashAttackHitbox");

    attackMap_["ledge"] = loadAttack<FighterAttack>("ledgeAttack", g, "LedgeAttack");
    attackMap_["ledge"]->setHitboxFrame("LedgeAttackHitbox");

    attackMap_["neutralTilt"] = loadAttack<FighterAttack>("neutralTiltAttack", g, "GroundNeutral");
    attackMap_["neutralTilt"]->setHitboxFrame("NeutralTiltHitbox");
    attackMap_["sideTilt"] = loadAttack<FighterAttack>("sideTiltAttack", g, "GroundSidetilt");
    attackMap_["sideTilt"]->setHitboxFrame("SideTiltHitbox");
    attackMap_["downTilt"] = loadAttack<FighterAttack>("downTiltAttack", g, "GroundDowntilt");
    attackMap_["downTilt"]->setHitboxFrame("DownTiltHitbox");
    attackMap_["upTilt"] = loadAttack<FighterAttack>("upTiltAttack", g, "GroundUptilt");
    attackMap_["upTilt"]->setHitboxFrame("UpTiltHitbox");

    // Load air attack special as it uses a different class
    attackMap_["airNeutral"] = loadAttack<FighterAttack>("airNeutralAttack", a, "AirNeutral");
    attackMap_["airNeutral"]->setHitboxFrame("AirNeutralHitbox");
    attackMap_["airFront"] = loadAttack<FighterAttack>("airFrontAttack", a, "AirFronttilt");
    attackMap_["airFront"]->setHitboxFrame("AirFronttiltHitbox");
    attackMap_["airBack"] = loadAttack<FighterAttack>("airBackAttack", a, "AirBacktilt");
    attackMap_["airDown"] = loadAttack<FighterAttack>("airDownAttack", a, "AirDowntilt");
    attackMap_["airDown"]->setHitboxFrame("AirDowntiltHitbox");
    attackMap_["airUp"] = loadAttack<FighterAttack>("airUpAttack", a, "AirUptilt");
    attackMap_["airUp"]->setHitboxFrame("AirUptiltHitbox");

    attackMap_["upSpecial"] = loadAttack<UpSpecialAttack>("upSpecialAttack", a, "UpSpecial");
    attackMap_["upSpecial"]->setHitboxFrame("UpSpecialHitbox");
    attackMap_["upSpecial"]->setStartSound("upspecial");
    attackMap_["neutralSpecial"] = new NeutralSpecialAttack("neutralSpecialAttack", "NeutralSpecial");
    attackMap_["neutralSpecial"]->setStartSound("projectile");
    attackMap_["dashSpecial"] = loadAttack<FighterAttack>("dashSpecialAttack", "dashspecialhit", "DashSpecial");
    attackMap_["dashSpecial"]->setHitboxFrame("Null");
    attackMap_["dashSpecial"]->setStartSound("dashspecialhit");

    attackMap_["tauntUp"] = loadAttack<FighterAttack>("tauntAttack", a, "TauntAttack");
    attackMap_["tauntDown"] = loadAttack<FighterAttack>("tauntAttack", a, "Bong");

    attackMap_["neutralSmash"] = loadAttack<VaryingDirectionAttack>("neutralSmashAttack", s, "NeutralSmash");
    attackMap_["neutralSmash"]->setTwinkle(true);
    attackMap_["neutralSmash"]->setHitboxFrame("Null");
    attackMap_["sideSmash"] = loadAttack<FighterAttack>("sideSmashAttack", s, "SideSmash");
    attackMap_["sideSmash"]->setTwinkle(true);
    attackMap_["sideSmash"]->setHitboxFrame("SideSmashHitbox");
    attackMap_["downSmash"] = loadAttack<VaryingDirectionAttack>("downSmashAttack", s, "DownSmash");
    attackMap_["downSmash"]->setTwinkle(true);
    attackMap_["downSmash"]->setHitboxFrame("DownSmashHitbox");
    attackMap_["upSmash"] = loadAttack<MovingHitboxAttack>("upSmashAttack", s, "UpSmash");
    attackMap_["upSmash"]->setTwinkle(true);
    attackMap_["upSmash"]->setHitboxFrame("UpSmashHitbox");

    // FIXME No noise for grab hit, or hitbox frame
    attackMap_["grab"] = loadAttack<FighterAttack>("grabAttack", "", "GrabAttempt");
    //attackMap_["grab"]->setHitboxFrame("GrabbingHitbox");


    // Set up the twinkle moves
    attackMap_["airFront"]->setTwinkle(true);

    state_ = 0;
}

void Fighter::setRespawnLocation(float x, float y)
{
    respawnx_ = x;
    respawny_ = y;
}

Fighter::~Fighter()
{
    delete state_;
    // Clean up attacks
    std::map<std::string, FighterAttack *>::iterator it;
    for (it = attackMap_.begin(); it != attackMap_.end(); it++)
        delete it->second;
}

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

rectangle Fighter::getRect() const
{
    return state_->getRect();
}

void Fighter::processInput(controller_state &controller, float dt)
{
    // TODO move this to update? frame vs realtime issue then
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

    // Update state
    stateWrapper(state_->processInput(controller, dt));
}

void Fighter::stateWrapper(FighterState *next)
{
    if (!next) return;
    delete state_;
    state_ = next;
}

void Fighter::update(float dt)
{
    // First integrate
    GameEntity::update(dt);

    // Regain some shield health
    shieldHealth_ += getParam("shield.regen") * dt;
    shieldHealth_ = std::min(shieldHealth_, getParam("shield.maxHealth"));

    // State update
    state_->update(dt);
}

void Fighter::collisionWithGround(const rectangle &ground, bool collision)
{
    stateWrapper(state_->collisionWithGround(ground, collision));
}

void Fighter::attackCollision(const Attack *inAttack)
{
    // If two attacks collide, just cancel them and go to cooldown
    assert(attack_);
    assert(inAttack);
    attack_->attackCollision(inAttack);
}

void Fighter::hitByAttack(const Attack *attack)
{
    assert(attack);
    assert(attack->canHit(this));

    // Can't be hit by our own attacks
    if (attack->getPlayerID() == playerID_) return;

    stateWrapper(state_->hitByAttack(attack));

    lastHitBy_ = attack->getPlayerID();
}

void Fighter::attackConnected(GameEntity *victim)
{
    assert(attack_);
    assert(attack_->canHit(victim));

    // Cant hit ourself
    if (victim->getPlayerID() == playerID_) return;

    // Defer to the state
    stateWrapper(state_->attackConnected(victim));
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
    pos_.x = respawnx_;
    pos_.y = respawny_;
    vel_ = glm::vec2(0.f);
    accel_ = glm::vec2(0.f);
    damage_ = 0;
    lastHitBy_ = -1;
    // Clear the streaks
    StatsManager::get()->setStat(statPrefix(playerID_) + "curKillStreak", 0);
    StatsManager::get()->setStat(statPrefix(playerID_) + "damageStreak", 0);
    // If we died remove a life and play a sound
    if (killed)
    {
        --lives_;
        AudioManager::get()->playSound("ko");
        StatsManager::get()->addStat(statPrefix(playerID_) + "deaths", 1);
    }
    delete state_;
    // Set the new state, either respawn or dead
    if (lives_ <= 0)
        state_ = new DeadState(this);
    else
        // Set state to respawn state
        state_ = new RespawnState(this);
}

LimpFighter* Fighter::goLimp(UnlimpCallback *l)
{
    // Cancel any current attack
    if (attack_)
    {
        attack_->finish();
        delete attack_;
        attack_ = 0;
    }

    // Then go into the limp state
    delete state_;
    LimpState* ret = new LimpState(this, l);
    state_ = ret;
    return ret;
}

bool Fighter::isAlive() const
{
    return lives_ > 0;
}

void Fighter::render(float dt)
{
    state_->render(dt);
}

void Fighter::stealLife()
{
    assert(lives_ > 1);
    lives_--;
}


void Fighter::renderHelper(float dt, const std::string &frameName, const glm::vec3 &color,
        const glm::mat4 &postTrans)
{
    printf("ID: %d  Damage: %.1f  Pos: [%.2f, %.2f]  Vel: [%.2f, %.2f]  Accel: [%.2f, %.2f]  Attack: %d  Dir: %.1f\n",
            id_, damage_, pos_.x, pos_.y, vel_.x, vel_.y, accel_.x, accel_.y, attack_ != 0, dir_);

    // Draw body
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(pos_.x, pos_.y, 0.0)),
            glm::vec3(dir_, 1.0f, 1.0f));

    FrameManager::get()->renderFrame(transform * postTrans, glm::vec4(color, 0.25f), frameName);

    // Check for rendering hitbox
    if (getParam("debug.drawHitbox") != 0.f)
    {
        rectangle hb = getRect();
        glm::mat4 hboxtrans = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(hb.x, hb.y, 0.0f)),
                glm::vec3(dir_ * hb.w, hb.h, 1.f));
        renderRectangle(hboxtrans, glm::vec4(color, 0.f));
    }

    if (attack_)
        attack_->render(dt);
}

template<class AttackClass>
AttackClass* Fighter::loadAttack(std::string attackName, const std::string &audioID,
        const std::string &fname)
{
    AttackClass *ret = new AttackClass(attackName, audioID, fname);

    return ret;

}



// ----------------------------------------------------------------------------
// Rectangle class methods
// ----------------------------------------------------------------------------

rectangle::rectangle() :
    x(0), y(0), w(0), h(0)
{}

rectangle::rectangle(float xin, float yin, float win, float hin) :
    x(xin), y(yin), w(win), h(hin)
{}

bool rectangle::overlaps(const rectangle &rhs) const
{
    return (rhs.x + rhs.w/2) > (x - w/2) && (rhs.x - rhs.w/2) < (x + w/2) &&
        (rhs.y + rhs.h/2) > (y - h/2) && (rhs.y - rhs.h/2) < (y + h/2);
}

bool rectangle::contains(const rectangle &rhs) const
{
    return (rhs.x - rhs.w/2) > (x - w/2) && (rhs.x + rhs.w/2) < (x + w/2) &&
        (rhs.y - rhs.h/2) > (y - h/2) && (rhs.y + rhs.h/2) < (y + h/2);
}
