#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "GameEntity.h"
#include "Attack.h"
#include "Fighter.h"
#include "Engine.h"
#include "Logger.h"

struct Ledge
{
    glm::vec2 pos;
    bool occupied;
    // Either -1 left, or +1 right the direction that the ledge can be grabbed
    float dir;
};

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

    std::vector<Ledge*> ledges_;
    rectangle ground_;
    std::vector<rectangle> platforms_;
    rectangle killbox_;

    int meshRes_;
    GLuint meshBuf_;
    GLuint **indicies_;
    float t_;
    GLuint sphereProgram_, wormholeProgram_;

    GLuint backProgram_, stageProgram_;

    // Hazard related members
    bool levelHazard_;
    float hazardT_;

    void initBackground();

    glm::vec3 ground_color_;
    mesh level_mesh_;
    mesh platform_mesh_;
};

// Renders a shiny sphere.
class BackgroundSphere
{
public:
    // Just trace out some lines along latitude and longitude
    void render(float dt);

    BackgroundSphere();
private:
    // How large is the sphere, in game units?
    float radius_;
    // The number of latitude and longitude lines displayed.
    float lineCount_;
    // Number of line segments used  in those lines, or a measure of 
    // sphere quality
    float divisionCount_;
    float pulseCount_; 

    // pulses are identified by just a single point, but probably rendered
    // as line segments.
    std::vector<glm::vec3> pulses_;

    void renderLatitude(void);
    void renderLongitude(void);
    void updateLitSegments(void);
};

class Emitter;

class VolcanoHazard : public GameEntity
{
public:
    VolcanoHazard(const glm::vec2 &pos);
    virtual ~VolcanoHazard();

    virtual std::string getType() const { return "VolcanoHazard"; }

    virtual bool isDone() const;
    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;
    virtual bool canBeHit() const { return false; }

    virtual void update(float dt);

    // GameEntity overrides
    virtual void render(float dt);
    virtual void attackCollision(const Attack*);
    virtual void attackConnected(GameEntity*);
    virtual void collisionWithGround(const rectangle&, bool, bool);
    virtual void hitByAttack(const Attack*);

private:
    SimpleAttack *attack_;
    std::string pre_;
    float t_;
    bool active_;

    Emitter *emitter_;
};

