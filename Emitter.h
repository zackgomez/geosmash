#pragma once
#include "Particle.h"
#include <list>
#include <glm/glm.hpp>

float normalRandom(float mu, float sigma);

// Forward declaration necessary for friending
class ParticleManager;

////////////////////////////////////////////////////
// Functors
/////////////////////////////////////////////////////

// The generic, simple lifetime function
struct lifetimeF
{
    float operator () (float f) { return f; }
};

struct lifetimeVarianceF : lifetimeF
{
    lifetimeVarianceF(float variance);
    float operator () (float f);
private:
    float var_;
};

// Simpleton velocity function
struct velocityF
{
    float operator() (float v) { return v; }
};

struct velocityVarianceF : velocityF
{
    velocityVarianceF(float f);
    float operator() (float v);
private:
    float var_;
};

//
// The emitter class is the main particle engine configuration point.
// With its setter methods particle lifetime, location, and spew rate
// can be defined.
// Emitters will spew particles (and take up clock cycles) until a call to
// ParticleManager::quashEmitter()
//
class Emitter 
{
public:
    Emitter* setParticleLifetime(float);
    Emitter* setParticleLifetimeF(lifetimeF *);

    Emitter* setParticleVelocity(float r);
    Emitter* setParticleVelocityF(velocityF *);

    Emitter* setLocation(const glm::vec3 &l);
    Emitter* setOutputRate(float r);
    Emitter* setRadius(float r);
    // How much time is left before this emitter expires?
    Emitter* setTimeRemaining(float);

    // Set particle color. Fourth component is glow, not opacity.
    Emitter* setParticleColor(const glm::vec4 &);
    Emitter* setParticleColorVariance(const glm::vec4 &);
    Emitter* setParticleColorBrightness(float mu, float sigma);

    bool isDone() const;
    // The update function. Spew some new particles, given that dt seconds
    // have elapsed.
    void emit(std::list<Particle*>&, float dt);
private:
    Emitter();

    // Lifetime, and a functor that determines particle lifetime
    float lifetime_;
    lifetimeF *lifetime_func;

    // particle velocity, and its associated functor
    float vel_;
    velocityF *velocity_func;

    // Since this is a sperical emitter, we need two pieces of information:
    glm::vec3 loc_;
    // ... and ...
    float radius_;
    // Particles will spawn with this initial velocity
    // And with a frequency determined by this (particles per second)
    float rate_;
    // Each particle will be about this size.
    glm::vec3 size_;

    glm::vec4 color_, colorvar_;
    float colorbright_, colorbrightvar_;

    float timeRemaining_;

    friend class ParticleManager;
};


