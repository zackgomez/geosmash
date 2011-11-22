#include "Projectile.h"
#include "ParamReader.h"

Projectile::Projectile(const glm::vec2 &pos, const glm::vec2 &dir,
        const std::string &paramPrefix, const std::string &frameName,
        int playerID) :
    GameEntity(),
    attack_(NULL),
    hit_(false),
    frameName_(frameName)
{
    paramPrefix_ = paramPrefix + '.';
    playerID_ = playerID;

    pos_ = pos;
    vel_ = dir * getParam(paramPrefix + "speed");
    accel_ = glm::vec2(0.f);
}
