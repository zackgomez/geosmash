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
