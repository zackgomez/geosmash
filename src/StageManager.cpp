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

    // TODO move to initStage
    /*
    initBackground();

    // Load the ground mesh
    level_mesh_ = createMesh("models/level.obj", true);
    platform_mesh_ = createMesh("models/cube.obj");
    ship_mesh_ = createMesh("models/ship1-2.obj");
    ship_main_mesh_ = createMesh("models/ship_main.obj", true);

    wormholeProgram_ = make_program("shaders/wormholebg.v.glsl", "shaders/wormholebg.f.glsl");
    */
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
    std::string pp(stage);
    for (size_t i = 0; i < pp.size(); i++)
        if (pp[i] == ' ')
            pp[i] = '_';
    pp += '.';

    assert(!stage_);
    stage_ = new Stage(pp);
}

void StageManager::clear()
{
    if (stage_)
    {
        stage_->clear();
        delete stage_;
        stage_ = NULL;
    }
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
    /*
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


    // XXX this renders ship at the end of the tunnel, move to level specific
    // area
    float v = -1;
    float off = 0.2 * sin(5*v - t_) * 7*(-v);
    float xfact = (sin(M_PI*t_/10 + v) + 1) / 2;

    float xoff = off * xfact;
    float yoff = off * (1 - xfact);

    glm::mat4 backtrans = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(r*0.8,r*0.8,r)),
            glm::vec3(xoff, yoff, 2*v));
    renderMesh(glm::rotate(glm::scale(backtrans, glm::vec3(.15)), -90.f, glm::vec3(0,1,0)), ship_mesh_, stageProgram_);
    */
}

void StageManager::renderStage(float dt)
{
    stage_->renderStage(dt);
    /*
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
    */
}

rectangle StageManager::getGroundRect() const
{
    return stage_->getGroundRect();
}

