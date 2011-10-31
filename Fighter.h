#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

struct Controller
{
    // The positions [-1, 1] of the main analog stick
    float joyx, joyy;
    // The velocities of the main analog stick over some time period
    float joyxv, joyyv;

    // nonzero if the button is pressed
    int buttona, buttonb, buttonc, jumpbutton;

    // nonzero if the button was pressed this frame
    int pressa, pressb, pressc, pressjump;
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
    Fighter(const Rectangle &rect, float respawnx, float respawny, const glm::vec3 &color);
    ~Fighter();

    void update(const Controller&, float dt);
    void render(float dt);

    int getLives() const;
    float getDamage() const;

    void collisionWithGround(const Rectangle &ground, bool collision);
    void attackCollision(); // Called when two attacks collide
    void hitByAttack(const Rectangle &hitbox);  // Called when hit by an attack
    void hitWithAttack(); // Called when you hit with an attack
    const Rectangle& getRectangle() const;

    // Returns true if this Fighter is currently attacking and has an attack
    // hitbox (see getAttackBox())
    bool hasAttack() const;
    Rectangle getAttackBox() const;

    void respawn(bool killed);
    bool isAlive() const;

private:
    // Game state members
    Rectangle rect_;
    float xvel_, yvel_;
    float dir_; // 1 or -1 look in xdir
    int state_;
    float stunTime_, stunDuration_;
    float damage_;
    int lives_;

    // Fighter ID members
    float respawnx_, respawny_;
    glm::vec3 color_;

    float jumpTime_; // amount of time since jump inputted
    bool canSecondJump_;
    

    // Attack members
    float attackTime_; // -1 when not attacking
    bool attackHit_; // True if current attack animation has hit
    const float attackStartup_, attackDuration_, attackCooldown_;
    const float attackDamage_, attackKnockback_, attackStun_; // attack stun in seconds
    // Attack hitbox description
    const float attackX_, attackY_;
    const float attackW_, attackH_;

    // Fighter stats
    const float walkSpeed_; // Maximum walking speed
    const float airForce_; // Force applied to allow player air control
    const float airAccel_; // "Gravity"
    const float jumpStartupTime_; // Delay before jump begins, also short hop/full jump control time
    const float jumpSpeed_; // Speed of a full jump
    const float hopSpeed_; // Speed of a short hop
    const float jumpAirSpeed_; // The maximum x speed for jumping (only for player control)
    const float secondJumpSpeed_; // Speed of the second jump

    // Helper functions
    float damageFunc() const; // Returns a scaling factor based on damage
    bool inAttackAnimation() const;
};
