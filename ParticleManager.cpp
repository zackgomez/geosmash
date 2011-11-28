#include <iostream>
#include "ParticleManager.h"
#include "Particle.h"
#include "Emitter.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "glutils.h"
#include <cstdlib>


glm::vec3 pointOnSphere(float r, glm::vec3 pos)
{
    float theta = M_PI * float(rand()) / RAND_MAX;
    float phi = 2 * M_PI * float(rand()) / RAND_MAX;
    glm::vec3 ans;
    ans.x = r * sin(theta) * cos(phi);
    ans.y = r * sin(theta) * sin(phi);
    ans.z = r * cos(theta);
    return pos + ans;
}

// Approximate a normal variable.
// http://en.wikipedia.org/wiki/Normal_distribution#Generating_values_from_normal_distribution
// A sum of several uniform variables is like a normal distr.
// (central limit theorom)
float normalRandom(float mu, float sigma)
{
    float ans;
    for (int i = 0; i < 6; i++)
    {
        ans += rand() / RAND_MAX;
    }
    return mu + (ans - 6) * sigma;
}

void test_random() {
    float s1 = 1;
    float mu = 7;
    for (int i = 0; i < 10; i++)
    {
        printf("normalRandom(mu = %f, sigma = %f) = %f\n", mu, s1);
    }

    s1 = 10;
    mu = -100;
    for (int i = 0; i < 10; i++)
    {
        printf("normalRandom(mu = %f, sigma = %f) = %f\n", mu, s1);
    }


}

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
    loc = loc + vel * dt;
    t += dt;
}
    
void Particle::render() 
{
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
        p->loc = pointOnSphere(radius_, loc_);
        p->t = lifetime_;
        // Particle's velocity is the normal at that point
        // scaled by emitter's velocity value and given some
        // random nonsense
        p->vel = normalRandom(vel_, 1) * (p->loc - loc_) / radius_;
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

Emitter* Emitter::setParticleVelocityVariance(float r) 
{ 
    var_ = r;
    return this;
}

Emitter* Emitter::setRadius(float r)
{
    radius_ = r;
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


