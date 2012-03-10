#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include "ExplosionManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "AudioManager.h"
#include "Attack.h"
#include "Controller.h"
#include "FighterState.h"
// TODO remove this to a factory method of some kind
#include "StickmanStates.h"
#include "StatsManager.h"

const std::string Fighter::type = "FighterEntity";

Fighter::Fighter(const glm::vec3& color, int playerID,
        int teamID, int startingLives, const std::string &username,
        const std::string &fighterName) :
    state_(0),
    pre_(fighterName + '.'),
    dir_(-1),
    damage_(0),
    shieldHealth_(param("shield.maxHealth")),
    lives_(startingLives),
    respawnx_(0), respawny_(0),
    color_(color),
    username_(username),
    attack_(NULL)
{
    // Set GameEntity members
    pos_ = vel_ = accel_ = glm::vec2(0.f, 0.f);
    size_ = glm::vec2(param("w"), param("h"));
    playerID_ = playerID;
    teamID_ = teamID;

    // Create the renderer
    if (fighterName == "charlie")
        renderer_ = new BixelFighterRenderer();
    else if (fighterName == "stickman")
        renderer_ = new SkeletonFighterRenderer();
    else
        assert(false && "Unknown fighter name");

    fillAttacks(fighterName);
}

void Fighter::fillAttacks(const std::string &moveset)
{
    assert(moveset == "charlie" || moveset == "stickman");

    // Load ground attacks
    std::string g = "groundhit";
    std::string a = "airhit";
    std::string s = "smashhit";

    attackMap_["neutralTilt"] = loadAttack<FighterAttack>("neutralTiltAttack", g, "GroundNeutral");
    attackMap_["sideTilt"] = loadAttack<FighterAttack>("sideTiltAttack", g, "GroundSidetilt");
    attackMap_["downTilt"] = loadAttack<FighterAttack>("downTiltAttack", g, "GroundDowntilt");
    attackMap_["upTilt"] = loadAttack<FighterAttack>("upTiltAttack", g, "GroundUptilt");

    attackMap_["airNeutral"] = loadAttack<VaryingDirectionAttack>("airNeutralAttack", a, "AirNeutral");
    attackMap_["airFront"] = loadAttack<FighterAttack>("airFrontAttack", a, "AirFronttilt");
    attackMap_["airUp"] = loadAttack<FighterAttack>("airUpAttack", a, "AirUptilt");

    attackMap_["neutralSmash"] = loadAttack<FighterAttack>("neutralSmashAttack", s, "NeutralSmash");
    attackMap_["neutralSmash"]->setTwinkle(true);
    attackMap_["sideSmash"] = loadAttack<FighterAttack>("sideSmashAttack", s, "SideSmash");
    attackMap_["sideSmash"]->setTwinkle(true);
    attackMap_["upSmash"] = loadAttack<MovingHitboxAttack>("upSmashAttack", s, "UpSmash");
    attackMap_["upSmash"]->setTwinkle(true);

    attackMap_["dash"] = loadAttack<DashAttack>("dashAttack", g, "DashAttack");

    attackMap_["ledge"] = loadAttack<FighterAttack>("ledgeAttack", g, "LedgeAttack");
    attackMap_["ledge"]->setHitboxFrame("LedgeAttackHitbox");

    attackMap_["tauntUp"] = loadAttack<FighterAttack>("tauntAttack", a, "TauntA");
    attackMap_["tauntDown"] = loadAttack<FighterAttack>("tauntAttack", a, "TauntB");

    attackMap_["grab"] = loadAttack<FighterAttack>("grabAttack", "", "GrabAttempt");
    attackMap_["grab"]->setStartSound("grabattempt");

    // TODO make most of this be in a static map or params read in loadAttack
    if (moveset == "charlie")
    {
        attackMap_["airDown"] = loadAttack<FighterAttack>("airDownAttack", a, "AirDowntilt");
        attackMap_["downSmash"] = loadAttack<FighterAttack>("downSmashAttack", s, "DownSmash");
        attackMap_["downSmash"]->setTwinkle(true);

        attackMap_["neutralTilt"]->setHitboxFrame("NeutralTiltHitbox");
        attackMap_["sideTilt"]->setHitboxFrame("SideTiltHitbox");
        attackMap_["downTilt"]->setHitboxFrame("DownTiltHitbox");
        attackMap_["upTilt"]->setHitboxFrame("UpTiltHitbox");
        attackMap_["dash"]->setHitboxFrame("DashAttackHitbox");

        attackMap_["neutralSmash"]->setHitboxFrame("Null");
        attackMap_["sideSmash"]->setHitboxFrame("SideSmashHitbox");
        attackMap_["downSmash"]->setHitboxFrame("DownSmashHitbox");
        attackMap_["upSmash"]->setHitboxFrame("UpSmashHitbox");

        attackMap_["airNeutral"]->setHitboxFrame("AirNeutralHitbox");
        attackMap_["airFront"]->setHitboxFrame("AirFronttiltHitbox");
        attackMap_["airFront"]->setTwinkle(true);
        attackMap_["airDown"]->setHitboxFrame("AirDowntiltHitbox");
        attackMap_["airUp"]->setHitboxFrame("AirUptiltHitbox");

        // FIXME no hitbox frame
        //attackMap_["grab"]->setHitboxFrame("GrabbingHitbox");

        attackMap_["upSpecial"] = loadAttack<UpSpecialAttack>("upSpecialAttack", a, "UpSpecial");
        attackMap_["upSpecial"]->setHitboxFrame("UpSpecialHitbox");
        attackMap_["upSpecial"]->setStartSound("upspecial");

        attackMap_["dashSpecial"] = loadAttack<FighterAttack>("sideSpecialAttack", "dashspecialhit", "DashSpecial");
        attackMap_["dashSpecial"]->setHitboxFrame("Null");
        attackMap_["dashSpecial"]->setStartSound("dashspecialhit");
        
        specialStateFactory_ = &getCharlieSpecialState;
    }
    else if (moveset == "stickman")
    {
        attackMap_["neutralTilt"]->setHitboxFrame("Null");
        attackMap_["sideTilt"]->setHitboxFrame("Null");
        attackMap_["downTilt"]->setHitboxFrame("Null");
        attackMap_["upTilt"]->setHitboxFrame("Null");
        attackMap_["dash"]->setHitboxFrame("Null");

        attackMap_["downSmash"] = loadAttack<MovingHitboxAttack>("downSmashAttack", s, "DownSmash");
        attackMap_["downSmash"]->setTwinkle(true);
        attackMap_["airDown"] = loadAttack<RepeatingAttack>("airDownAttack", a, "AirDowntilt");

        attackMap_["airNeutral"]->setHitboxFrame("Null");
        attackMap_["airFront"]->setHitboxFrame("Null");
        attackMap_["airDown"]->setHitboxFrame("Null");
        attackMap_["airUp"]->setHitboxFrame("Null");

        attackMap_["upSpecial"] = loadAttack<VelocityDirectionAttack>("upSpecialAttack", a, "UpSpecial");
        attackMap_["upSpecial"]->setHitboxFrame("Null");
        attackMap_["upSpecial"]->setStartSound("upspecial");

        specialStateFactory_ = &getStickmanSpecialState;
    }
    else
        assert(false && "unknown fighter");
}

void Fighter::setRespawnLocation(float x, float y)
{
    respawnx_ = x;
    respawny_ = y;
}

Fighter::~Fighter()
{
    delete state_;
    delete renderer_;
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
    if (airData_.find("lastHitBy") != airData_.end())
        return airData_.find("lastHitBy")->second;
    else
        // -1 signifies no last hit by, or suicide
        return -1;
}

rectangle Fighter::getRect() const
{
    return state_->getRect();
}

std::string Fighter::getFrameName() const 
{
    return lastFrameName_;
}

std::string Fighter::getFighterName() const
{
    return pre_.substr(0, pre_.length()-1);
}

void Fighter::reflect()
{
    GameEntity::reflect();
    dir_ = -dir_;
};

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
    shieldHealth_ += param("shield.regen") * dt;
    shieldHealth_ = std::min(shieldHealth_, param("shield.maxHealth"));

    // State update
    state_->update(dt);
}

void Fighter::collisionWithGround(const rectangle &ground, bool collision, bool platform)
{
    if (collision)
        lastGround_ = ground;

    stateWrapper(state_->collisionWithGround(ground, collision, platform));
}

void Fighter::attackCollision(const Attack *inAttack)
{
    // If two attacks collide, just cancel them and go to cooldown
    assert(attack_);
    assert(inAttack);
    stateWrapper(state_->attackCollision(inAttack));
}

void Fighter::hitByAttack(const Attack *attack)
{
    assert(attack);
    assert(attack->canHit(this));

    // Can't be hit by our own attacks
    if (attack->getPlayerID() == playerID_) return;

    stateWrapper(state_->hitByAttack(attack));

    // If attack is stage hazard, and we've already been hit,
    // keep current lastHitBy
    if (attack->getPlayerID() == -2 && airData_.count("lastHitBy"))
    { }
    else
        airData_["lastHitBy"] = attack->getPlayerID();
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
    // Cache for later
    bool selfkill = getLastHitBy() == -1;
    // Reset vars
    pos_.x = respawnx_;
    pos_.y = respawny_;
    vel_ = glm::vec2(0.f);
    accel_ = glm::vec2(0.f);
    damage_ = 0;
    airData_.clear();
    // Clear the streaks
    StatsManager::get()->setStat(statPrefix(playerID_) + "curKillStreak", 0);
    StatsManager::get()->setStat(statPrefix(playerID_) + "damageStreak", 0);
    // If we died remove a life and play a sound
    if (killed)
    {
        --lives_;
        // Play a sound for death, different for suicide or ko
        if (selfkill)
            AudioManager::get()->playSound("selfkill");
        else
            AudioManager::get()->playSound("ko");
        StatsManager::get()->addStat(statPrefix(playerID_) + "deaths", 1);

        // If player is dead and doesn't already have a place, set place
        if (lives_ == 0 &&
            StatsManager::get()->getStat(statPrefix(playerID_) + "place") == 0)
        {
            // Get, then update, next place stat
            int nextPlace = StatsManager::get()->getStat("numPlayers")
                - StatsManager::get()->getStat("tmp.playersOut");
            StatsManager::get()->addStat("numPlayers", -1.f);

            StatsManager::get()->setStat(statPrefix(playerID_) + "place", nextPlace);

            // Also set time of death
            StatsManager::get()->setStat(statPrefix(playerID_) + "deathTime",
                    static_cast<int>((getCurrentMillis() - StatsManager::get()->getStat("startMillis")) / 1000.f));
        }
    }
    delete state_;
    // Set the new state, either respawn or dead
    if (lives_ <= 0)
        state_ = new DeadState(this);
    else
        // Set state to respawn state
        state_ = new RespawnState(this);
}

void Fighter::takeDamage(float damage, int playerID, int teamID)
{
    state_->takeDamage(damage, playerID, teamID);
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

void Fighter::addLives(int delta)
{
    assert(delta > 0);
    lives_ += delta;
    
    // If they had zero lives before, respawn them
    if (lives_ == delta)
        respawn(false);
}


void Fighter::renderHelper(float dt, const glm::vec3 &color,
        const glm::mat4 &postTrans)
{
    //printf("ID: %d  Damage: %.1f  Pos: [%.2f, %.2f]  Vel: [%.2f, %.2f]  Accel: [%.2f, %.2f]  Attack: %d  Dir: %.1f\n",
            //id_, damage_, pos_.x, pos_.y, vel_.x, vel_.y, accel_.x, accel_.y, attack_ != 0, dir_);

    std::string frameName = state_->getFrameName();
    if (attack_)
        frameName = attack_->getFrameName();

    // Cache the frame name, so AI (or anybody) can request the name of the
    // last drawn frame
    lastFrameName_ = frameName;

    // Draw body
    glm::mat4 transform = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(pos_.x, pos_.y, 0.0)),
            dir_ == -1 ? 180.f : 0.f, glm::vec3(0, 1, 0));

    renderer_->render(transform * postTrans, glm::vec4(color, 0.25f), frameName, dt);
    //FrameManager::get()->renderFrame(transform * postTrans, glm::vec4(color, 0.25f), frameName);

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
AttackClass* Fighter::loadAttack(const std::string &attackName, const std::string &audioID,
        const std::string &fname)
{
    AttackClass *ret = new AttackClass(pre_ + attackName, audioID, fname);

    return ret;
}

float Fighter::param(const std::string &p) const
{
    return getParam(pre_+ p);
}

// ----------------------------------------------------------------------------
// BixelFighterRenderer
// ----------------------------------------------------------------------------

void BixelFighterRenderer::render(const glm::mat4 &transform, const glm::vec4 &color,
        const std::string &frameName, float dt) const
{
    // Frame manager draws frames a specific size, so we don't need to scale
    FrameManager::get()->renderFrame(transform, color, frameName);
}

// ----------------------------------------------------------------------------
// SkeletonFighterRenderer
// ----------------------------------------------------------------------------

SkeletonFighterRenderer::SkeletonFighterRenderer()
{
    skeleton_ = new Skeleton();
    renderer_ = new GeosmashBoneRenderer();

    skeleton_->readSkeleton("models/test.bones");
    skeleton_->setBoneRenderer(renderer_);
}

SkeletonFighterRenderer::~SkeletonFighterRenderer()
{
    delete skeleton_;
}

void SkeletonFighterRenderer::render(const glm::mat4 &transform, const glm::vec4 &color,
        const std::string &frameName, float dt) const
{
    glm::mat4 scaledTransform = glm::rotate(glm::scale(transform, glm::vec3(60, 50, 60)), 
		getParam("stickman.rotation"), glm::vec3(0.f, 1.f, 0.f));

    renderer_->setColor(color);

    skeleton_->resetPose();
    skeleton_->setPose(FrameManager::get()->getPose(frameName).bones);

    skeleton_->render(scaledTransform);
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
