#pragma once
#include "FighterState.h"


class UpSpecialState : public SpecialState
{
public:
    UpSpecialState(Fighter *f, bool ground);
    virtual ~UpSpecialState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);

private:
    std::string pre_;
};

class DashSpecialState : public SpecialState
{
public:
    DashSpecialState(Fighter *f, bool ground);
    virtual ~DashSpecialState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);

    virtual void update(float dt);

private:
    std::string pre_;
};

class CharlieNeutralSpecial : public SpecialState
{
public:
    CharlieNeutralSpecial(Fighter *f, bool ground);

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);

private:
    std::string pre_;
    float t_;
    bool shot_;
};

