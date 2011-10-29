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

    float jumpTime_; // amount of time since last jump
    

    // Attack members
    float attackTime_; // -1 when not attacking
    bool attackHit_; // True if current attack animation has hit
    const float attackStartup_, attackDuration_, attackCooldown_;
    const float attackDamage_, attackKnockback_, attackStun_; // attack stun in seconds
    // Attack hitbox description
    const float attackX_, attackY_;
    const float attackW_, attackH_;

    // Fighter stats
    const float walkSpeed_;
    const float airForce_;
    const float airAccel_;
    const float jumpSpeed_;
    const float jumpAirSpeed_; // The maximum x speed for jumping (only for player control)
    //TODO: const float jumpInterval_; // length of time disalowed from second jump

    // Helper functions
    float damageFunc() const; // Returns a scaling factor based on damage
    bool inAttackAnimation() const;
};
