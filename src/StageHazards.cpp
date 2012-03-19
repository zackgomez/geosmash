#include "StageHazards.h"
#include "ParamReader.h"
#include "PManager.h"
#include "Attack.h"
#include "AudioManager.h"

//////////////////////////////////////////////
// Volcano Hazard  
//////////////////////////////////////////////
VolcanoHazard::VolcanoHazard(const glm::vec2 &pos) :
    GameEntity(),
    t_(0.f),
	active_(false)
{
    pos_ = pos;
    size_  = glm::vec2(0.f);
    vel_   = glm::vec2(0.f);
    accel_ = glm::vec2(0.f);

    pre_ = "volcanoHazard.";

    glm::vec2 kbdir = glm::normalize(glm::vec2(
                getParam(pre_ + "knockbackx"),
                getParam(pre_ + "knockbacky")));
    kbdir *= glm::vec2(glm::sign(vel_.x), 1.f);

    glm::vec2 asize = glm::vec2(
            getParam(pre_ + "hitboxx"),
            getParam(pre_ + "hitboxy"));
    glm::vec2 apos = glm::vec2(pos_.x, pos_.y + asize.y/2);

    attack_ = new SimpleAttack(
            kbdir,
            getParam(pre_ + "kbbase"),
            getParam(pre_ + "kbscaling"),
            getParam(pre_ + "damage"),
            getParam(pre_ + "stun"),
            getParam(pre_ + "priority"),
            apos, asize,
            1, // odir
            -2, -2, // player, team IDs
            "hazardhit"); // audio ID

    emitter_ = ParticleManager::get()->newEmitter();
    emitter_->setParticleLifetimeF(new lifetimeNormalF(1.8f, 0.5f))
            ->setParticleVelocityF(new coneVelocityF(60.f, 3.f, glm::vec3(0,1,0), 0.9f))
            ->setParticleLocationF(new locationF(1.0f))
            ->setLocation(glm::vec3(pos, 0.f))
            ->setOutputRate(200);
    ParticleManager::get()->addEmitter(emitter_);

    AudioManager::get()->playSound("hazardwarn");

}

VolcanoHazard::~VolcanoHazard()
{
    delete attack_;
    ParticleManager::get()->quashEmitter(emitter_);
}

bool VolcanoHazard::isDone() const
{
    // Done when gone through all stages
    return t_ > getParam(pre_ + "startup") +
        getParam(pre_ + "duration") + getParam(pre_ + "cooldown");
}

bool VolcanoHazard::hasAttack() const
{
    return t_ > getParam(pre_ + "startup")
        && t_ < getParam(pre_ + "startup") + getParam(pre_ + "duration");
}

const Attack* VolcanoHazard::getAttack() const
{
    // Quick sanity check
    assert(hasAttack());
    return attack_;
}

void VolcanoHazard::update(float dt)
{
    t_ += dt;

    if (t_ > getParam(pre_ + "startup") && !active_)
    {
        active_ = true;
        AudioManager::get()->playSound("hazardactive");
        glm::vec4 pcolors_raw[] =
        {
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.2, 0.8, 0.1, 0.8),
            glm::vec4(0.5, 0.8, 0.1, 0.8),
            glm::vec4(0.5, 0.8, 0.1, 0.8),
            glm::vec4(0.2, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.1, 0.8),
            glm::vec4(0.1, 0.8, 0.3, 0.0),
            glm::vec4(0.1, 0.8, 0.3, 0.0),
            glm::vec4(0.1, 0.8, 0.1, 0.0),
            glm::vec4(0.1, 0.8, 0.1, 0.0),
        };
        std::vector<glm::vec4> pcolors(pcolors_raw,
                pcolors_raw + sizeof(pcolors_raw)/sizeof(glm::vec4));
        // Set color and shit to be Fire and brimstone
        emitter_->setParticleLocationF(new circleInteriorLocationF(attack_->getHitbox().w, glm::vec3(0,1,0)))
                ->setParticleVelocityF(new coneVelocityF(800.f, 200.f, glm::vec3(0,1,0), 1.0f))
                ->setOutputRate(6000)
                ->setParticleColorF(new discreteColorF(pcolors))
                ->setParticleSize(glm::vec3(3,3,3));
    }

    GameEntity::update(dt);
}

void VolcanoHazard::render(float dt)
{
    // Depending on state render different things
    // Startup, just have it steam
    if (t_ < getParam(pre_ + "startup"))
    {
        float fact = t_ / getParam(pre_ + "startup");
        // TODO Set parameters to make the steam grow
    }
    else if (t_ > getParam(pre_ + "startup")
            && t_ < getParam(pre_ + "startup") + getParam(pre_ + "duration"))
    {
        // Taken care of by update
    }
    else
    {
        // Smokey
        // XXX add gravity
        emitter_->setParticleLocationF(new circleInteriorLocationF(attack_->getHitbox().w/5, glm::vec3(0,1,0)))
                ->setParticleVelocityF(new coneVelocityF(50.f, 3.f, glm::vec3(0,1,0), 0.95f))
                ->setParticleLifetimeF(new lifetimeNormalF(0.5f, 0.2f))
                ->setOutputRate(500)
                ->setParticleSize(glm::vec3(1.f))
                ->setParticleColorF(new colorF(glm::vec4(0.4f), 0.7, 0.1));
    }
}

void VolcanoHazard::attackCollision(const Attack*)
{
    // Ignore
}

void VolcanoHazard::attackConnected(GameEntity *victim)
{
    victim->hitByAttack(attack_);
    attack_->hit(victim);
}

void VolcanoHazard::collisionWithGround(const rectangle &, bool, bool)
{
    // Ignore 
}

void VolcanoHazard::hitByAttack(const Attack*)
{
    // Should never happen
    assert(false);
}

