#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "glutils.h"
#include "FrameManager.h"
#include "ParticleManager.h"

void Explosion::render(float dt)
{
    t_ += dt;

    float frac = std::min(1.0f, t_ / duration_);
    glm::mat4 transform = 
        glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(x_, y_, -0.1)),
                frac * glm::vec3(size_, size_, 1.0f));
    renderRectangle(transform, glm::vec4(color_, 0.2));
}

bool Explosion::isDone() const
{
    return t_ > duration_;
}

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
    //explosions_.push_back(Explosion(x, y, t, glm::vec3(1.0f, 0.42f, 0.0f), 30.0f));
    Emitter *em = ParticleManager::get()->newEmitter();
    em->setLocation(glm::vec3(x, y, 0.f))
        ->setTimeRemaining(t)
        ->setParticleLifetime(0.03)
        ->setOutputRate(1000);
    //em->setEmitterLifetime(t);
    ParticleManager::get()->addEmitter(em);
}

void ExplosionManager::addPuff(float x, float y, float t)
{
    explosions_.push_back(Explosion(x, y, t, glm::vec3(0.8f, 0.8f, 0.8f), 20.0f));
}

void ExplosionManager::addTwinkle(float x, float y)
{
    twinkles_.push_back(Twinkle(x, y, 0.25, 1.0f));
}

void ExplosionManager::render(float dt)
{
    std::vector<Explosion>::iterator it = explosions_.begin();
    for (; it != explosions_.end(); )
    {
        Explosion& ex = *it;
        // Render
        ex.render(dt);
        // Check for explosion death
        if (ex.isDone())
            it = explosions_.erase(it);
        else
            it++;
    }

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
