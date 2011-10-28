#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Controller
{
    float joyx, joyy;
    int buttona, buttonb, buttonc, jumpbutton;
};

class Rectangle
{
public:
    Rectangle();
    Rectangle(float x, float y, float w, float h);

    bool overlaps(const Rectangle &rhs) const;

    float x, y, w, h;
};

class Fighter
{
public:
    Fighter(const Rectangle &rect, const glm::vec3 &color);
    ~Fighter();

    void update(const Controller&, float dt);
    void render(float dt);

    void collisionWithGround(const Rectangle &ground, bool collision);
    const Rectangle& getRectangle();
    void respawn();

private:
    // Game state members
    Rectangle rect_;
    float xvel_, yvel_;
    int state_;
    glm::vec3 color_;

    // Fighter stats
    const float walkSpeed_;
    const float airForce_;
    const float airAccel_;
    const float jumpSpeed_;

    // Helper functions
};
