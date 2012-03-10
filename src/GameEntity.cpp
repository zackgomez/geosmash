#include "GameEntity.h"
#include "Fighter.h"

int GameEntity::lastID_ = 100;

GameEntity::GameEntity() :
    pos_(), vel_(), accel_(), size_(), id_(lastID_++), playerID_(-1), teamID_(-1)
{
    /* Empty */
}

GameEntity::~GameEntity()
{
    /* Empty */
}

rectangle GameEntity::getRect() const
{
    return rectangle(pos_.x, pos_.y, size_.x, size_.y);
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

void GameEntity::reflect()
{
    vel_.x *= -1;
}

void GameEntity::reown(int playerID, int teamID)
{
    playerID_ = playerID;
    teamID_ = teamID;
}

// ----------------------------------------------------------------------------
// Rectangle class methods
// ----------------------------------------------------------------------------
rectangle::rectangle() :
    x(0), y(0), w(0), h(0)
{}

rectangle::rectangle(float xin, float yin, float win, float hin) :
    x(xin), y(yin), w(win), h(hin)
{}

bool rectangle::overlaps(const rectangle &rhs) const
{
    return (rhs.x + rhs.w/2) > (x - w/2) && (rhs.x - rhs.w/2) < (x + w/2) &&
        (rhs.y + rhs.h/2) > (y - h/2) && (rhs.y - rhs.h/2) < (y + h/2);
}

bool rectangle::contains(const rectangle &rhs) const
{
    return (rhs.x - rhs.w/2) > (x - w/2) && (rhs.x + rhs.w/2) < (x + w/2) &&
        (rhs.y - rhs.h/2) > (y - h/2) && (rhs.y + rhs.h/2) < (y + h/2);
}

