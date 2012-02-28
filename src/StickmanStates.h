#pragma once
#include "Fighter.h"
#include "FighterState.h"

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
