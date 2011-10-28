#pragma once
#include <GL/glew.h>

struct Controller
{
    float joyx, joyy;
    int buttona, buttonb, jumpbutton;
};

class Rectangle
{
public:
    Rectangle(float x, float y, float w, float h);

    bool isCollision(float x, float y, float w, float h);

    float x, y, w, h;
};

class Fighter
{
public:
    Fighter(const Rectangle &rect);
    ~Fighter();

    void update(const Controller&, float dt);
    void render(float dt);

    void collisionWithGround();
    const Rectangle& getRectangle();

private:
    // Game state members
    Rectangle rect_;
    float xvel_, yvel_;
    int state_;

    // Fighter stats
    const float walkSpeed_;
    const float airSpeed_;
    const float airAccel_;

    // Helper functions
};
