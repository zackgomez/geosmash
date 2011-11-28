#include "Emitter.h"
#include <cstdio>
#include <iostream>

// Approximate a normal variable.
// http://en.wikipedia.org/wiki/Normal_distribution#Generating_values_from_normal_distribution
// A sum of several uniform variables is like a normal distr.
// (central limit theorom)
float normalRandom(float mu, float sigma)
{
    float ans;
    for (int i = 0; i < 6; i++)
    {
        ans += static_cast<float>(rand()) / RAND_MAX;
    }
    return mu + (ans - 3) * sigma;
}

void test_random() {
    float s1 = 1;
    float mu = 7;
    for (int i = 0; i < 10; i++)
    {
        printf("normalRandom(mu = %f, sigma = %f) = %f\n", mu, s1, normalRandom(mu, s1));
    }

    s1 = 10;
    mu = -100;
    for (int i = 0; i < 10; i++)
    {
        printf("normalRandom(mu = %f, sigma = %f) = %f\n", mu, s1, normalRandom(mu, s1));
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

Emitter::Emitter() :
    lifetime_(0.3f),
    var_(0.f),
    loc_(glm::vec3(0.f)),
    radius_(2.f),
    vel_(50.f),
    rate_(400.f),
    size_(1.f),
    color_(glm::vec4(1.0f, 0.0f, 0.0f, 0.9f)),
    timeRemaining_(0.5f)
{
}

Emitter* Emitter::setTimeRemaining(float r)
{
    timeRemaining_ = r;
    return this;
}

bool Emitter::isDone() const 
{
    return timeRemaining_ < 0;
}

void Emitter::emit(std::list<Particle*>& particles, float dt) 
{

    // first decrease time remaining _on the emitter_.
    timeRemaining_ -= dt;

    int numNewParts = rate_ * dt;
    for (int j = 0; j < numNewParts; j++)     
    {
        Particle *p = new Particle();
        p->loc = pointOnSphere(radius_, loc_);
        p->t = normalRandom(lifetime_, 0.3);
        // Particle's velocity is the normal at that point
        // scaled by emitter's velocity value and given some
        // random nonsense
        p->vel = /*normalRandom(vel_, 1) */ vel_ * (p->loc - loc_) / radius_;
        p->size = glm::vec3(size_);
        p->color = color_;
        particles.push_back(p);


        /*
        std::cout << "Added particle with life " << p->t <<
            " and location: " << p->vel[0] << ' ' << p->vel[1]
            << ' ' << p->vel[2] << '\n';
            */
    }
}

Emitter* Emitter::setParticleVelocity(float r) 
{
    vel_ = r;
    return this;
}

Emitter* Emitter::setParticleLifetime(float l) 
{ 
    lifetime_ = l; 
    return this;
}

Emitter* Emitter::setParticleColor(glm::vec4 c) 
{ 
    color_ = c;
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


