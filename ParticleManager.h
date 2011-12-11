#pragma once

#include <glm/glm.hpp>
#include <list>
#include "Fighter.h"
#include "Particle.h"
#include "Emitter.h"

class PGroup;

// Utility functions

glm::vec3 pointOnSphere(float r, glm::vec3 pos);  
float normalRandom(float mu, float sigma);

//
// Particle system manager declaration.
//
class ParticleManager 
{
public:
    // Get() access to the singleton instance
    static ParticleManager* get();

    void addGroup(PGroup *);

    void render(float dt);

private:
    ParticleManager();
    ~ParticleManager();

    std::list<PGroup*> groups_; 

};

