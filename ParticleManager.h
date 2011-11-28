#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <list>
#include "Fighter.h"

glm::vec3 pointOnSphere(float r, glm::vec3 pos);  
float normalRandom(float mu, float sigma);
// Forward declarations necessary for friending.
class ParticleManager;
class Particle;

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
    Emitter* setLocation(glm::vec3 l);
    Emitter* setOutputRate(float r);
    // A measure of 'how random' particles coming off are.
    // Note, this isn't actual variance!
    Emitter* setParticleVelocityVariance(float r);
    Emitter* setRadius(float r);
    // 
    void emit(std::list<Particle*>, float dt);
private:
    Emitter() { }

    float lifetime_;

    // A measure of how variable the particle velocities are
    float var_;
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
    friend class ParticleManager;
    friend class Particle;
};

struct Particle
{
    // Location of the particle, in world space
    glm::vec3 loc;
    // How fast it's moving.
    glm::vec3 vel;
    // Size it should be rendered as. (currently, all particles are squares)
    glm::vec3 size;
    // How much longer will it remain alive? 
    float t; 
    // Parent of this particle. Maybe we can get rid of this pointer.
    Emitter *emitter;

    // Physics calculations are performed here.
    void update(float dt);
    // Just create a matrix and render this shit to the screen.
    void render();
};


class ParticleManager 
{
public:
    ParticleManager();
    // Get() access to the singleton instance
    static ParticleManager* get();

    void render(float dt);
    Emitter* newEmitter();
    // 
    void addEmitter(Emitter*);
    // Get rid of this emitter.
    void quashEmitter(Emitter*);

private:
    std::vector<Emitter*> emitters_;
    std::list<Particle*> particles_;

};

