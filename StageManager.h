#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "GameEntity.h"
#include "Attack.h"

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

    // Called every frame.
    // For now, just put out a stage hazard (maybe)
    void update(float dt);

    void renderSphereBackground(float dt);

    // Cleans up and restores this stage manager to default state
    void clear();

    // Returns the nearest non occupied ledge, or null
    Ledge * getPossibleLedge(const glm::vec2 &pos);

private:
    StageManager();

    std::vector<Ledge*> ledges_;

    int meshRes_;
    GLuint meshBuf_;
    GLuint sphereProgram_;
    GLuint **indicies_;
    float t_;
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

class LimpFighter;
class Emitter;
struct CentripetalForceF;

class HazardEntity : public GameEntity 
{
public:
    HazardEntity(const std::string &audioID);
    ~HazardEntity();

    virtual std::string getType() const { return "HazardEntity"; }

    virtual bool isDone() const;
    virtual bool hasAttack() const;
    virtual const Attack * getAttack() const;
    virtual bool canBeHit() const { return false; }

    virtual void update(float dt);

    // GameEntity overrides
    virtual void render(float dt);
    virtual void attackCollision(const Attack*);
    virtual void attackConnected(GameEntity*);
    virtual void collisionWithGround(const rectangle&, bool);
    virtual void hitByAttack(const Attack*);
    
    // When called disconnects from victim, assuming that LimpFighter interface
    // is no longer valid.
    virtual void disconnectCallback();
    
private:
    float lifetime_;
    SimpleAttack *attack_;
    std::string pre_;
    int dir_;
    float t_;

    Emitter *emitter;
    CentripetalForceF *emitter_force;

    LimpFighter *victim_;
};

