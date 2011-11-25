#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <list>
#include "Fighter.h"

class ParticleManager;
class Emitter 
{
public:
    Emitter() { }
    Emitter* setParticleLifetime(float l);
    Emitter* setLocation(glm::vec3 l);
    Emitter* setOutputRate(float r); // how many particles are output per second
private:
    float lifetime_;
    glm::vec3 loc_;
    glm::vec3 vel_;
    float rate_;
    friend class ParticleManager;
    friend class Particle;
};

struct Particle
{
    glm::vec3 loc;
    glm::vec3 vel;
    float t; // how much longer will it remain alive? 
    Emitter *emitter;

    void update(float dt);
};


class ParticleManager 
{
public:
    ParticleManager();
    static ParticleManager* get();

    void render(float dt);
    Emitter* newEmitter();
    void addEmitter(Emitter*);
private:
    std::vector<Emitter*> emitters;
    std::list<Particle*> particles;

};

