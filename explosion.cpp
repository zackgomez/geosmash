#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "glutils.h"
#include "FrameManager.h"
#include "ParticleManager.h"

void Twinkle::render(float dt)
{
    t_ += dt;
    glm::mat4 transform =
        glm::rotate(
                glm::scale(
                    glm::translate(glm::mat4(1.0f), glm::vec3(x_, y_, -0.1)),
                    glm::vec3(size_, size_, 1.0f)),
                static_cast<float>(45.f),
                glm::vec3(0,0,1));
    FrameManager::get()->renderFrame(transform, glm::vec4(.7, .7 ,.7, .3),
            "StrongAttackInd");
}

bool Twinkle::isDone() const
{
    return t_ > duration_;
}

ExplosionManager::ExplosionManager()
{}

ExplosionManager* ExplosionManager::get()
{
    static ExplosionManager em;
    return &em;
}

void ExplosionManager::addExplosion(float x, float y, float t)
{
    glm::vec4 pcolors_raw[] =
    {
        glm::vec4(0.9, 0.1, 0.0, 0.9),
        glm::vec4(0.9, 0.1, 0.0, 0.9),
        glm::vec4(0.9, 0.1, 0.0, 0.9),
        glm::vec4(0.9, 0.1, 0.0, 0.9),
        glm::vec4(0.9, 0.1, 0.0, 0.9),
        glm::vec4(0.9, 0.25, 0.0, 0.9),
        glm::vec4(0.9, 0.6, 0.0, 0.9),
    };
    std::vector<glm::vec4> pcolors;
    pcolors.assign(pcolors_raw, pcolors_raw + sizeof(pcolors_raw) / sizeof(glm::vec4));

    Emitter *em = ParticleManager::get()->newEmitter();
    em->setLocation(glm::vec3(x, y, 0.f))
        ->setTimeRemaining(t)
        ->setParticleLifetimeF(new lifetimeNormalF(0.03, 0.3))
        ->setParticleVelocityF(new velocityF(30.f, 30.f, 10.f))
        ->setParticleLocationF(new locationF(7.f))
        ->setParticleColorF(new discreteColorF(pcolors))
        ->setOutputRate(2000);
    ParticleManager::get()->addEmitter(em);
}

void ExplosionManager::addPuff(float x, float y, float t)
{
    Emitter *em = ParticleManager::get()->newEmitter();
    em->setLocation(glm::vec3(x, y, 0.f))
        ->setTimeRemaining(t/2)
        ->setParticleLifetimeF(new lifetimeNormalF(0.3, 0.3))
        ->setParticleVelocityF(new velocityF(20.f, 20.f, 5.f))
        ->setParticleLocationF(new locationF(5.f))
        ->setParticleColorF(new colorF(glm::vec4(0.8, 0.8, 0.8, 0.5), 0.8, 0.2))
        ->setOutputRate(1000);
    ParticleManager::get()->addEmitter(em);
}

void ExplosionManager::addTwinkle(float x, float y)
{
    twinkles_.push_back(Twinkle(x, y, 0.25, 1.0f));
}

void ExplosionManager::render(float dt)
{
    std::vector<Twinkle>::iterator tit = twinkles_.begin();
    for (; tit != twinkles_.end(); )
    {
        Twinkle& twinkle = *tit;
        // Render
        twinkle.render(dt);
        // Check for explosion death
        if (twinkle.isDone())
            tit = twinkles_.erase(tit);
        else
            tit++;
    }
}
