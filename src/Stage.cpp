#include "Stage.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ParamReader.h"
#include "util.h"
#include "StageHazards.h"

void addEntity(GameEntity *entity);

std::string meshIDToFilename(const std::string &id)
{
    return "models/" + id + ".obj";
}

// =========
// - STAGE -
// ---------
Stage::Stage(const std::string &pp) :
    paramPrefix_(pp)
{
    glm::vec2 groundpos(getParam(pp + "ground.x"), getParam(pp + "ground.y"));
    glm::vec2 groundsize(getParam(pp + "ground.w"), getParam(pp + "ground.h"));
    groundDepth_ = getParam(pp + "ground.d");

    ground_ = rectangle(groundpos.x, groundpos.y, groundsize.x, groundsize.y);
    groundDepth_ = getParam(pp + "ground.d");

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

    // read platforms
    int nplatforms = getParam(pp + "platforms");
    for (int i = 0; i < nplatforms; i++)
    {
        std::stringstream ss;
        ss << pp << "platform" << i+1 << '.';
        const std::string prefix = ss.str();
        rectangle platform(getParam(prefix + "x"), getParam(prefix + "y"),
                getParam(prefix + "w"), getParam(prefix + "h"));
        platforms_.push_back(platform);
    }

    // set up renderer
    std::string levelFile = meshIDToFilename(strParam(pp + "levelModel"));
    mesh* levelMesh    = createMesh(levelFile, true);

    mesh* platformMesh = NULL;
    if (!platforms_.empty())
    {
        std::string platformFile = meshIDToFilename(strParam(pp + "platformModel"));
        platformMesh = createMesh(platformFile, true);
    }
    std::string programID = strParam(pp + "background");
    GLuint bgProgram   = make_program(
            ("shaders/"+programID+".v.glsl").c_str(),
            ("shaders/"+programID+".f.glsl").c_str());
    renderer_ = new StageRenderer(levelMesh, platformMesh, bgProgram);
}

Stage::~Stage()
{
    delete renderer_;
}

void Stage::update(float dt)
{
    // All the add on callbacks
    for (std::list<StageAddOn*>::iterator it = addOns_.begin(); it != addOns_.end(); it++)
        (*it)->update(dt);
}

void Stage::renderBackground(float dt)
{
    renderer_->renderBackground(dt);

    // All the add on callbacks
    for (std::list<StageAddOn*>::iterator it = addOns_.begin(); it != addOns_.end(); it++)
        (*it)->renderBackground(dt);
}

void Stage::renderStage(float dt)
{
    // Render level
    renderer_->renderLevel(ground_, groundDepth_, groundColor_);

    // Render platforms
    for (size_t i = 0; i < platforms_.size(); i++)
    {
        rectangle pf = platforms_[i];
        renderer_->renderPlatform(pf, groundDepth_, groundColor_);
    }
}

void Stage::addOn(StageAddOn *addon)
{
    addOns_.push_back(addon);
}


// ==================
// - STAGE RENDERER -
// ------------------
StageRenderer::StageRenderer(mesh *levelMesh, mesh* platformMesh, GLuint bgProgram) :
    levelMesh_(levelMesh), platformMesh_(platformMesh), backProgram_(bgProgram),
    t_(0.f)
{
    // TODO parameterize this as well
    levelProgram_ = make_program("shaders/stage.v.glsl", "shaders/stage.f.glsl");
    platformProgram_ = make_program("shaders/stage.v.glsl", "shaders/stage.f.glsl");
}

StageRenderer::~StageRenderer()
{
    freeMesh(levelMesh_);
    freeMesh(platformMesh_);

    // TODO clean up programs
}

void StageRenderer::renderLevel(const rectangle &ground, float depth, const glm::vec3 &color)
{
    // Initialize program
    glUseProgram(levelProgram_);
    glm::vec4 lightPos = getViewMatrixStack().current() * glm::vec4(500.f, 400.f, 200.f, 1.f);
    lightPos /= lightPos.w;
    GLuint colorUniform = glGetUniformLocation(levelProgram_, "color");
    GLuint lightPosUniform = glGetUniformLocation(levelProgram_, "lightpos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(glm::vec4(color, 0.0f)));
    glUniform4fv(lightPosUniform, 1, glm::value_ptr(lightPos));

    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground.x, ground.y, 0.1)),
            glm::vec3(ground.w/2, ground.h/2, depth/2));
    renderMesh(transform, levelMesh_, levelProgram_);

    glUseProgram(0);
}

void StageRenderer::renderPlatform(const rectangle &platform, float depth, const glm::vec3 &color)
{
    // Initialize program
    glUseProgram(platformProgram_);
    glm::vec4 lightPos = getViewMatrixStack().current() * glm::vec4(500.f, 400.f, 200.f, 1.f);
    lightPos /= lightPos.w;
    GLuint colorUniform = glGetUniformLocation(platformProgram_, "color");
    GLuint lightPosUniform = glGetUniformLocation(platformProgram_, "lightpos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(glm::vec4(color, 0.0f)));
    glUniform4fv(lightPosUniform, 1, glm::value_ptr(lightPos));

    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(platform.x, platform.y, 0.1)),
            // XXX there is a divided by 3 here
            glm::vec3(platform.w/2, platform.h/2, depth/3));
    renderMesh(transform, platformMesh_, levelProgram_);

    glUseProgram(0);
}

void StageRenderer::renderBackground(float dt)
{
    t_ += dt;
	if (getParam("background.shouldRender") == 0) 
		return;

    glEnable(GL_CULL_FACE);
    float r = getParam("backgroundSphere.radius");
    glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(r, r, r));

    GLuint timeUniform = glGetUniformLocation(backProgram_, "t");
    GLuint colorUniform = glGetUniformLocation(backProgram_, "color");

    glUseProgram(backProgram_);
    glUniform3fv(colorUniform, 1, glm::value_ptr(glm::vec3(0.f)));
    glUniform1f(timeUniform, t_);
    glUseProgram(0);

    renderPlane(transform, backProgram_);
    glDisable(GL_CULL_FACE);
}


// =======================
// - WORMHOLE SHIP ADDON -
// -----------------------
WormholeShipAddOn::WormholeShipAddOn(const Stage* stage) :
    StageAddOn(stage),
    shipMesh_(NULL),
    shipProgram_(0),
    t_(0.f)
{
    shipMesh_ = createMesh(meshIDToFilename("ship1-2"), false);
    shipProgram_ = make_program("shaders/stage.v.glsl", "shaders/stage.f.glsl");
}

WormholeShipAddOn::~WormholeShipAddOn()
{
    freeMesh(shipMesh_);
}

void WormholeShipAddOn::renderBackground(float dt)
{
    t_ += dt;
    if (!getParam("background.shouldRender"))
            return;

    float v = -1;
    float off = 0.2 * sin(5*v - t_) * 7*(-v);
    float xfact = (sin(M_PI*t_/10 + v) + 1) / 2;

    float xoff = off * xfact;
    float yoff = off * (1 - xfact);

    float r = getParam("backgroundSphere.radius");
    float scale = getParam("wormholeShip.scale");
    const glm::vec3 color(0.2f);
    glm::mat4 backtrans = glm::rotate(
            glm::scale(
                glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(r*0.8,r*0.8,r)),
                    glm::vec3(xoff, yoff, 2*v)),
                glm::vec3(scale)),
            -90.f, glm::vec3(0,1,0));

    // Initialize program
    glUseProgram(shipProgram_);
    glm::vec4 lightPos = getViewMatrixStack().current() * glm::vec4(500.f, 400.f, 200.f, 1.f);
    lightPos /= lightPos.w;
    GLuint colorUniform = glGetUniformLocation(shipProgram_, "color");
    GLuint lightPosUniform = glGetUniformLocation(shipProgram_, "lightpos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(glm::vec4(color, 0.0f)));
    glUniform4fv(lightPosUniform, 1, glm::value_ptr(lightPos));

    renderMesh(backtrans, shipMesh_, shipProgram_);

    glUseProgram(0);
}

// ========================
// - VOLCANO HAZARD ADDON -
// ------------------------
VolcanoHazardAddOn::VolcanoHazardAddOn(const Stage *stage) :
    StageAddOn(stage)
{
    t_ = nextTime();
}

void VolcanoHazardAddOn::update(float dt)
{
    t_ -= dt;
    if (t_ < 0.f)
    {
        const rectangle ground = stage_->getGroundRect();
        // spawn hazard at random location on ground
        glm::vec2 hpos(random_float(ground.x - ground.w/2, ground.x + ground.w/2),
                ground.y + ground.h/2);
        // Make sure it's not on the edge
        hpos.x *= 0.8f;

        addEntity(new VolcanoHazard(hpos));

        // Reset timer
        t_ = nextTime();
        //logger_->info() << "Spawning hazard.  Next in " << hazardT_ << "s\n";
    }
}

float VolcanoHazardAddOn::nextTime()
{
    return random_float(getParam("volcanoHazard.mintime"),
            getParam("volcanoHazard.maxtime"));
}

