#include "GameEntity.h"
#include "Fighter.h"

int GameEntity::lastID_ = 100;

GameEntity::GameEntity() :
    pos_(), vel_(), accel_(), size_(), id_(lastID_++), playerID_(-1)
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

void GameEntity::push(const glm::vec2 &vec)
{
    pos_ += vec;
}
