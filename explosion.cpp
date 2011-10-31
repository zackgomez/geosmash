#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "explosion.h"
#include "glutils.h"

ExplosionManager::ExplosionManager()
{}

ExplosionManager* ExplosionManager::get()
{
    static ExplosionManager em;
    return &em;
}

void ExplosionManager::addExplosion(float x, float y, float t)
{
    explosions_.push_back(explosion(x, y, t));
}

void ExplosionManager::render(float dt)
{
    std::vector<explosion>::iterator it = explosions_.begin();
    for (; it != explosions_.end(); )
    {
        explosion& ex = *it;
        // Check for explosion death
        if ((ex.t += dt) >= ex.duration)
        {
            it = explosions_.erase(it);
        }
        else
        {
            float frac = ex.t / ex.duration;
            glm::mat4 transform = 
                glm::scale(
                        glm::translate(glm::mat4(1.0f), glm::vec3(ex.x, ex.y, 0.0)),
                        frac * glm::vec3(30.0f, 30.0f, 1.0f));
            renderRectangle(transform, glm::vec3(1.0f, 0.42, 0.0f));
            // Next explosion
            it++;
        }
    }
}
