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

    //attackCollision can be nop

    
    
    HazardEntity(const std::string &audioID);

private:
    float lifetime_;
    Attack *attack_;
    glm::vec2 pos_;
    glm::vec2 vel_;
    glm::vec2 size_;
};

