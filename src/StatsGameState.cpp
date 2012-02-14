#include "StatsGameState.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include "Fighter.h"
#include "Engine.h"
#include "StatsManager.h"
#include "FontManager.h"
#include "FrameManager.h"
#include "AudioManager.h"
#include "MenuState.h"
#include "Player.h"

fighter_stat::fighter_stat(const std::string &sname, const std::string &dname) :
    stat_name(sname), display_name(dname)
{
}

void fighter_stat::render(const rectangle &rect, int playerID)
{
    const glm::vec3 stat_color(0.8f, 0.8f, 0.8f);
    glm::mat4 transform =
        glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(rect.x, rect.y, 0.f)),
                glm::vec3(rect.w, rect.h, 1.f));
    //renderRectangle(transform, glm::vec4(1,0,0,0));

    // First render the stat name
    transform = glm::scale(glm::translate(glm::mat4(1.f),
                glm::vec3(rect.x - rect.w / 2 + rect.h * 0.75f * display_name.length() / 2.f, rect.y, 0.f)),
            glm::vec3(rect.h, rect.h, 1.f));
    FontManager::get()->renderString(transform, stat_color, display_name);

    // Calculate the stat value
    int value = StatsManager::get()->getStat(
            StatsManager::getStatPrefix(playerID) + stat_name);
    // And now the stat value
    transform = glm::scale(glm::translate(glm::mat4(1.f),
                glm::vec3(rect.x + rect.w / 2 - rect.h * 0.75f * FontManager::numDigits(value) / 2.f, rect.y, 0.f)),
            glm::vec3(rect.h, rect.h, 1.f));
    FontManager::get()->renderNumber(transform, stat_color, value);
}

StatsGameState::StatsGameState(
        const std::vector<Player *> players,
        int winningTeam) :
    players_(players),
    winningTeam_(winningTeam),
    ready_(players.size(), false)
{
    backgroundTex_ = make_texture("images/back003.png");

    AudioManager::get()->setSoundtrack("sfx/09 Virtual Void (loop).ogg");
    AudioManager::get()->startSoundtrack();

    stats_.push_back(new fighter_stat("kills.total", "Kills"));
    stats_.push_back(new fighter_stat("deaths", "Deaths"));
    stats_.push_back(new fighter_stat("suicides", "Suicides"));
    stats_.push_back(new fighter_stat("damageGiven", "Damage Given"));
    stats_.push_back(new fighter_stat("damageTaken", "Damage Taken"));
    stats_.push_back(new fighter_stat("maxDamageStreak", "Damage Streak"));
    stats_.push_back(new fighter_stat("maxKillStreak", "Max KO Streak"));
    stats_.push_back(new fighter_stat("deathTime", "Seconds Alive"));
    // TODO check to see if it's a team game for these ones
    // if (teamGame_)
    stats_.push_back(new fighter_stat("kills.team", "Team Kills"));
    stats_.push_back(new fighter_stat("teamDamageGiven", "Team Damage"));

    for (size_t i = 0; i < players_.size(); i++)
    {
        std::stringstream ss;
        ss << "Player" << players_[i]->getPlayerID();
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
    for (size_t i = 0; i < players_.size(); i++)
        delete players_[i];
    for (size_t i = 0; i < stats_.size(); i++)
        delete stats_[i];

    free_texture(backgroundTex_);
}

GameState * StatsGameState::processInput(const std::vector<Controller*> &controllers,
        float dt)
{
    for (size_t i = 0; i < players_.size(); i++)
    {
        controller_state cs = players_[i]->getState();
        if (players_[i]->wantsStatsContinue())
        {
            ready_[i] = true;
            continue;
        }
        // Check if they're ready
        if (cs.pressb)
            ready_[i] = false;
        if (cs.pressa)
            ready_[i] = true;
    }

    // if everyone is ready, transition
    bool allReady = true;
    for (size_t i = 0; i < ready_.size(); i++)
        allReady &= ready_[i];

    if (allReady)
        return new MenuState();

    return NULL;
}

void StatsGameState::preFrame()
{
    // nop
}

void StatsGameState::postFrame()
{
    // nop
}

void StatsGameState::update(float dt)
{
    // nop
}

const float columnWidth = 1920.f/4 - 10.f;
// The border around each column
const float columnBorder = columnWidth / 20;
float columnCenter(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 1.5f);
}

float columnLeft(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 2.0f) + columnBorder;
}

float columnRight(int col)
{
    assert(col >= 0 && col < 4);
    return 1920.f/2 + columnWidth * (col - 1.0f) - columnBorder;
}

void StatsGameState::render(float dt)
{
    // Draw the background
    glm::mat4 backtrans = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(1920.f/2, 1080.f/2, -0.1f)),
                glm::vec3(1920.f, -1080.f, 1.f));
    renderTexturedRectangle(backtrans, backgroundTex_);

    // Some constants
    const float name_height = 1080.f - 1080.f/15.f;
    const float name_size = 60.f;
    const float fighter_height = 1080.f - 1080.f/5.f;
    const float stat_size = 35.f;
    const float stat_height = 1080.f - 1080.f/3 - stat_size;
    const glm::vec3 stat_color(0.8f, 0.8f, 0.8f);


    for (unsigned i = 0; i < players_.size(); i++)
    {
        glm::vec4 fighter_color = glm::vec4(players_[i]->getColor(), 0.f);
        if (players_[i]->getTeamID() == winningTeam_)
            fighter_color.a = 0.5f;
        glm::mat4 transform;

        // Fighter place
        transform = glm::scale(
                glm::translate(glm::mat4(1.f),
                    glm::vec3(columnCenter(i) - stat_size * (players_[i]->getUsername().length()/2 + 1.5),
                        name_height, 0.0f)),
                glm::vec3(name_size, name_size, 1.f));
        FontManager::get()->renderNumber(transform, glm::vec3(fighter_color),
                StatsManager::get()->getStat(statPrefix(players_[i]->getPlayerID()) + "place"));

        // Profile name
        transform = glm::scale(
                glm::translate(glm::mat4(1.f),
                    glm::vec3(columnCenter(i) + stat_size, name_height, 0.0f)),
                glm::vec3(name_size, name_size, 1.f));
        FontManager::get()->renderString(transform, glm::vec3(fighter_color), players_[i]->getUsername());

        // TODO draw correct fighter, not always charlie
        // draw fighter at top of column, only if not ready
        assert(ready_.size() == players_.size());
        if (!ready_[i])
        {
            transform = glm::scale(
                    glm::translate(glm::mat4(1.f),
                        glm::vec3(columnCenter(i), fighter_height, 0.0f)),
                    glm::vec3(3.f, 3.f, 1.f));
            FrameManager::get()->renderFrame(transform, fighter_color,  "GroundNormal");
        }

        // Next loop over the stats and render each
        for (size_t j = 0; j < stats_.size(); j++)
        {
            fighter_stat *stat = stats_[j];
            rectangle r;
            r.x = columnCenter(i);
            r.y = stat_height - static_cast<int>(j) * stat_size * 1.5f;
            r.w = columnWidth - 2 * columnBorder;
            r.h = stat_size;
            stat->render(r, players_[i]->getPlayerID());
        }
    }
}
