#pragma once
#include <vector>
#include "GameEntity.h" // for rectangle
#include "Engine.h"

struct Ledge
{
    glm::vec2 pos;
    bool occupied;
    // Either -1 left, or +1 right the direction that the ledge can be grabbed
    float dir;
};

class StageRenderer
{
public:
    StageRenderer();
    ~StageRenderer();

    void renderLevel(const rectangle &level_, float depth, const glm::vec3 &color);
    void renderPlatform(const rectangle &platform_, float depth, const glm::vec3 &color);
    void renderBackground(float dt);

private:
    mesh *levelMesh_;
    mesh *platformMesh_;

    GLuint levelProgram_, platformProgram_;
    GLuint sphereProgram_;

    float t_;
};

class Stage
{
public:
    Stage(const std::string &paramPrefix);
    virtual ~Stage();

    virtual void update(float dt);
    virtual void renderBackground(float dt);
    virtual void renderStage(float dt);

    const rectangle &getGroundRect() const { return ground_; }
    const std::vector<Ledge*>& getLedges() const { return ledges_; }
    const std::vector<rectangle>& getPlatforms() const { return platforms_; }
    const rectangle &getKillBox() const { return killbox_; }
    
protected:
    const std::string paramPrefix_;

    std::vector<Ledge*> ledges_;
    rectangle ground_;
    std::vector<rectangle> platforms_;
    rectangle killbox_;

    // Graphics members
    glm::vec3 groundColor_;
    float groundDepth_;

    StageRenderer *renderer_;
};

