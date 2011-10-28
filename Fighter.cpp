#include "Fighter.h"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <GL/glew.h>
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>

const static int AIR_NORMAL_STATE = 0;
const static int AIR_STUNNED_STATE = 1;
const static int GROUND_STATE = 2;

Fighter::Fighter(const Rectangle &rect, const glm::vec3& color) :
    rect_(rect),
    xvel_(0), yvel_(0),
    state_(AIR_NORMAL_STATE),
    color_(color),
    walkSpeed_(300.0),
    airForce_(200.0),
    airAccel_(-500.0),
    jumpSpeed_(350.0)
{}

Fighter::~Fighter()
{}

void Fighter::update(const struct Controller &controller, float dt)
{
    // Update
    if (state_ == GROUND_STATE)
    {
        // Check for jump
        if (controller.jumpbutton || controller.joyy > 0.5f)
        {
            state_ = AIR_NORMAL_STATE;
            xvel_ = fabs(controller.joyx) > 0.2f ? controller.joyx * 0.5 * walkSpeed_ : 0.0f;
            yvel_ = jumpSpeed_;
        }
        else
        {
            // If no jump update 
            if (fabs(controller.joyx) > 0.2f)
                xvel_ = controller.joyx * walkSpeed_;
            else
                xvel_ = 0.0f;
        }
    }
    if (state_ == AIR_NORMAL_STATE)
    {
        // update for air normal state
        if (fabs(controller.joyx) > 0.2f)
            xvel_ += controller.joyx * airForce_ * dt;
        yvel_ += airAccel_ * dt;
    }

    // Update position
    rect_.x += xvel_ * dt;
    rect_.y += yvel_ * dt;
}

void Fighter::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (collision)
    {
        if (state_ != GROUND_STATE)
        {
            state_ = GROUND_STATE;
            xvel_ = yvel_ = 0;
        }
        // Make sure we're barely overlapping the ground (by 1 unit)
        rect_.y = ground.y + ground.h / 2 + rect_.h/2 - 1;
    }
    else
    {
        if (state_ == GROUND_STATE)
            state_ = AIR_NORMAL_STATE;
    }
}

const Rectangle& Fighter::getRectangle()
{
    return rect_;
}

void Fighter::respawn()
{
    rect_.x = 0.0f;
    rect_.y = -100.0f;
    xvel_ = yvel_ = 0.0f;
    state_ = AIR_NORMAL_STATE;
}

void Fighter::render(float dt)
{
    printf("State: %d   Position: [%f, %f]   Velocity: [%f, %f]\n", 
            state_, rect_.x, rect_.y, xvel_, yvel_);

    glm::mat4 transform(1.0);
    transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(rect_.x, rect_.y, 0.0)),
            glm::vec3(rect_.w, rect_.h, 1.0));

    renderRectangle(transform, color_);
}


Rectangle::Rectangle() :
    x(0), y(0), w(0), h(0)
{}

Rectangle::Rectangle(float xin, float yin, float win, float hin) :
    x(xin), y(yin), w(win), h(hin)
{}

bool Rectangle::overlaps(const Rectangle &rhs) const
{
    return (rhs.x + rhs.w/2) > (x - w/2) && (rhs.x - rhs.w/2) < (x + w/2) &&
        (rhs.y + rhs.h/2) > (y - h/2) && (rhs.y - rhs.h/2) < (y + h/2);
}
