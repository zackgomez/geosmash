#pragma once
#include <glm/glm.hpp>

class Rectangle;

class GameEntity
{
public:
    GameEntity();
    virtual ~GameEntity();

    // Accessor functions
    virtual const glm::vec2& getPosition() const { return pos_; }
    virtual const glm::vec2& getSize() const { return size_; }
    virtual const glm::vec2& getVelocity() const { return vel_; }
    virtual const glm::vec2& getAccel() const { return accel_; }
    // Returns the bounding box of this GameEntity
    virtual Rectangle getRect() const;

    /*
     * This function performs some sort of integration to update this
     * GameEntity.  The position, etc are integrated with timestep dt.
     * Other actions could be performed by overriding this function
     */
    virtual void update(float dt);

    /*
     * Render this game entity, dt is time from last call to render.
     * Must be overriden, making GameEntity abstract
     */
    virtual void render(float dt) = 0;

protected:
    glm::vec2 pos_, vel_, accel_;
    glm::vec2 size_;
};
