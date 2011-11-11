#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "glutils.h"

void Explosion::render(float dt)
{
    t_ += dt;

    float frac = std::min(1.0f, t_ / duration_);
    glm::mat4 transform = 
        glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(x_, y_, 0.0)),
                frac * glm::vec3(size_, size_, 1.0f));
    renderRectangle(transform, glm::vec4(color_, 0.2));
}

bool Explosion::isDone() const
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
    explosions_.push_back(Explosion(x, y, t, glm::vec3(1.0f, 0.42f, 0.0f), 30.0f));
}

void ExplosionManager::addPuff(float x, float y, float t)
{
    explosions_.push_back(Explosion(x, y, t, glm::vec3(0.8f, 0.8f, 0.8f), 20.0f));
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
}
