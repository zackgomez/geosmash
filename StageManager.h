#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "GameEntity.h"
#include "Attack.h"

struct Ledge
{
    glm::vec2 pos;
    bool occupied;
};

class StageManager
{
public:
    // Return the singleton instance
    static StageManager* get();

    // Called every frame.
    // For now, just put out a stage hazard (maybe)
    void update(float dt);

    // Cleans up and restores this stage manager to default state
    void clear();

    // Returns the nearest non occupied ledge, or null
    Ledge * getPossibleLedge(const glm::vec2 &pos);

private:
    StageManager();

    std::vector<Ledge*> ledges_;
};


class HazardEntity : public GameEntity 
{
public:
    virtual bool isDone() const;
    virtual bool hasAttack() const { return true; }
    virtual const Attack * getAttack() const;
    virtual bool canBeHit() const { return false; }

    virtual void update(float dt);

    virtual void render(float dt);
    // Someone hit us. Shake in our boots.
    virtual void attackCollision(const Attack*);
    // We hit someone! Fuck 'em up!
    virtual void attackConnected(GameEntity*);
    // XXX what happens here?
    virtual void collisionWithGround(const Rectangle&, bool);
    virtual void hitByAttack(const Attack*);

    //attackCollision can be nop
    
    HazardEntity(const std::string &audioID);

private:
    float lifetime_;
    SimpleAttack *attack_;
    std::string frameName_;
    std::string pre_;
    int dir_;
    float t_;
};

