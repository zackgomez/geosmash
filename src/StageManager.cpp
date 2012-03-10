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

void addEntity(GameEntity *);

StageManager::StageManager() :
    t_(0)
{
    logger_ = Logger::getLogger("StageManager");
    initBackground();

    // TODO move to initStage
    // Load the ground mesh
    level_mesh_ = createMesh("models/level.obj", true);
    platform_mesh_ = createMesh("models/cube.obj");
    ship_mesh_ = createMesh("models/ship1-2.obj");
    ship_main_mesh_ = createMesh("models/ship_main.obj", true);

    stageProgram_ = make_program("shaders/stage.v.glsl", "shaders/stage.f.glsl");
    wormholeProgram_ = make_program("shaders/wormholebg.v.glsl", "shaders/wormholebg.f.glsl");
    
    // Add a particle group for gravity
    logger_->debug() << "Adding gravity particle group\n"; 
}

// XXX this already exists in kiss-particles utils, make a global utils
float random_float(float min, float max)
{
    return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

void StageManager::update(float dt)
{
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
    return killbox_;
}

void StageManager::initLevel(const std::string &stage)
{
    glm::vec2 groundpos(getParam("level.x"), getParam("level.y"));
    glm::vec2 groundsize(getParam("level.w"), getParam("level.h"));

    ground_ = rectangle(groundpos.x, groundpos.y, groundsize.x, groundsize.y);
    ground_color_ = glm::vec3(getParam("level.r"),
            getParam("level.g"), getParam("level.b"));

    levelHazard_ = false;
    hazardT_ = HUGE_VAL;
    killbox_ = rectangle(getParam("killbox.x"), getParam("killbox.y"), 
            getParam("killbox.w"), getParam("killbox.h"));
    // XXX: hardcoded
    if (stage == "bandwidth bar")
    {
        // Set up hazard params
        levelHazard_ = true;
        hazardT_ = random_float(getParam("volcanoHazard.mintime"),
                getParam("volcanoHazard.maxtime"));
        backProgram_ = sphereProgram_;
    }
    else if (stage == "realtime ranch")
    {
        platforms_.push_back(rectangle(-200, 40, 220, 10));
        platforms_.push_back(rectangle(0, 140, 220, 10));
        platforms_.push_back(rectangle(200, 40, 220, 10));

        ground_.w *= 0.80f;
        backProgram_ = wormholeProgram_;
    }
    else if (stage == "ether net")
    {
        platforms_.push_back(rectangle(-200, 40, 220, 10));
        // This guy is the fatty platform in the middle
        platforms_.push_back(rectangle(10, 190, 440, 10));
        platforms_.push_back(rectangle(200, 340, 220, 10));

        ground_.w *= 0.80f;
        backProgram_ = sphereProgram_;
        killbox_ = rectangle(getParam("ether_net.killbox.x"), getParam("ether_net.killbox.y"), 
            getParam("ether_net.killbox.w"), getParam("ether_net.killbox.h"));

    }
    else
    {
        assert(false && "Unknown level");
    }

    // Set up ledges based on location of ground
    // Left side ledge
    Ledge l;
    l.pos = glm::vec2(ground_.x - ground_.w / 2, ground_.y + ground_.h/2);
    l.occupied = false;
    l.dir = -1;
    ledges_.push_back(new Ledge(l));

    // Right side ledge
    l.pos = glm::vec2(ground_.x + ground_.w / 2, ground_.y + ground_.h/2);
    l.occupied = false;
    l.dir = 1;
    ledges_.push_back(new Ledge(l));
}

void StageManager::clear()
{
    ground_ = rectangle();
    ledges_.clear();
    platforms_.clear();
}

void StageManager::initBackground()
{
    // Create a mesh [-5, 5] with some number of vertices
    meshRes_ = getParam("backgroundSphere.meshRes");
    float *mesh = new float[2 * meshRes_ * meshRes_];
    for (int y = 0; y < meshRes_; y++)
    {
        for (int x = 0; x < meshRes_; x++)
        {
            int ind = 2*(y*meshRes_ + x);
            mesh[ind]     = (x - meshRes_/2.f) / (meshRes_-1) * 10.f;
            mesh[ind + 1] = (y - meshRes_/2.f) / (meshRes_-1) * 10.f;
        }
    }
    meshBuf_ = make_buffer(GL_ARRAY_BUFFER, mesh, sizeof(float) * meshRes_ * meshRes_ * 2);
    delete[] mesh;

    // Create the element indices for the mesh
    indicies_ = new GLuint*[meshRes_ - 1];
    for (int i = 0; i < meshRes_ - 1; i++)
    {
        indicies_[i] = new GLuint[meshRes_*2];
        GLuint *array = indicies_[i];
        for (int j = 0; j < meshRes_; j++)
        {
            array[2*j + 0] = i * meshRes_ + j;
            array[2*j + 1] = (i + 1) * meshRes_ + j;
        }
    }

    // make the shaders
    sphereProgram_ = make_program("shaders/sphere.v.glsl", "shaders/sphere.f.glsl");
    assert(sphereProgram_);
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

std::vector<rectangle> StageManager::getPlatforms() const
{
    return platforms_;
}

void StageManager::renderBackground(float dt)
{
    t_ += 1*dt;
	if (getParam("backgroundSphere.shouldRender") == 0) 
	{
		return;
	}

    glEnable(GL_CULL_FACE);
    float r = getParam("backgroundSphere.radius");
    glm::mat4 transform = glm::rotate(glm::scale(glm::mat4(1.f), glm::vec3(r, r, r)),
            //5*sinf(t_), glm::normalize(glm::vec3(0, cosf(t_/25), sinf(t_/25))));
            0.f, glm::normalize(glm::vec3(0, 1, 0)));
            //t_, glm::normalize(glm::vec3(0, 1, 0)));
    
    GLuint projectionUniform = glGetUniformLocation(backProgram_, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(backProgram_, "modelViewMatrix");
    GLuint timeUniform = glGetUniformLocation(backProgram_, "t");
    GLuint colorUniform = glGetUniformLocation(backProgram_, "color");
    GLuint positionAttrib = glGetAttribLocation(backProgram_, "position");

    glUseProgram(backProgram_);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(getProjectionMatrixStack().current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(getViewMatrixStack().current() * transform));
    glUniform3fv(colorUniform, 1, glm::value_ptr(glm::vec3(0.f)));
    glUniform1f(timeUniform, t_);

    glBindBuffer(GL_ARRAY_BUFFER, meshBuf_);
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, (void*)0);

    for (int i = 0; i < meshRes_ - 1; i++)
    {
        glDrawElements(GL_TRIANGLE_STRIP, meshRes_*2, GL_UNSIGNED_INT, indicies_[i]);
    }

    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
    glDisable(GL_CULL_FACE);


    // XXX render rectangle at end of tunnel
    
    float v = -1;
    float off = 0.2 * sin(5*v - t_) * 7*(-v);
    float xfact = (sin(M_PI*t_/10 + v) + 1) / 2;

    float xoff = off * xfact;
    float yoff = off * (1 - xfact);

    glm::mat4 backtrans = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(r*0.8,r*0.8,r)),
            glm::vec3(xoff, yoff, 2*v));
    renderMesh(glm::rotate(glm::scale(backtrans, glm::vec3(.15)), -90.f, glm::vec3(0,1,0)), ship_mesh_, stageProgram_);
    //renderRectangle(backtrans, glm::vec4(1,1,1,0));
}

void StageManager::renderStage(float dt)
{
    // Set initial shader values
    glUseProgram(stageProgram_);
    glm::vec4 lightPos = getViewMatrixStack().current() * glm::vec4(500.f, 400.f, 200.f, 1.f);
    lightPos /= lightPos.w;
    GLuint colorUniform = glGetUniformLocation(stageProgram_, "color");
    GLuint lightPosUniform = glGetUniformLocation(stageProgram_, "lightpos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(glm::vec4(ground_color_, 0.0f)));
    glUniform4fv(lightPosUniform, 1, glm::value_ptr(lightPos));

    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground_.x, ground_.y, 0.1)),
            glm::vec3(ground_.w/2, ground_.h/2, getParam("level.d")/2));
    renderMesh(transform, level_mesh_, stageProgram_);

    // Draw the platforms
    for (size_t i = 0; i < platforms_.size(); i++)
    {
        rectangle pf = platforms_[i];
        transform = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(pf.x, pf.y, 0.0)),
                glm::vec3(pf.w, pf.h, getParam("level.d")/3));
        renderMesh(transform, platform_mesh_, stageProgram_);
    }

    glUseProgram(0);
}

rectangle StageManager::getGroundRect() const
{
    return ground_;
}

//////////////////////////////////////////////
// Volcano Hazard  
//////////////////////////////////////////////

VolcanoHazard::VolcanoHazard(const glm::vec2 &pos) :
    GameEntity(),
    t_(0.f)
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
            /*
            glm::vec4(0.8, 0.1, 0.1, 0.8),
            glm::vec4(0.8, 0.1, 0.1, 0.8),
            glm::vec4(0.8, 0.1, 0.1, 0.8),
            glm::vec4(0.8, 0.1, 0.1, 0.8),
            glm::vec4(0.8, 0.2, 0.1, 0.8),
            glm::vec4(0.7, 0.4, 0.1, 0.8),
            glm::vec4(0.7, 0.4, 0.1, 0.8),
            glm::vec4(0.8, 0.2, 0.1, 0.8),
            glm::vec4(0.8, 0.3, 0.1, 0.8),
            glm::vec4(0.8, 0.3, 0.1, 0.8),
            glm::vec4(0.3, 0.3, 0.3, 0.0),
            glm::vec4(0.3, 0.3, 0.3, 0.0),
            glm::vec4(0.1, 0.1, 0.1, 0.0),
            glm::vec4(0.1, 0.1, 0.1, 0.0),
            */
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
        emitter_->setOutputRate(200 + 200*fact);
        // TODO Set parameters to make the steam grow
    }
    else if (t_ > getParam(pre_ + "startup")
            && t_ < getParam(pre_ + "startup") + getParam(pre_ + "duration"))
    {
    }
    else
    {
        PGroup * gravityGroup = ParticleManager::get()->newGroup("gravity");
        gravityGroup->setAction(new ConstForceF(8, glm::vec3(0, -1, 0)));
        // Smokey
        // XXX fix the gravity group!
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


