#pragma once
#include <glm/glm.hpp>

class Rectangle;
class Attack;

class GameEntity
{
public:
    GameEntity();
    virtual ~GameEntity();

    // ID functions
    // Returns a unique ID for this GameEntity
    virtual int getID() const { return id_; }
    // Returns the ID of the player that 'owns' this GameEntity or -1 if none
    virtual int getPlayerID() const { return playerID_; }

    // Accessor functions
    virtual const glm::vec2& getPosition() const { return pos_; }
    virtual const glm::vec2& getSize() const { return size_; }
    virtual const glm::vec2& getVelocity() const { return vel_; }
    virtual const glm::vec2& getAccel() const { return accel_; }

    // Returns the bounding box of this GameEntity
    virtual Rectangle getRect() const;

    // Returns true if this GameEntity has a damaging attack
    virtual bool hasAttack() const = 0;
    // Returns the Attack
    virtual const Attack * getAttack() const = 0;

    // When two attacks hit each other- called on both GameEntities
    virtual void attackCollision(const Attack *other) = 0;
    // When this game entity is hit by an attack
    virtual void hitByAttack(const Attack *attack) = 0;
    // When this game entity hits someone else with an attack, note
    // the other is non const
    virtual void attackConnected(GameEntity *other) = 0;

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


    // Mutators
    void setPlayerID(int playerID);

    // Push the game entity by the passed vector
    void push(const glm::vec2 &vec);

protected:
    glm::vec2 pos_, vel_, accel_;
    glm::vec2 size_;

    int id_, playerID_;

    static int lastID_;
};
