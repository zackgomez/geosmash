#include "StatsGameState.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Fighter.h"
#include "glutils.h"
#include "StatsManager.h"
#include "FontManager.h"
#include "FrameManager.h"

StatsGameState::StatsGameState(
        const std::vector<Fighter *> fighters,
        int winningTeam) :
    fighters_(fighters),
    winningTeam_(winningTeam)
{
    backgroundTex_ = make_texture("images/back003.png");
}

StatsGameState::~StatsGameState()
{
    for (size_t i = 0; i < fighters_.size(); i++)
        delete fighters_[i];

    // TODO delete background tex
    free_texture(backgroundTex_);
}

GameState * StatsGameState::processInput(const std::vector<SDL_Joystick*> &joysticks,
        float dt)
{
    // TODO go to menu state on start
    return NULL;
}

void StatsGameState::update(float dt)
{
    // Empty
}

void StatsGameState::render(float dt)
{
    // Set up matrices
    setProjectionMatrix(glm::ortho(0.f, 1920.f, 0.f, 1080.f, -1.f, 1.f));
    setViewMatrix(glm::mat4(1.f));

    // Draw the background
    glm::mat4 backtrans = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(1920.f/2, 1080.f/2, 0)),
                glm::vec3(1920.f, -1080.f, 1.f));
    renderTexturedRectangle(backtrans, backgroundTex_);

    // Draw the players, highlight the winner
    glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(1920.f/10 + 1920.f/5, 1080.f - 1080.f/3/2, 0.1f)), glm::vec3(1.f, 1.f, 1.f));
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        float glow = 0.5f;
        if (fighters_[i]->getTeamID() != winningTeam_)
            glow = 0.0f;
        FrameManager::get()->renderFrame(glm::scale(transform, glm::vec3(3.f, 3.f, 0.f)), glm::vec4(fighters_[i]->getColor(), glow), "GroundNormal");
        transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    }
    // Kills
    transform = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(1920.f/10, 1080.f - 1080.f/3 - 1080.f/3/2, 0.1f)), glm::vec3(1.f, 1.f, 1.f));
    // Draw banner
    FrameManager::get()->renderFrame(glm::scale(transform, glm::vec3(3.f, 3.f, 0.f)), glm::vec4(1.f, 1.f, 1.f, 0.f), "KO");
    transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        float kills = StatsManager::get()->getStat(StatsManager::getStatPrefix(fighters_[i]->getPlayerID()) + "kills.total");
        FontManager::get()->renderNumber(glm::scale(transform, glm::vec3(100.f, 100.f, 1.f)), fighters_[i]->getColor(), kills);
        transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    }
    // Damage
    transform = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(1920.f/10, 1080.f - 2*1080.f/3 - 1080.f/3/2, 0.1f)), glm::vec3(1.f, 1.f, 1.f));
    // Draw banner
    FrameManager::get()->renderFrame(glm::scale(transform, glm::vec3(3.f, 3.f, 0.f)), glm::vec4(1.f, 1.f, 1.f, 0.f), "DMG");
    transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        float damage = StatsManager::get()->getStat(StatsManager::getStatPrefix(fighters_[i]->getPlayerID()) + "damageGiven");
        FontManager::get()->renderNumber(glm::scale(transform, glm::vec3(100.f, 100.f, 1.f)), fighters_[i]->getColor(), damage);
        transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    }
}
