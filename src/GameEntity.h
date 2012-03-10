#pragma once
#include <glm/glm.hpp>
#include <string>

class rectangle;
class Attack;

class GameEntity
{
public:
    GameEntity();
    virtual ~GameEntity();

    // Returns a string representing the type of GameEntity - should be unique
    // per subclass
    virtual std::string getType() const = 0;
    // ID functions
    // Returns a unique ID for this GameEntity
    virtual int getID() const { return id_; }
    // Returns the ID of the player that 'owns' this GameEntity or -1 if none
    virtual int getPlayerID() const { return playerID_; }
    // Returns the ID of the team this player is on
    virtual int getTeamID() const { return teamID_; }

    // Returns true if this GameEntity is no longer needed and should be
    // cleaned up
    virtual bool isDone() const = 0;

    // Accessor functions
    virtual const glm::vec2& getPosition() const { return pos_; }
    virtual const glm::vec2& getSize() const { return size_; }
    virtual const glm::vec2& getVelocity() const { return vel_; }
    virtual const glm::vec2& getAccel() const { return accel_; }

    // Returns the bounding box of this GameEntity
    virtual rectangle getRect() const;

    // Returns true if this GameEntity has a damaging attack
    virtual bool hasAttack() const = 0;
    // Returns the Attack
    virtual const Attack * getAttack() const = 0;

    // When false attacks will not ever connect, or even have their hit
    // methods called on this object
    virtual bool canBeHit() const = 0;
    // When two attacks hit each other- called on both GameEntities
    virtual void attackCollision(const Attack *other) = 0;
    // When this game entity is hit by an attack, and the owning entity
    // determined that this method should be called.  This method should then
    // deal with damage kb etc.
    virtual void hitByAttack(const Attack *attack) = 0;
    // When this game entity hits someone else with an attack, note
    // the other is non const
    virtual void attackConnected(GameEntity *other) = 0;

    // Called every frame with the ground rect and a boolean on whether or 
    // not this game entity hit the ground, if platform is true, the message
    // can be ignored
    virtual void collisionWithGround(const rectangle &ground, bool collision,
            bool platform) = 0;

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


    // Push the game entity by the passed vector
    void push(const glm::vec2 &vec);
    // Reflects the velocity of the entity over the y axis
    virtual void reflect();
    // Sets the player/team IDs for this entity, (may be ignored by subclasses)
    virtual void reown(int playerID, int teamID);

protected:
    glm::vec2 pos_, vel_, accel_;
    glm::vec2 size_;

    int id_, playerID_, teamID_;

private:
    static int lastID_;
};

class rectangle
{
public:
    rectangle();
    rectangle(float x, float y, float w, float h);

    bool overlaps(const rectangle &rhs) const;
    bool contains(const rectangle &rhs) const;

    float x, y, w, h;
};

