#pragma once

#include "glutils.h"
#include <vector>
#include <glm/glm.hpp>
#include "Fighter.h"
class ParticleManager 
{
public:
    ParticleManager();
    static ParticleManager* get();

    void render(float dt);


};

