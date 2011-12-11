#include "Emitter.h"
#include <list>

#include "PGroup.h"
void PGroup::render(float dt)
{
    // First create new particles.
    std::list<Emitter*>::iterator eit;
    for (eit = emitters_.begin(); eit != emitters_.end(); eit++)
	{
        (*eit)->emit(particles_, dt);
        if ((*eit)->isDone()) 
        {
            delete *eit;
            eit = emitters_.erase(eit);
        }
    }

    std::list<Particle*>::iterator pit;
    for (pit = particles_.begin(); pit != particles_.end(); ++pit)
    {
        // apply each action to each particle
        std::list<PAction*>::iterator ait;
        for (ait = actions_.begin(); ait != actions_.end(); ++ait)
        {
            (**ait)(*pit);
        }
    }

}
