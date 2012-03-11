#define _USE_MATH_DEFINES
#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include "Fighter.h"
#include "FrameManager.h"
#include "PManager.h"
#include "AudioManager.h"
#include "StatsManager.h"
#include "Stage.h"

StageManager::StageManager() :
    stage_(NULL)
{
    logger_ = Logger::getLogger("StageManager");
}

void StageManager::update(float dt)
{
    stage_->update(dt);
    /*
    hazardT_ -= dt;
    if (levelHazard_ && hazardT_ < 0.f)
    {
        // spawn hazard at random location on ground
        glm::vec2 hpos(random_float(ground_.x - ground_.w/2, ground_.x + ground_.w/2),
                ground_.y + ground_.h/2);
        // Make sure it's not on the edge
        hpos.x *= 0.8f;

        addEntity(new VolcanoHazard(hpos));

        // Reset timer
        hazardT_ = random_float(getParam("volcanoHazard.mintime"),
                getParam("volcanoHazard.maxtime"));
        logger_->info() << "Spawning hazard.  Next in " << hazardT_ << "s\n";
    }
    */
}

std::vector<std::string> StageManager::getStageNames() const
{
    std::vector<std::string> ret;
    ret.push_back("bandwidth bar");
    ret.push_back("realtime ranch");
    ret.push_back("ether net");

    return ret;
}

rectangle StageManager::getKillBox() const
{
    return stage_->getKillBox();
}

void StageManager::initLevel(const std::string &stage)
{
    assert(!stage_);

    // Create the prefix, replace ' ' with '_' and append '.'
    std::string pp(stage);
    for (size_t i = 0; i < pp.size(); i++)
        if (pp[i] == ' ')
            pp[i] = '_';
    pp += '.';

    stage_ = new Stage(pp);

    // XXX remove this hardcoded hack
    if (stage == "realtime ranch")
        stage_->addOn(new WormholeShipAddOn());
}

void StageManager::clear()
{
    if (stage_)
    {
        delete stage_;
        stage_ = NULL;
    }
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
    const std::vector<Ledge*>& ledges = stage_->getLedges();
    for (unsigned i = 0; i < ledges.size(); i++)
    {
        Ledge *l = ledges[i];
        float dist = glm::length(pos - l->pos);
        if (!l->occupied && dist < mindist)
        {
            ret = l;
            mindist = dist;
        }
    }

    return ret;
}

std::vector<rectangle> StageManager::getPlatforms() const
{
    return stage_->getPlatforms();
}

void StageManager::renderBackground(float dt)
{
    stage_->renderBackground(dt);
}

void StageManager::renderStage(float dt)
{
    stage_->renderStage(dt);
}

rectangle StageManager::getGroundRect() const
{
    return stage_->getGroundRect();
}

