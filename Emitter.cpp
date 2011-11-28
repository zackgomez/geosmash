#include "Emitter.h"
#include <cstdio>

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


