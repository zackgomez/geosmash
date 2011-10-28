#include "Fighter.h"
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>

const static int AIR_NORMAL_STATE = 0;
const static int AIR_STUNNED_STATE = 1;
const static int GROUND_STATE = 2;

Fighter::Fighter(const Rectangle &rect) :
    rect_(rect),
    xvel_(0), yvel_(0),
    state_(GROUND_STATE),
    walkSpeed_(1.0),
    airSpeed_(0.0),
    airAccel_(0.0)
{}

Fighter::~Fighter()
{}

void Fighter::update(const struct Controller &controller, float dt)
{
    // Clean up controller input
    // TODO

    // Update
    if (state_ == GROUND_STATE)
    {
        // Check for jump
        // TODO

        // If no jump update 
        if (fabs(controller.joyx) > 0.2f)
            xvel_ = controller.joyx * walkSpeed_;
        else
            xvel_ = 0.0f;
    }
    else
    {
        // update for air normal state
        // TODO
    }

    // Update position
    rect_.x += xvel_ * dt;
    rect_.y += yvel_ * dt;
}

void Fighter::collisionWithGround()
{
    if (state_ != GROUND_STATE)
    {
        state_ = GROUND_STATE;
        xvel_ = yvel_ = 0;
    }
}

void Fighter::render(float dt)
{
    printf("Position: [%f, %f]   Velocity: [%f, %f]\n", 
            rect_.x, rect_.y, xvel_, yvel_);

    glm::vec3 color(1.0, 1.0, 1.0);
    glm::mat4 transform(1.0);
    transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(0.1, 0.1, 1.0));
    renderRectangle(transform, color);
}


Rectangle::Rectangle(float xin, float yin, float win, float hin) :
    x(xin), y(yin), w(win), h(hin)
{}

bool Rectangle::isCollision(float x, float y, float w, float h)
{
    // TODO
    return false;
}
