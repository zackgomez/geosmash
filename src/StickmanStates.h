#pragma once
#include "Fighter.h"
#include "FighterState.h"
#include "PManager.h"

class StickmanUpSpecial : public SpecialState
{
public:
    StickmanUpSpecial(Fighter *f, bool ground);
    ~StickmanUpSpecial();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    std::string pre_;
    glm::vec2 dir_;

    Emitter *emitter_;
};

class StickmanNeutralSpecial : public SpecialState
{
public:
    StickmanNeutralSpecial(Fighter *f, bool ground);
    ~StickmanNeutralSpecial();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    std::string pre_;
};

class CounterState : public SpecialState
{
public:
    CounterState(Fighter *f, bool ground);
    virtual ~CounterState() {}

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* collisionWithGround(const rectangle &ground, bool collision,
            bool platform);
    virtual FighterState* hitByAttack(const Attack *attack);

private:
    // How long have we been in this state?
    float t_;
    std::string pre_;
    bool playedSound_;
};


class StickmanSideSpecial : public SpecialState
{
public:
    StickmanSideSpecial(Fighter *f, bool ground);
    ~StickmanSideSpecial();

    virtual FighterState* processInput(controller_state&, float dt);
    virtual void render(float dt);
    virtual FighterState* hitByAttack(const Attack *attack);
    virtual FighterState* attackConnected(GameEntity *victim);
    virtual FighterState* attackCollision(const Attack *other);

private:
    std::string pre_;
    bool pushed_;
};

