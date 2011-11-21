#include "GameEntity.h"
#include "Fighter.h"

GameEntity::GameEntity() :
    pos_(), vel_(), accel_(), size_()
{
    /* Empty */
}

GameEntity::~GameEntity()
{
    /* Empty */
}

Rectangle GameEntity::getRect() const
{
    return Rectangle(pos_.x, pos_.y, size_.x, size_.y);
}

void GameEntity::update(float dt)
{
    // Simple Euler integration...
    vel_ += accel_ * dt;
    pos_ += vel_ * dt;
}
