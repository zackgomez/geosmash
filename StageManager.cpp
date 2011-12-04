#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Fighter.h"
#include "FrameManager.h"

StageManager::StageManager()
{
    glm::vec2 groundpos(getParam("level.x"), getParam("level.y"));
    glm::vec2 groundsize(getParam("level.w"), getParam("level.h"));
    Ledge l;
    l.pos = glm::vec2(groundpos.x - groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    ledges_.push_back(new Ledge(l));

    l.pos = glm::vec2(groundpos.x + groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    ledges_.push_back(new Ledge(l));
}

StageManager *StageManager::get()
{
    static StageManager sm;
    return &sm;
}

Ledge * StageManager::getPossibleLedge(const glm::vec2 &pos)
{
    Ledge *ret = NULL;
    float mindist = HUGE_VAL;
    for (unsigned i = 0; i < ledges_.size(); i++)
    {
        Ledge *l = ledges_[i];
        float dist = glm::length(pos - l->pos);
        if (!l->occupied && dist < mindist)
        {
            ret = l;
            mindist = dist;
        }
    }

    return ret;
}

//////////////////////////////////////////////
// Background Sphere
//////////////////////////////////////////////

BackgroundSphere::BackgroundSphere()
{
    std::string pre_ = "backgroundSphere.radius";
    radius_ = getParam(pre_ + "radius");
    lineCount_ = getParam(pre_ + "lineCount");
    pulseCount_ = getParam(pre_ + "pulseCount");
    divisionCount_ = getParam(pre_ + "divisionCount");
}

void BackgroundSphere::render(float dt)
{
    updateLitSegments();
    renderLatitude();
    renderLongitude();
}

void BackgroundSphere::updateLitSegments(void)
{
    // TODO: Move the lit parts around the sphere somehow.
    // NOTE: we may need to add a class for the pulses.
}

void BackgroundSphere::renderLongitude(void)
{
    // TODO: Loop over each longitudinal line, similar to renderLatitude().
}

void BackgroundSphere::renderLatitude(void)
{
    // Sweep over each lat. line, drawing segments between points 
    float r = radius_;
    float theta = 0; // elevation
    float phi = 0;   // azimuth

    // First draw latitude lines
    // For each of these, elevation (theta) is constant
    for (int i = 0; i < lineCount_; i++)
    {
        // Set current elevation angle
        theta = M_PI * lineCount_ - (M_PI / 2);
        phi = 0;

        // Now, go in a circle adding line segments (phi goes from 0 --> 2pi)
        for (int segi = 0; segi < divisionCount_; segi++)
        {
            // TODO: fix these constructors. Can we use glm like 
            // "glm::vec3 a(0,1,2);"? Didn't work for me. Unary operand error.
            glm::vec3 segStart, segEnd;
            segStart.x = r * sin(theta) * cos(phi);
            segStart.y = r * sin(theta) * sin(phi);
            segStart.z = r * cos(theta);
            phi += (2 * M_PI) / divisionCount_; 
            segEnd.x = r * sin(theta) * cos(phi);
            segEnd.y = r * sin(theta) * sin(phi);
            segEnd.z = r * cos(theta);
            // Now, render a rectangle connecting these two points

            // We can fudge by taking the midpoint.
            // We can fudge further by just drawing one at the beginning point.
            
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f), segStart), glm::vec3(2.0f));
            // glm::length(segStart - segEnd))// Use this instead of 2.0f;
            renderRectangle(transform, glm::vec4(0.5, 0.5, 0.5, 0.5));

            // now, check to see if we're close to a lit segments.
        }
    }
}

//////////////////////////////////////////////
// Hazard  
//////////////////////////////////////////////

bool HazardEntity::isDone() const
{
    return false; 
}
void HazardEntity::update(float dt)
{
    // Each frame, we only need to do a couple things.
    // First, move a bit randomly
    // then, update our attack to reflect our location.

    // change direction every now and then:
    dir_ = rand() % static_cast<int>(getParam(pre_ + "switchRate")) ? dir_ : -dir_;
    vel_.x = getParam(pre_ + "speed") * dir_;


    t_ += dt;
    if (t_ > getParam(pre_ + "cooldown")) 
    {
        attack_->clearHit();
        t_ = 0;
    }

    GameEntity::update(dt);
    attack_->setPosition(pos_);
}

void HazardEntity::render(float dt)
{
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
            glm::vec3(1.f));
#define HAZARD_COLOR 0.5, 0.5, 1
    FrameManager::get()->renderFrame(transform, glm::vec4(glm::vec4(HAZARD_COLOR, 0.3)),
            frameName_);
}

HazardEntity::HazardEntity(const std::string &audioID) 
{
    pre_ = "stageHazardAttack.";

    frameName_ = "Hazard";
    pos_ = glm::vec2(0, getParam("level.y") + getParam("level.h") / 2 + getParam(pre_ + "sizey") / 2 - 1);
    size_ = glm::vec2(getParam(pre_ + "sizex"), getParam(pre_ + "sizey"));
    lifetime_ = getParam(pre_ + "lifetime");
    dir_ = 1; // initially, we'll go right.

    attack_ = new SimpleAttack(
            getParam(pre_ + "knockbackpow") *
            glm::normalize(glm::vec2(getParam(pre_ + "knockbackx"),
                      getParam(pre_ + "knockbacky"))),
            getParam(pre_ + "damage"),
            getParam(pre_ + "stun"),
            getParam(pre_ + "priority"),
            pos_, size_, playerID_,
            audioID);
}

void HazardEntity::attackCollision(const Attack*)
{
}

const Attack *HazardEntity::getAttack() const
{
    attack_->setKBDirection(dir_);
    return attack_;
}

void HazardEntity::hitByAttack(const Attack*) 
{
}

void HazardEntity::attackConnected(GameEntity *victim)
{
    attack_->hit(victim);
}

void HazardEntity::collisionWithGround(const Rectangle &ground, bool collision)
{
    if (!collision)
        dir_ = -dir_;
}

