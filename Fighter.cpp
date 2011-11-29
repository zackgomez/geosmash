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
#include "Attack.h"

void pause(int playerID);
Fighter *getPartnerLifeSteal(int playerID);

Fighter::Fighter(float respawnx, float respawny, const glm::vec3& color, int id) :
    dir_(-1),
    state_(0),
    damage_(0),
    shieldHealth_(getParam("shield.maxHealth")),
    lives_(getParam("fighter.lives")),
    respawnx_(respawnx), respawny_(respawny),
    color_(color),
    attack_(NULL),
    lastHitBy_(-1)
{
    // Set GameEntity members
    pos_ = vel_ = accel_ = glm::vec2(0.f, 0.f);
    size_ = glm::vec2(getParam("fighter.w"), getParam("fighter.h"));
    id_ = id;
    playerID_ = id;

    // Load ground attacks
    std::string g = "groundhit";
    std::string a = "airhit";
    std::string s = "smashhit";

    attackMap_["dash"] = loadAttack<DashAttack>("dashAttack", g, "DashAttack");

    attackMap_["neutralTilt"] = loadAttack<Attack>("neutralTiltAttack", g, "GroundNeutral");
    attackMap_["sideTilt"] = loadAttack<Attack>("sideTiltAttack", g, "GroundSidetilt");
    attackMap_["downTilt"] = loadAttack<Attack>("downTiltAttack", g, "GroundDowntilt");
    attackMap_["upTilt"] = loadAttack<Attack>("upTiltAttack", g, "GroundUptilt");

    // Load air attack special as it uses a different class
    attackMap_["airNeutral"] = loadAttack<Attack>("airNeutralAttack", a, "AirNeutral");
    attackMap_["airFront"] = loadAttack<Attack>("airFrontAttack", a, "AirFronttilt");
    attackMap_["airBack"] = loadAttack<Attack>("airBackAttack", a, "AirBacktilt");
    attackMap_["airDown"] = loadAttack<Attack>("airDownAttack", a, "AirDowntilt");
    attackMap_["airUp"] = loadAttack<Attack>("airUpAttack", a, "AirUptilt");

    attackMap_["upSpecial"] = loadAttack<UpSpecialAttack>("upSpecialAttack", a, "UpSpecial");
    attackMap_["neutralSpecial"] = new NeutralSpecialAttack("neutralSpecialAttack", "NeutralSpecial");

    attackMap_["taunt"] = loadAttack<Attack>("tauntAttack", a, "TauntAttack");

    attackMap_["neutralSmash"] = loadAttack<Attack>("neutralSmashAttack", s, "NeutralSmash");
    attackMap_["neutralSmash"]->setTwinkle(true);
    //attackMap_["neutralSmash"]->setHitboxFrame("NeutralSmashHitbox");
    attackMap_["sideSmash"] = loadAttack<Attack>("sideSmashAttack", s, "SideSmash");
    attackMap_["sideSmash"]->setTwinkle(true);
    attackMap_["sideSmash"]->setHitboxFrame("SideSmashHitbox");
    attackMap_["downSmash"] = loadAttack<Attack>("downSmashAttack", s, "DownSmash");
    attackMap_["downSmash"]->setTwinkle(true);
    attackMap_["downSmash"]->setHitboxFrame("DownSmashHitbox");
    attackMap_["upSmash"] = loadAttack<MovingAttack>("upSmashAttack", s, "UpSmash");
    attackMap_["upSmash"]->setTwinkle(true);
    attackMap_["upSmash"]->setHitboxFrame("UpSmashHitbox");

    // Set up the twinkle moves
    attackMap_["airFront"]->setTwinkle(true);

    state_ = 0;
}

Fighter::~Fighter()
{
    delete state_;
    // Clean up attacks
    std::map<std::string, Attack *>::iterator it;
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

Rectangle Fighter::getRect() const
{
    return state_->getRect();
}

void Fighter::processInput(Controller &controller, float dt)
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

    // Check for pause/life steal
    if (controller.pressstart)
    {
        // Pause when character is alive
        if (isAlive())
            pause(id_);
        // Otherwise check for life steal from partner
        else
        {
            Fighter *partner = getPartnerLifeSteal(playerID_);
            if (partner)
            {
                assert(partner->lives_ > 1);
                partner->lives_--;
                lives_++;
                respawn(false);
            }
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
    state_->processInput(controller, dt);
}

void Fighter::update(float dt)
{
    // Regain some shield health
    shieldHealth_ += getParam("shield.regen") * dt;
    shieldHealth_ = std::min(shieldHealth_, getParam("shield.maxHealth"));
    GameEntity::update(dt);
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

void Fighter::hitByAttack(const Attack *attack)
{
    assert(attack);
    assert(attack->canHit(this));

    // Can't be hit by our own attacks
    if (attack->getPlayerID() == playerID_) return;

    state_->hitByAttack(attack);

    lastHitBy_ = attack->getPlayerID();
}

void Fighter::attackConnected(GameEntity *victim)
{
    assert(attack_);
    assert(attack_->canHit(victim));

    attack_->hit(victim);
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
        Rectangle hb = getRect();
        glm::mat4 hboxtrans = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(hb.x, hb.y, 0.0f)),
                glm::vec3(dir_ * hb.w, hb.h, 1.f));
        renderRectangle(hboxtrans, glm::vec4(color, 0.f));
    }

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
    AttackClass *ret = new AttackClass(attackName, audioID, fname);

    return ret;

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
