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
    std::list<Particle*>::iterator pit;
    for (pit = particles_.begin(); pit != particles_.end(); pit++)  
    {    
        (*pit)->update(dt);
        if ((*pit)->t < 0) 
        {
            particles_.erase(pit);
        }
    }
 
    // finally draw existing particles
    // Make a call to drawRectangle()
}

void Particle::update(float dt) 
{
    assert(emitter);
    loc = loc + vel * dt;
    t += dt;
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
        p->emitter = this;
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


