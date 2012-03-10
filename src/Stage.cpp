#include "Stage.h"
#include "ParamReader.h"
#include <glm/gtc/matrix_transform.hpp>

Stage::Stage(const std::string &pp) :
    paramPrefix_(pp)
{
    glm::vec2 groundpos(getParam(pp + "ground.x"), getParam(pp + "ground.y"));
    glm::vec2 groundsize(getParam(pp + "ground.w"), getParam(pp + "ground.h"));
    groundDepth_ = getParam(pp + "ground.d");

    ground_ = rectangle(groundpos.x, groundpos.y, groundsize.x, groundsize.y);
    groundColor_ = glm::vec3(getParam(pp + "ground.r"),
            getParam(pp + "ground.g"), getParam(pp + "ground.b")); 

    killbox_ = rectangle(getParam(pp + "killbox.x"), getParam(pp + "killbox.y"), 
            getParam(pp + "killbox.w"), getParam(pp + "killbox.h"));

    // Set ledges based on ground
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

    // TODO init some gfx, like meshes and programs

    // TODO read platforms
}

void Stage::update(float dt)
{
    // nop
}

void Stage::renderBackground(float dt)
{
    // TODO
}

void Stage::renderStage(float dt)
{
    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground_.x, ground_.y, 0.1)),
            glm::vec3(ground_.w/2, ground_.h/2, getParam("level.d")/2));
    renderMesh(transform, levelMesh_, levelProgram_);

    // Draw the platforms
    for (size_t i = 0; i < platforms_.size(); i++)
    {
        rectangle pf = platforms_[i];
        transform = glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(pf.x, pf.y, 0.0)),
                glm::vec3(pf.w, pf.h, getParam("level.d")/3));
        renderMesh(transform, platformMesh_, platformProgram_);
    }
}

// Reset stage variables (not ground, ledges, etc)
void Stage::clear()
{
    // nop
}

void Stage::initGraphics()
{
    // TODO
}
