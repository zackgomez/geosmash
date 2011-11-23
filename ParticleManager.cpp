#include <iostream>
#include "ParticleManager.h"
#include "ParamReader.h"

ParticleManager::ParticleManager()
{}

void ParticleManager::render(float dt)
{
    // first create new particles
    for (int i = 0; i < emitters.size(); i++) 
    {
        Emitter* e = emitters[i];
        int numNewParts = emitters[i]->rate_ * dt;
        for (int j = 0; j < numNewParts; j++)
        {
            Particle *p = new Particle();
            p->loc = e->loc_;
            p->t = e->lifetime_;
            p->vel = e->vel_;
            particles.push_back(p);
        }

    }

    // then update old particles
    for (unsigned i = 0; i < particles.size(); i++) 
    {
        
        //Particle *p = particles[i];
    }
 
    // finally draw existing particles
}

ParticleManager* ParticleManager::get()
{
    static ParticleManager pm;
    return &pm;
}

Emitter* Emitter::setParticleLifetime(float l) 
{ 
    lifetime_ = l; 
    return this;
}


Emitter* Emitter::setLocation(glm::vec3 l) 
{ 
    loc_ = l; 
    return this;
}

Emitter* Emitter::setOutputRate(float r)
{
    rate_ = r;
    return this;
}

