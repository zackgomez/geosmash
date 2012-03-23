#pragma once
#include "GameState.h"
#include <glm/glm.hpp>
#include "Logger.h"

class rectangle;

class LevelSelectState : public GameState
{
public:
    LevelSelectState(int controllerIdx);
    virtual ~LevelSelectState();

    virtual GameState* processInput(const std::vector<Controller*> &controllers,
            float dt);
    virtual void render(float dt);

    // unused
    virtual void preFrame() { }
    virtual void update(float dt) { }
    virtual void postFrame() { }

private:
    LoggerPtr logger_;
    int controllerIdx_;
    glm::vec2 cursorPos_;
    std::vector<std::string> stages_;

    // Index of the currently selected stage, or -1 if no selected
    // stage
    int curStage_;

    rectangle stageRect(int idx);
    GameState* newGame(const std::vector<Controller*>&);
};

