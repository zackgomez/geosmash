#include <iostream>
#include "ParticleManager.h"
#include "Particle.h"
#include "Emitter.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
//#include "glutils.h"
#include <cstdlib>

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

ParticleManager* ParticleManager::get()
{
    static ParticleManager pm;
    return &pm;
}

