#pragma once
#include <vector>

struct explosion
{
    explosion(float xx, float yy, float dur) : x(xx), y(yy), t(0), duration(dur) {}
    float x, y;
    float t;
    float duration;
};

class ExplosionManager
{
public:
    static ExplosionManager* get();

    // Adds an explosion at game coordinates x,y with duration t
    void addExplosion(float x, float y, float t);

    // Renders all explosions on the screen
    void render(float dt);

private:
    // Private shits for singleton
    ExplosionManager();
    ExplosionManager(const ExplosionManager&);

    std::vector<explosion> explosions_;
};
