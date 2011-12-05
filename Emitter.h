#pragma once
#include "Particle.h"
#include <list>
#include <glm/glm.hpp>

// Forward declaration necessary for friending
class ParticleManager;

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
    Emitter* setParticleLifetime(float l);
    Emitter* setParticleLifetimeVariance(float);
    Emitter* setLocation(const glm::vec3 &l);
    Emitter* setOutputRate(float r);
    // A measure of 'how random' particles coming off are.
    // Note, this isn't actual variance!
    Emitter* setParticleVelocityVariance(float r);
    Emitter* setParticleVelocity(float r);
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

    float lifetime_;

    // A measure of how variable the particle velocities are
    float var_;
    float lifetimeVar_;
    // Since this is a sperical emitter, we need two pieces of information:
    glm::vec3 loc_;
    // ... and ...
    float radius_;
    // Particles will spawn with this initial velocity
    float vel_;
    // And with a frequency determined by this (particles per second)
    float rate_;
    // Each particle will be about this size.
    glm::vec3 size_;

    glm::vec4 color_, colorvar_;
    float colorbright_, colorbrightvar_;

    float timeRemaining_;

    friend class ParticleManager;
};


