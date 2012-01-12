#include "StatsGameState.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include "Fighter.h"
#include "glutils.h"
#include "StatsManager.h"
#include "FontManager.h"
#include "FrameManager.h"
#include "AudioManager.h"
#include "MenuState.h"

fighter_stat::fighter_stat(const std::string &sname, const std::string &dname) :
    stat_name(sname), display_name(dname)
{
}

StatsGameState::StatsGameState(
        const std::vector<Fighter *> fighters,
        int winningTeam) :
    fighters_(fighters),
    winningTeam_(winningTeam)
{
    backgroundTex_ = make_texture("images/back003.png");

    AudioManager::get()->setSoundtrack("sfx/09 Virtual Void (loop).ogg");
    AudioManager::get()->startSoundtrack();

    stats_.push_back(new fighter_stat("kills.total", "Kills"));
    stats_.push_back(new fighter_stat("deaths", "Deaths"));
    stats_.push_back(new fighter_stat("suicides", "Suicides"));
    stats_.push_back(new fighter_stat("damageGiven", "Damage Given"));
    stats_.push_back(new fighter_stat("damageTaken", "Damage Taken"));
    stats_.push_back(new fighter_stat("teamDamageGiven", "Team Damage"));
    stats_.push_back(new fighter_stat("maxKillStreak", "Max KO Streak"));

    for (size_t i = 0; i < fighters_.size(); i++)
    {
        std::stringstream ss;
        ss << "Player" << i;
        stats_.push_back(
                new fighter_stat("kills." + ss.str(), ss.str() + " KOs"));
    }

    // Print stats to console
    StatsManager::get()->printStats();


    // Set up matrices
    getProjectionMatrixStack().clear();
    getProjectionMatrixStack().current() = glm::ortho(0.f, 1920.f, 0.f, 1080.f, -1.f, 1.f);
    getViewMatrixStack().clear();
}

StatsGameState::~StatsGameState()
{
    for (size_t i = 0; i < fighters_.size(); i++)
        delete fighters_[i];

    free_texture(backgroundTex_);
}

GameState * StatsGameState::processInput(const std::vector<SDL_Joystick*> &joysticks,
        float dt)
{
    static const int JOYSTICK_START = 7;
    assert(joysticks.size() > 0);

    bool startbutton = SDL_JoystickGetButton(joysticks[0], JOYSTICK_START);

    if (startbutton)
        return new MenuState();

    return NULL;
}

void StatsGameState::update(float dt)
{
    // Empty
}

const float columnWidth = 1920.f/4 - 10.f;
float columnCenter(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 1.5f);
}

float columnLeft(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 2.0f);
}

float columnRight(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 1.0f);
}

void StatsGameState::render(float dt)
{
    // Draw the background
    glm::mat4 backtrans = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(1920.f/2, 1080.f/2, -0.1f)),
                glm::vec3(1920.f, -1080.f, 1.f));
    renderTexturedRectangle(backtrans, backgroundTex_);

    // Some constants
    const float fighter_height = 1080.f - 1080.f/6.f;
    const float stat_size = 35.f;
    const float stat_height = 1080.f - 1080.f/3 - stat_size;;
    const float stat_xmargin = 10.f;
    const glm::vec3 stat_color(0.8f, 0.8f, 0.8f);


    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        glm::vec4 fighter_color = glm::vec4(fighters_[i]->getColor(), 0.f);
        if (fighters_[i]->getTeamID() == winningTeam_)
            fighter_color.a = 0.5f;
        glm::mat4 transform;

        // First draw fighter at top of column
        transform = glm::scale(
                glm::translate(glm::mat4(1.f),
                    glm::vec3(columnCenter(i), fighter_height, 0.0f)),
                glm::vec3(3.f, 3.f, 1.f));
        FrameManager::get()->renderFrame(transform, fighter_color,  "GroundNormal");

        // Next loop over the stats and render each
        for (size_t j = 0; j < stats_.size(); j++)
        {
            fighter_stat *stat = stats_[j];

            // Render the display name
            float xoffset = stat_size * 0.75f * stat->display_name.length() / 2.f + stat_xmargin;
            float yoffset = -j * stat_size * 1.5f;
            transform = glm::scale(
                    glm::translate(glm::mat4(1.f),
                        glm::vec3(columnLeft(i) + xoffset, stat_height + yoffset, 0.0f)),
                    glm::vec3(stat_size, stat_size, 1.f));
            FontManager::get()->renderString(transform, stat_color, stat->display_name);

            // Now render value
            int value = StatsManager::get()->getStat(
                    StatsManager::getStatPrefix(fighters_[i]->getPlayerID()) +
                    stat->stat_name);
            xoffset = -stat_size * 0.75f * FontManager::numDigits(value) / 2.f - stat_xmargin;
            transform = glm::scale(
                    glm::translate(glm::mat4(1.f),
                        glm::vec3(columnRight(i) + xoffset, stat_height + yoffset, 0.0f)),
                    glm::vec3(stat_size, stat_size, 1.f));
            FontManager::get()->renderNumber(transform, stat_color, value);
        }
    }
}
