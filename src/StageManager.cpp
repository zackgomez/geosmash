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

void addEntity(GameEntity *);

StageManager::StageManager() :
    t_(0)
{
    logger_ = Logger::getLogger("StageManager");
    initBackground();

    // TODO move to initStage
    // Load the ground mesh
    level_mesh_ = createMesh("models/level.obj");
    platform_mesh_ = createMesh("models/cube.obj");

    GLuint vert = make_shader(GL_VERTEX_SHADER, "shaders/stage.v.glsl");
    GLuint frag = make_shader(GL_FRAGMENT_SHADER, "shaders/stage.f.glsl");
    if (!vert || !frag)
    {
        std::cerr << "Unable to read background sphere shaders\n";
        exit(1);
    }
    stageProgram_ = make_program(vert, frag);
    assert(stageProgram_);
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
    ret.push_back("default");
    ret.push_back("platforms");

    return ret;
}

void StageManager::initLevel(const std::string &stage)
{
    glm::vec2 groundpos(getParam("level.x"), getParam("level.y"));
    glm::vec2 groundsize(getParam("level.w"), getParam("level.h"));

    ground_ = rectangle(groundpos.x, groundpos.y, groundsize.x, groundsize.y);
    ground_color_ = glm::vec3(getParam("level.r"),
            getParam("level.g"), getParam("level.b"));

    // XXX: hardcoded
    if (stage == "default")
    {
        // Set up hazard params
        levelHazard_ = true;
        hazardT_ = random_float(getParam("volcanoHazard.mintime"),
                getParam("volcanoHazard.maxtime"));
    }
    if (stage == "platforms")
    {
        platforms_.push_back(rectangle(-200, 40, 220, 10));
        platforms_.push_back(rectangle(0, 140, 220, 10));
        platforms_.push_back(rectangle(200, 40, 220, 10));

        ground_.w *= 0.80f;
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

std::vector<rectangle> StageManager::getPlatforms() const
{
    return platforms_;
}

void StageManager::renderSphereBackground(float dt)
{
    t_ += 3*dt;
	if (getParam("backgroundSphere.shouldRender") == 0) 
	{
		return;
	}
    glEnable(GL_CULL_FACE);
    float r = getParam("backgroundSphere.radius");
    glm::mat4 transform = glm::rotate(glm::scale(glm::mat4(1.f), glm::vec3(r, r, r)),
            //5*sinf(t_), glm::normalize(glm::vec3(0, cosf(t_/25), sinf(t_/25))));
            t_, glm::normalize(glm::vec3(0, 1, 0)));
    
    GLuint projectionUniform = glGetUniformLocation(sphereProgram_, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(sphereProgram_, "modelViewMatrix");
    GLuint positionAttrib = glGetAttribLocation(sphereProgram_, "position");

    glUseProgram(sphereProgram_);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(getProjectionMatrixStack().current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(getViewMatrixStack().current() * transform));

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
    GameEntity()
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
            -1, -1, // player, team IDs
            ""); // audio ID

}

VolcanoHazard::~VolcanoHazard()
{
    delete attack_;
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

    GameEntity::update(dt);
}

void VolcanoHazard::render(float dt)
{
    // Depending on state render different things
    // Startup, just have it steam
    if (t_ < getParam(pre_ + "startup"))
    {
        glm::mat4 transform = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
                glm::vec3(getParam(pre_ + "steamw"), getParam(pre_ + "steamh"), 1.f));

        renderRectangle(transform, glm::vec4(0.8f, 0.8f, 0.8f, 0.0f));
    }
    else if (t_ > getParam(pre_ + "startup")
            && t_ < getParam(pre_ + "startup") + getParam(pre_ + "duration"))
    {
        // Fire and brimstone
        glm::mat4 transform = glm::scale(
                glm::translate(glm::mat4(1.f),
                    glm::vec3(attack_->getHitbox().x, attack_->getHitbox().y, 0.f)),
                glm::vec3(attack_->getHitbox().w, attack_->getHitbox().h, 1.f));

        renderRectangle(transform, glm::vec4(0.8f, 0.1f, 0.1f, 0.0f));
    }
    else
    {
        // Cooldown, a smokey texture
        glm::mat4 transform = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(pos_, 0.f)),
                glm::vec3(getParam(pre_ + "steamw"), getParam(pre_ + "steamh"), 1.f));

        renderRectangle(transform, glm::vec4(0.2f, 0.2f, 0.2f, 0.0f));
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


