#include <iostream>
#include "ParticleManager.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"

ParticleManager::ParticleManager()
{}

// A combined update/render call...
// Two types of operations occur here:
//  - those that update emitters and particles
//  - those that render particles
void ParticleManager::render(float dt)
{
    // First create new particles.
    for (int i = 0; i < emitters_.size(); i++) 
	{
        emitters_[i]->emit(particles_, dt);
    }

    // Then update old particles.
    std::list<Particle*>::iterator pit;
    for (pit = particles_.begin(); pit != particles_.end(); pit++)  
    {    
        (*pit)->update(dt);
        // After updating, we have to delete any expired particles
        // (The particle who just updated isn't really capable of
        // cleaning up after itself.)
        if ((*pit)->t < 0) 
        {
            particles_.erase(pit);
        }
    }
 
    // finally draw existing particles
    for (pit = particles_.begin(); pit != particles_.end(); pit++)
    {
        (*pit)->render();
    }
}

void Particle::update(float dt) 
{
    assert(emitter);
    loc = loc + vel * dt;
    t += dt;
}
    
void Particle::render() 
{
    assert(emitter);
    glm::mat4 transform = glm::scale(
        glm::translate(
            glm::mat4(1.0f), loc), size);
    renderRectangle(transform, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
}

void Emitter::emit(std::list<Particle*> particles, float dt) 
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


