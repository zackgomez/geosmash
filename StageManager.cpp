#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"
#include "Fighter.h"

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
    return lifetime_ > 0;
}

HazardEntity::HazardEntity(const std::string &audioID)
{


    std::string pre = "stageHazard";
    attack_ = new ProjectileHelperAttack(
            getParam(pre + "knockbackpow") *
            glm::normalize(glm::vec2(getParam(pre + "knockbackx"),
                      getParam(pre + "knockbacky"))),
            getParam(pre + "damage"),
            getParam(pre + "stun"),
            pos_, size_, playerID_,
            audioID);

}

const Attack *HazardEntity::getAttack() const
{
    return attack_;
}

