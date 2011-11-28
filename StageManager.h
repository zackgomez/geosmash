#pragma once
#include <glm/glm.hpp>
#include "GameEntity.h"
#include "Attack.h"
class StageManager
{

    // Return the singleton instance
    StageManager* get();

    // Called every frame.
    // For now, just put out a stage hazard (maybe)
    void update(float dt);



};


class HazardEntity : public GameEntity 
{

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

