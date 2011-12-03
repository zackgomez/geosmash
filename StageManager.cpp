#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Fighter.h"
#include "FrameManager.h"

StageManager::StageManager()
{
    glm::vec2 groundpos(getParam("level.x"), getParam("level.y"));
    glm::vec2 groundsize(getParam("level.w"), getParam("level.h"));
    Ledge l;
    l.pos = glm::vec2(groundpos.x - groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    ledges_.push_back(new Ledge(l));

    l.pos = glm::vec2(groundpos.x + groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    ledges_.push_back(new Ledge(l));
}

StageManager *StageManager::get()
{
    static StageManager sm;
    return &sm;
}

Ledge * StageManager::getPossibleLedge(const glm::vec2 &pos)
{
    Ledge *ret = NULL;
    float mindist = HUGE_VAL;
    for (unsigned i = 0; i < ledges_.size(); i++)
    {
        Ledge *l = ledges_[i];
        float dist = glm::length(pos - l->pos);
        if (!l->occupied && dist < mindist)
        {
            ret = l;
            mindist = dist;
        }
    }

    return ret;
}

bool HazardEntity::isDone() const
{
    return false; 
}

void HazardEntity::update(float dt)
{
    // Each frame, we only need to do a couple things.
    // First, move a bit randomly
    // then, update our attack to reflect our location.

    // change direction every now and then:
    dir_ = rand() % static_cast<int>(getParam(pre_ + "switchRate")) ? dir_ : -dir_;
    vel_.x = getParam(pre_ + "speed") * dir_;


    t_ += dt;
    if (t_ > getParam(pre_ + "cooldown")) 
    {
        attack_->clearHit();
        t_ = 0;
    }

    GameEntity::update(dt);
    attack_->setPosition(pos_);
}

void HazardEntity::render(float dt)
{
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(1.f));
#define HAZARD_COLOR 0.5, 0.5, 1
    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(HAZARD_COLOR, 0.3)),
            frameName_);
}

HazardEntity::HazardEntity(const std::string &audioID) 
{
    pre_ = "stageHazardAttack.";

    frameName_ = "Hazard";
    pos_ = glm::vec2(0, getParam("level.y") + getParam("level.h") / 2 + getParam(pre_ + "sizey") / 2 - 1);
    size_ = glm::vec2(getParam(pre_ + "sizex"), getParam(pre_ + "sizey"));
    lifetime_ = getParam(pre_ + "lifetime");
    dir_ = 1; // initially, we'll go right.

    attack_ = new SimpleAttack(
            getParam(pre_ + "knockbackpow") *
            glm::normalize(glm::vec2(getParam(pre_ + "knockbackx"),
                      getParam(pre_ + "knockbacky"))),
            getParam(pre_ + "damage"),
            getParam(pre_ + "stun"),
            getParam(pre_ + "priority"),
            pos_, size_, playerID_,
            audioID);
}

void HazardEntity::attackCollision(const Attack*)
{
}

const Attack *HazardEntity::getAttack() const
{
    attack_->setKBDirection(dir_);
    return attack_;
}

void HazardEntity::hitByAttack(const Attack*) 
{
}

void HazardEntity::attackConnected(GameEntity *victim)
{
    attack_->hit(victim);
}

void HazardEntity::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision)
        dir_ = -dir_;
}

