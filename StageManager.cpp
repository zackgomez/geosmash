#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"

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

