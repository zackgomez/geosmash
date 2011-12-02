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
    GameEntity::update(dt);
    // Each frame, we only need to do a couple things.
    // First, move a bit randomly
    // then, update our attack to reflect our location.

    // change direction every now and then:
    dir_ = rand() % 50 ? dir_ : -dir_;  
    pos_.x += dir_ * 1;
    attack_->setPosition(pos_);
}

void HazardEntity::render(float dt)
{
    printf("Rendering some shit.\n");
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(1.f));
#define HAZARD_COLOR 0.5, 0.5, 1
    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(HAZARD_COLOR, 0.3)),
            frameName_);
}

HazardEntity::HazardEntity(const std::string &audioID)
{


    std::string pre = "stageHazardAttack.";
    attack_ = new SimpleAttack(
            getParam(pre + "knockbackpow") *
            glm::normalize(glm::vec2(getParam(pre + "knockbackx"),
                      getParam(pre + "knockbacky"))),
            getParam(pre + "damage"),
            getParam(pre + "stun"),
            getParam(pre + "priority"),
            pos_, size_, playerID_,
            audioID);

    frameName_ = "Hazard";
    pos_.x = 0;
    pos_.y = getParam("level.y") + 50;
    lifetime_ = 1e7;
    dir_ = 1; // initially, we'll go right.
}

void HazardEntity::attackCollision(const Attack*)
{
    // Someone hit us. Oh, I'm so scared! o wait, NOP.
}
const Attack *HazardEntity::getAttack() const
{
    return attack_;
}

void HazardEntity::hitByAttack(const Attack*) 
{
    // NOP!
}

void HazardEntity::attackConnected(GameEntity *)
{
    // Do I care? 
    // NOP!
}

void HazardEntity::collisionWithGround(const Rectangle &, bool)
{
    // NOP!
}

