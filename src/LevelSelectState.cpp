#include "LevelSelectState.h"
#include <glm/gtc/matrix_transform.hpp>
#include "MenuState.h"
#include "InGameState.h"
#include "Controller.h"
#include "ParamReader.h"
#include "Player.h"
#include "StatsManager.h"
#include "StageManager.h"
#include "FontManager.h"
#include "Fighter.h"
#include "Engine.h"

LevelSelectState::LevelSelectState(int controllerIdx) :
    controllerIdx_(controllerIdx),
    cursorPos_(glm::vec2(0.5f) * glm::vec2(1920.f, 1080.f)),
    curStage_(-1)
{
    logger_ = Logger::getLogger("LevelSelectState");

    // Cache stages
    stages_ = StageManager::get()->getStageNames();
    // Select the last stage that was played
    const std::string lastStage = strParam("menu.stage");
    for (int i = 0; i < stages_.size(); i++)
        if (stages_[i] == lastStage)
            curStage_ = i;
    assert(curStage_ >= 0);

    cursorPos_ = stageRect(curStage_).center();

    // Set up matrices
    getProjectionMatrixStack().clear();
    getViewMatrixStack().clear();
    getProjectionMatrixStack().current() = glm::ortho(0.f, 1920.f, 1080.f, 0.f, -1.f, 1.f);
}

LevelSelectState::~LevelSelectState()
{
}

GameState* LevelSelectState::processInput(const std::vector<Controller*> &controllers,
        float dt)
{
    assert(controllerIdx_ >= 0 && controllerIdx_ < controllers.size());
    controller_state controller = controllers[controllerIdx_]->getState();

    // on start or A, start a new game, if a stage is selected
    if ((controller.pressstart || controller.pressa)
            && curStage_ >= 0)
    {
        logger_->debug() << "Starting new game\n";
        return newGame(controllers);
    }

    // On B, go back to menu
    if (controller.pressb)
        return new MenuState();

    // Move the cursor around
    glm::vec2 joypos(controller.joyx, -controller.joyy);
    if (glm::length(joypos) > getParam("input.cursorDeadzone"))
    {
        cursorPos_ += joypos * getParam("input.cursorSpeed");
        cursorPos_.x = glm::clamp(cursorPos_.x, 0.f, 1920.f);
        cursorPos_.y = glm::clamp(cursorPos_.y, 0.f, 1080.f);
    }

    // Update currently selected level
    for (int i = 0; i < stages_.size(); i++)
    {
        if (stageRect(i).contains(cursorPos_))
        {
            curStage_ = i;
            setParam("menu.stage", stages_[curStage_]);
            break;
        }
    }

    return NULL;
}

void LevelSelectState::render(float dt)
{
    // Draw title
    const float charsize = 100.f;
    const glm::vec3 titleColor(0.8f);
    glm::mat4 titleTrans = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(1920.f/2, 75.f, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(titleTrans, titleColor,
            "Level Select");

    // Draw cursor
    glm::mat4 cursorTrans = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(cursorPos_, 0.1f)),
            glm::vec3(10.f, 10.f, 1.f));
    glm::vec4 cursorColor(1.f, 1.f, 1.f, 0.f);
    renderRectangle(cursorTrans, cursorColor);

    // Draw levels
    // Just draw in a line in the middle of the screen for now
    for (int i = 0; i < stages_.size(); i++)
    {
        rectangle box = stageRect(i);
        
        // draw stage box
        glm::mat4 stageTrans = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(box.x, box.y, 0.f)),
                glm::vec3(box.w, box.h, 1.f));
        glm::vec4 boxColor(0.1f, 0.1f, 0.1f, 0.0f);
        renderRectangle(stageTrans, boxColor);

        // draw stage name
        glm::mat4 nameTrans = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(box.x, box.y, 0.05f)),
                glm::vec3(box.h/5.f, -box.h/5.f, 1.f));
        glm::vec3 nameColor(0.7f);
        FontManager::get()->renderString(nameTrans, nameColor, stages_[i]);

        // draw outline rect, change color if selected
        glm::vec4 outlineColor(0.8f, 0.8f, 0.8f, 0.f);
        if (i == curStage_)
            outlineColor = glm::vec4(0.7f, 0.7f, 0.2f, 0.0f);

        const float outlineSize = 10.f;
        glm::mat4 outlineTrans = glm::scale(
                glm::translate(glm::mat4(1.f), glm::vec3(box.x, box.y, 0.f)),
                glm::vec3(box.w + outlineSize, box.h + outlineSize, 1.f));
        renderRectangle(outlineTrans, outlineColor);
    }
}

rectangle LevelSelectState::stageRect(int idx)
{
    const int nstages = stages_.size();
    const glm::vec2 boxsz(300.f, 200.f);
    const glm::vec2 boxmargin(200.f);

    glm::vec2 pos = glm::vec2(1920.f, 1080.f)/2.f;
    pos.x += (boxsz.x + boxmargin.x) * (idx - nstages/2.f + 0.5f);

    return rectangle(pos, boxsz);
}

GameState* LevelSelectState::newGame(const std::vector<Controller*>& controllers)
{
    assert(controllers.size() <= getParam("maxPlayers"));
    int lives = getParam("menu.lives");
    bool recordStats = getParam("menu.recordStats");
    bool handicap = getParam("menu.handicap");
    std::string stage = strParam("menu.stage");

    std::vector<Player *> players;
    std::vector<Fighter *> fighters;

    for (size_t i = 0; i < controllers.size(); i++)
    {
        std::string prefix = MenuState::getPlayerMenuPrefix(i);
        if (getParam(prefix + "active"))
        {
            int teamID = getParam(prefix + "teamIDidx");
            glm::vec3 color = MenuState::getPlayerColor(i);
            std::string username = strParam(prefix + "profile");
            std::string fighterName = strParam(prefix + "fighter");

            int handicapLives = handicap ? getParam(prefix + "handicap") : 0;

            Fighter *fighter = new Fighter(color, i, teamID,
                    lives + handicapLives, username, fighterName);
            fighters.push_back(fighter);

            Player *player = NULL;
            if (username == StatsManager::ai_user)
            {
                player = new AIPlayer(fighter);
            }
            else if (username == StatsManager::ghost_ai_user)
            {
                player = new GhostAIPlayer(fighter);
            }
            else 
            {
                player = new LocalPlayer(controllers[i], fighter);
                controllers[i]->clearPresses();
            }
            players.push_back(player);
        }
    }

    // Create game mode
    GameMode *gameMode;
    if (getParam("debug"))
        gameMode = new DebugGameMode();
    else
        gameMode = new StockGameMode();

    GameState *gs = new InGameState(players, fighters, recordStats, stage, gameMode);
    return gs;
}
