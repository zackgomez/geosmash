#define _USE_MATH_DEFINES
#include "Emitter.h"
#include <cstdio>
#include <iostream>


lifetimeVarianceF::lifetimeVarianceF(float variance) 
    : var_(variance)
{ }

float lifetimeVarianceF::operator () (float f)
{
    return normalRandom(f, var_); 
}

velocityVarianceF::velocityVarianceF(float variance) 
    : var_(variance)
{ }

float velocityVarianceF::operator () (float f)
{
    return normalRandom(f, var_); 
}
// Approximate a normal variable.
// http://en.wikipedia.org/wiki/Normal_distribution#Generating_values_from_normal_distribution
// A sum of several uniform variables is like a normal distr.
// (central limit theorom)
float normalRandom(float mu, float sigma)
{
    float ans = 0.f;
    for (int i = 0; i < 6; i++)
    {
        ans += static_cast<float>(rand()) / RAND_MAX;
    }
    return mu + (ans - 3) * sigma;
}

void test_random() {
    float s1 = 1;
    float mu = 7;
    float sum = 0.f;
    for (int i = 0; i < 100000; i++)
    {
        sum += normalRandom(s1, mu);
    }
    sum /= 100000;
    printf("Average is %f\n", sum);

    s1 = 10;
    mu = -100;
    for (int i = 0; i < 10; i++)
    {
        printf("normalRandom(mu = %f, sigma = %f) = %f\n", mu, s1, normalRandom(mu, s1));
    }
}

glm::vec3 pointOnSphere(float r, const glm::vec3 &pos)
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
    lifetime_func(NULL),
    vel_(50.f),
    velocity_func(NULL),
    loc_(glm::vec3(0.f)),
    radius_(2.f),
    rate_(400.f),
    size_(1.f),
    color_(glm::vec4(1.0f, 0.0f, 0.0f, 0.9f)),
    colorvar_(0.f),
    colorbright_(1.0f), colorbrightvar_(0.f),
    timeRemaining_(HUGE_VAL)
{
    setParticleLifetimeF(new lifetimeF());
    setParticleVelocityF(new velocityF());
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
        p->t = (*lifetime_func)(lifetime_);
        // Particle's velocity is the normal at that point
        // scaled by emitter's velocity value and given some
        // random nonsense
        p->vel = normalRandom(vel_, 1) * (p->loc - loc_) / radius_;
        p->size = glm::vec3(size_);

        glm::vec4 colordelta = glm::vec4(
                normalRandom(1.f, colorvar_.r),
                normalRandom(1.f, colorvar_.g),
                normalRandom(1.f, colorvar_.b),
                1.f);
        colordelta = glm::max(colordelta, 0.f);
        p->color = glm::clamp(color_ * colordelta, 0.f, 1.f);
        p->color *= normalRandom(colorbright_, colorbrightvar_);
        particles.push_back(p);
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

Emitter* Emitter::setParticleLifetimeF(lifetimeF *lf)
{
    if (lifetime_func)
    {
        delete lifetime_func;
    }
    lifetime_func = lf;
    return this;
}

Emitter* Emitter::setParticleVelocityF(velocityF *vf)
{
    if (velocity_func)
    {
        delete velocity_func;
    }
    velocity_func = vf;
    return this;
}

Emitter* Emitter::setParticleColor(const glm::vec4 &c) 
{ 
    color_ = c;
    return this;
}

Emitter* Emitter::setParticleColorVariance(const glm::vec4 &c) 
{ 
    colorvar_ = c;
    return this;
}

Emitter* Emitter::setParticleColorBrightness(float mu, float sigma)
{
    colorbright_ = mu;
    colorbrightvar_ = sigma;
    return this;
}

Emitter* Emitter::setRadius(float r)
{
    radius_ = r;
    return this;
}

Emitter* Emitter::setLocation(const glm::vec3 &l) 
{ 
    loc_ = l; 
    return this;
}

Emitter* Emitter::setOutputRate(float r)
{
    rate_ = r;
    return this;
}


