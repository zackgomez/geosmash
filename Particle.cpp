#include <iostream>
#include "Particle.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <glm/glm.hpp>
#include "glutils.h"

void Particle::update(float dt) 
{
    loc = loc + vel * dt;
    t -= dt;
}
    
void Particle::render() 
{
    glm::mat4 transform = glm::scale(
        glm::translate(
            glm::mat4(1.0f), loc), size);
    renderRectangle(transform, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}
