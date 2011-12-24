#define _USE_MATH_DEFINES
#include "StageManager.h"
#include "Projectile.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Fighter.h"
#include "FrameManager.h"
#include "PManager.h"

StageManager::StageManager() :
    t_(0)
{
    glm::vec2 groundpos(getParam("level.x"), getParam("level.y"));
    glm::vec2 groundsize(getParam("level.w"), getParam("level.h"));
    Ledge l;
    // Left side ledge
    l.pos = glm::vec2(groundpos.x - groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    l.dir = -1;
    ledges_.push_back(new Ledge(l));

    // Right side ledge
    l.pos = glm::vec2(groundpos.x + groundsize.x / 2, groundpos.y + groundsize.y/2);
    l.occupied = false;
    l.dir = 1;
    ledges_.push_back(new Ledge(l));

    initBackground();

    ground_ = rectangle(
            getParam("level.x"), getParam("level.y"),
            getParam("level.w"), getParam("level.h"));
    ground_color_ = glm::vec3(getParam("level.r"),
            getParam("level.g"), getParam("level.b"));

    // Load the ground mesh
    level_mesh_ = createMesh("models/level.obj");
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
    GLuint vert = make_shader(GL_VERTEX_SHADER, "shaders/sphere.v.glsl");
    GLuint frag = make_shader(GL_FRAGMENT_SHADER, "shaders/sphere.f.glsl");
    if (!vert || !frag)
    {
        std::cerr << "Unable to read background sphere shaders\n";
        exit(1);
    }
    sphereProgram_ = make_program(vert, frag);
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

void StageManager::renderSphereBackground(float dt)
{
    t_ += 3*dt;
    glEnable(GL_CULL_FACE);
    float r = getParam("backgroundSphere.radius");
    glm::mat4 transform = glm::rotate(glm::scale(glm::mat4(1.f), glm::vec3(r, r, r)),
            //5*sinf(t_), glm::normalize(glm::vec3(0, cosf(t_/25), sinf(t_/25))));
            t_, glm::normalize(glm::vec3(0, 1, 0)));
    
    GLuint projectionUniform = glGetUniformLocation(sphereProgram_, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(sphereProgram_, "modelViewMatrix");

    glUseProgram(sphereProgram_);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(getProjectionMatrix()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(getViewMatrix() * transform));

    glBindBuffer(GL_ARRAY_BUFFER, meshBuf_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, (void*)0);

    for (int i = 0; i < meshRes_ - 1; i++)
    {
        glDrawElements(GL_TRIANGLE_STRIP, meshRes_*2, GL_UNSIGNED_INT, indicies_[i]);
    }

    glDisableVertexAttribArray(0);
    glUseProgram(0);
    glDisable(GL_CULL_FACE);
}

void StageManager::renderStage(float dt)
{
    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground_.x, ground_.y, 0.1)),
            glm::vec3(ground_.w/2, ground_.h/2, getParam("level.d")/2));
    renderMesh(level_mesh_, transform, ground_color_);
}

rectangle StageManager::getGroundRect() const
{
    return ground_;
}

//////////////////////////////////////////////
// Hazard  
//////////////////////////////////////////////

HazardEntity::HazardEntity(const std::string &audioID) :
    victim_(NULL)
{
    pre_ = "stageHazardAttack.";

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
            pos_, size_, -dir_, playerID_,
            audioID);

    // Set up particle effects
    glm::vec3 up(0, 1, 0);
    float radius = 0.8f * glm::length(size_/2.f);
    emitter = ParticleManager::get()->newEmitter();
    ParticleManager::get()->addEmitter(emitter);
    emitter_force = new CentripetalForceF(glm::vec3(pos_, 0.f), up, radius);

    PGroup *group = ParticleManager::get()->newGroup("hazardentity");
    group->addAction(emitter_force);
    group->addAction(new ConstForceF(50.f, up));

    emitter->setOutputGroup("hazardentity")
           ->setParticleLocationF(new circleLocationF(radius, up))
           ->setParticleVelocityF(new circleTangentVelocityF(100, 0, up))
           ->setParticleLifetimeF(new lifetimeNormalF(1.5, 0.3))
           ->setLocation(glm::vec3(pos_ - glm::vec2(0.f, size_.y/2), 0.f))
           ->setOutputRate(1000);
}

HazardEntity::~HazardEntity()
{
    ParticleManager::get()->quashEmitter(emitter);
    //ParticleManager::get()->removeGroup("hazardentity");
}

bool HazardEntity::isDone() const
{
    return false; 
}

void HazardEntity::update(float dt)
{
    // If we have a victim currently just continue to hold them
    if (victim_)
    {
        t_ += dt;
        if (t_ > getParam(pre_ + "twirlTime"))
        {
            // Hit the victim - disconnects them from us
            victim_->hit(attack_);
            // Make sure we're disconnected
            assert(!victim_);
            // all done for now
            return;
        }
        else
        {
            //float side = victim_->getEntity()->getPosition().x > pos_.x ? 1 : -1;
            //victim_->setAccel(glm::vec2(-side * getParam(pre_ + "xaccel"), 0));
            // Make them spin
            glm::mat4 spintrans = glm::rotate(glm::mat4(1.f), 1000.f*t_, glm::vec3(0, 1, 0));
            victim_->setPreTransform(spintrans);
        }
        // Don't do other updates
        return;
    }
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
    emitter->setLocation(glm::vec3(pos_ - glm::vec2(0.f, size_.y/2.f), 0.f));
    emitter_force->setCenter(glm::vec3(pos_, 0.f));
}

void HazardEntity::render(float dt)
{
    // Do nothing, the particle manager does our rendering
}

void HazardEntity::attackCollision(const Attack*)
{
    // Ignore this, our attacks don't get cancelled
}

bool HazardEntity::hasAttack() const
{
    // Don't attack people when we're twirling someone
    return !victim_;
}

const Attack *HazardEntity::getAttack() const
{
    attack_->setKBDirection(dir_);
    attack_->setOriginDirection(-dir_);
    return attack_;
}

void HazardEntity::hitByAttack(const Attack*) 
{
    // HazardEntity doesn't have life or anything, just ignore it
}

void HazardEntity::attackConnected(GameEntity *victim)
{
    attack_->hit(victim);
    // On a hit we need to do a couple of things
    // Only hit fighters
    if (victim->getType() != Fighter::type)
        return;
    Fighter *fighter = (Fighter *) victim;
    // Make them go limp and store it
    victim_ = fighter->goLimp(new GenericUnlimpCallback<HazardEntity>(this));

    // Reset the timer
    t_ = 0;

    // Set initial conditions on the fighter
    float yspeed = getParam(pre_ + "twirlHeight") / getParam(pre_ + "twirlTime");
    victim_->setPosition(pos_);
    victim_->setVelocity(glm::vec2(0.f, yspeed));
    victim_->setAccel(glm::vec2(0.f));
    /*
    // TODO don't "jerk" the fighter
    victim_->setPosition(pos_ + glm::vec2(size_.x, 0));
    victim_->setVelocity(glm::vec2(0, yspeed));
    victim_->setAccel(glm::vec2(-getParam(pre_ + "xaccel"), 0));
    */
}

void HazardEntity::collisionWithGround(const rectangle &ground, bool collision)
{
    if (!collision)
        dir_ = -dir_;
}

void HazardEntity::disconnectCallback()
{
    assert(victim_);
    // Simply forget about the victim
    victim_ = NULL;

    // Reset the (now hit reset) timer
    t_ = 0;
}

