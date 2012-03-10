#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "GameEntity.h"
#include "Attack.h"
#include "Fighter.h"
#include "Engine.h"
#include "Logger.h"

class Stage;
struct Ledge;

class StageManager
{
public:
    // Return the singleton instance
    static StageManager* get();

    std::vector<std::string> getStageNames() const;
    void initLevel(const std::string &stageName);

    // Called every frame.
    // For now, just put out a stage hazard (maybe)
    void update(float dt);

    void renderBackground(float dt);

    // Renders the actual stage platform
    void renderStage(float dt);

    // Cleans up and restores this stage manager to default state
    void clear();

    // Returns the nearest non occupied ledge, or null
    Ledge * getPossibleLedge(const glm::vec2 &pos);

    rectangle getKillBox() const;
    rectangle getGroundRect() const;

    std::vector<rectangle> getPlatforms() const;

private:
    StageManager();

    LoggerPtr logger_;

    Stage *stage_;



    int meshRes_;
    GLuint meshBuf_;
    GLuint **indicies_;
    GLuint sphereProgram_, wormholeProgram_;

    GLuint backProgram_, stageProgram_;

    // Hazard related members
    bool levelHazard_;
    float hazardT_;

    void initBackground();

    glm::vec3 ground_color_;
    mesh *level_mesh_;
    mesh *platform_mesh_;
    mesh *ship_mesh_;
    mesh *ship_main_mesh_;
};

