#pragma once
#include <vector>

class Twinkle
{
public:
    Twinkle(float x, float y, float dur, float size) :
        x_(x), y_(y), t_(0), duration_(dur), size_(size)
    {}

    void render(float dt);
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
    void render(float dt);

private:
    // Private shits for singleton
    ExplosionManager();
    ExplosionManager(const ExplosionManager&);

    std::vector<Twinkle> twinkles_;
};
