#pragma once
#include <vector>

class Explosion
{
public:
    Explosion(float x, float y, float dur, const glm::vec3& col, float size) :
        x_(x), y_(y), t_(0), duration_(dur), color_(col), size_(size)
    {}

    void render(const glm::mat4& trans, float dt);
    bool isDone() const;

private:
    float x_, y_;
    float t_;
    float duration_;
    glm::vec3 color_;
    float size_;
};

class Twinkle
{
public:
    Twinkle(float x, float y, float dur, float size) :
        x_(x), y_(y), t_(0), duration_(dur), size_(size)
    {}

    void render(const glm::mat4& trans, float dt);
    bool isDone() const;

private:
    float x_, y_;
    float t_;
    float duration_;
    float size_;
};

class ExplosionManager
{
public:
    static ExplosionManager* get();

    // Adds an explosion at game coordinates x,y with duration t
    void addExplosion(float x, float y, float t);

    // Adds a colored 'puff' for dashing or landing
    void addPuff(float x, float y, float t);

    // Adds a glowy twinkle for specifying strong hits
    void addTwinkle(float x, float y);

    // Renders all explosions on the screen
    void render(const glm::mat4& trans, float dt);

private:
    // Private shits for singleton
    ExplosionManager();
    ExplosionManager(const ExplosionManager&);

    std::vector<Explosion> explosions_;
    std::vector<Twinkle> twinkles_;
};
