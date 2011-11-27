#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <list>
#include "Fighter.h"

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
    Emitter* setOutputRate(float r); // how many particles are output per second

    // 
    void emit(std::list<Particle*>, float dt);
private:
    Emitter() { }
    float lifetime_;
    glm::vec3 loc_;
    glm::vec3 vel_;
    float rate_;
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

