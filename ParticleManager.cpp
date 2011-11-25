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
        emitters[i]->emit(particles_);
    }

    // then update old particles
    for (unsigned i = 0; i < particles.size(); i++) 
    {
        
        Particle *p = particles[i];
        p->loc_ = p->loc_ + p->vel_;
    }
 
    // finally draw existing particles
}

void Emitter::emit(std::list<Particle*> particles) 
{
    int numNewParts = rate_ * dt;
    for (int j = 0; j < numNewParts; j++)     
    {
        Particle *p = new Particle();
        p->loc = loc_;
        p->t = lifetime_;
        p->vel = vel_;
        particles.push_back(p);
    }
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


