#include "MenuState.h"
#include "InGameState.h"
#include "GameState.h"
#include "Fighter.h"
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "glutils.h"
#include "FontManager.h"
#include "AudioManager.h"
#include "StatsManager.h"
#include "ParamReader.h"
#include "FrameManager.h"

#define JOYSTICK_START 7
#define MAX_JOYSTICK_VALUE 32767.0f

static const glm::vec3 playerColors[] =
{
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.1, 0.6, 0.1),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.7, 0.7, 0.2)
};
static const glm::vec3 teamColors[] =
{
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.2, 0.6, 0.8),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.8, 0.35, 0.1)
};

/*
static int defplayers = 2;
static int defteams = 0;
static int deflives = 4;
static int defhazard = 0;

NumberSelectWidget::NumberSelectWidget(const std::string &name, int min, int max, int defval) :
    name_(name), min_(min), max_(max), value_(min), primed_(false)
{
    if (defval != -1)
        value_ = defval;
}

void NumberSelectWidget::handleInput(float val)
{
    if (fabs(val) < getParam("menu.deadzone"))
    {
        primed_ = true;
        return;
    }

    if (!primed_)
        return;

    int sign = val > 0 ? 1 : -1;

    value_ += sign;
    value_ = std::max(std::min(value_, max_), min_);
    primed_ = false;
}

void NumberSelectWidget::render(const glm::vec2 &center, bool selected) const
{
    static const float charsize = 100.f;
    glm::vec3 color = selected ? glm::vec3(.9, .9, .9) : glm::vec3(.3, .3, .3);
    glm::mat4 transform;

    // Render name
    glm::vec2 strcenter = center - glm::vec2(0.75f * charsize * name_.length()/2, 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(strcenter, 0.f)),
            glm::vec3(charsize, charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            name_);

    // Render value
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * FontManager::numDigits(value_), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, charsize, 1.f));
    FontManager::get()->renderNumber(
            transform,
            color,
            value_);
}

int NumberSelectWidget::value() const
{
    return value_;
}
*/

StringSelectWidget::StringSelectWidget(const std::string &name,
        const std::vector<std::string> &strings, int defval) :
    name_(name), values_(strings), idx_(defval), primed_(false)
{
}

int StringSelectWidget::value() const
{
    return idx_;
}

std::string StringSelectWidget::strValue() const
{
    return values_[idx_];
}

void StringSelectWidget::handleInput(float val)
{
    if (fabs(val) < getParam("menu.deadzone"))
    {
        primed_ = true;
        return;
    }

    if (!primed_)
        return;

    int sign = val > 0 ? 1 : -1;

    idx_ += sign;
    idx_ = std::max(std::min(idx_, (int)values_.size() - 1), 0);
    primed_ = false;
}

void StringSelectWidget::render(const glm::vec2 &center, const glm::vec2 &size,
        bool selected) const
{
    static const float charsize = size.y;
    glm::vec3 color = selected ? glm::vec3(.9, .9, .9) : glm::vec3(.3, .3, .3);
    glm::mat4 transform;

    // Render name
    glm::vec2 strcenter = center - glm::vec2(0.75f * charsize * name_.length()/2, 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(strcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            name_);

    // Render value
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * (values_[idx_].length()/2 + 1), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            values_[idx_]);
}

PlayerWidget::PlayerWidget(const glm::vec3 &col, const glm::vec3 &teamcol,
        int playerID) :
    playerID_(playerID), col_(col), teamcol_(teamcol),
    usernameWidget_("Profile ", StatsManager::get()->getUsernames(), 0)
{
}

PlayerWidget::~PlayerWidget() { }

bool PlayerWidget::isActive() const { return active_; }
bool PlayerWidget::wantsStart() const { return pressStart_; }

void PlayerWidget::processInput(SDL_Joystick *joystick, float dt)
{
    // A to join
    if (SDL_JoystickGetButton(joystick, 0))
        active_ = true;

    if (!active_)
        return;

    // B to leave
    if (SDL_JoystickGetButton(joystick, 1))
        active_ = false;

    // Start button
    if (SDL_JoystickGetButton(joystick, 7))
    {
        pressStart_ = !holdStart_;
        holdStart_ = true;
    }
    else
    {
        pressStart_ = holdStart_ = false;
    }

    // Left stick
    float xval = SDL_JoystickGetAxis(joystick, 0) / MAX_JOYSTICK_VALUE;
    float yval = -SDL_JoystickGetAxis(joystick, 1) / MAX_JOYSTICK_VALUE;

    usernameWidget_.handleInput(xval);
}

void PlayerWidget::render(const glm::vec2 &center, const glm::vec2 &size, float dt)
{
        glm::mat4 trans;

        if (!active_)
        {
            trans = glm::scale(glm::translate(glm::mat4(1.f),
                        glm::vec3(center.x, center.y, -0.1f)),
                    glm::vec3(50.f, -50.f, 0.f));

            FontManager::get()->renderString(trans, glm::vec3(0.5, 0.5, 0.5),
                    "Press A to join");
        }
        else
        {
            trans = glm::scale(glm::translate(glm::mat4(1.f),
                        glm::vec3(center.x + 0.3f * size.x, center.y, -0.1f)),
                    glm::vec3(5.f, -5.f, 0.f));
            FrameManager::get()->renderFrame(trans, glm::vec4(col_, 0.f), "GroundNormal");

            usernameWidget_.render(center - glm::vec2(size.x * 0.1, size.y * 0.25),
                    size * glm::vec2(0.33f, 0.1), dt);
        }

}

void PlayerWidget::getController(int teamID, int lives, SDL_Joystick *stick,
        Fighter **outfighter, Controller **outcntrl) const
{
    *outfighter = new Fighter(
            col_,
            playerID_,
            teamID,
            lives,
            usernameWidget_.strValue());

    *outcntrl = new Controller(*outfighter, stick);
}

MenuState::MenuState()
{
    // Start the menu soundtrack
    AudioManager::get()->setSoundtrack("sfx/02 Escape Velocity (loop).ogg");
    AudioManager::get()->startSoundtrack();

    std::vector<std::string> boolstrs;
    boolstrs.push_back("no"); boolstrs.push_back("yes");

    for (int i = 0; i < 4; i++)
    {
        widgets_.push_back(PlayerWidget(playerColors[i], teamColors[i], i));
    }

    getProjectionMatrixStack().clear();
    getViewMatrixStack().clear();
    getProjectionMatrixStack().current() = glm::ortho(0.f, 1920.f, 1080.f, 0.f, -1.f, 1.f);
}

MenuState::~MenuState()
{
}

void MenuState::update(float)
{
    // nop
}


void MenuState::render(float dt)
{
    const float xsize = 1920.f/2;
    const float ysize = 500.f;
    const float xoffset = 0.f;
    const float yoffset = 1080.f - 2 * ysize;
    for (int i = 0; i < widgets_.size(); i++)
    {
        int x = i % 2;
        int y = i / 2;
        glm::vec2 center(xoffset + xsize * (x + 0.5f), yoffset + ysize * (y + 0.5f));
        glm::vec2 size(xsize, ysize);


        widgets_[i].render(center, size, dt);
    }
}

GameState* MenuState::processInput(const std::vector<SDL_Joystick*> &stix, float dt)
{
    assert(stix.size() >= widgets_.size());

    bool shouldStart = false;
    int numPlayers = 0;
    for (int i = 0; i < widgets_.size(); i++)
    {
        widgets_[i].processInput(stix[i], dt);
        shouldStart |= widgets_[i].wantsStart();
        if (widgets_[i].isActive())
            numPlayers += 1;
    }

    if (numPlayers >= 2 && shouldStart)
    {
        return newGame(stix);
    }

    // No transition
    return NULL;
}

GameState* MenuState::newGame(const std::vector<SDL_Joystick*> &sticks)
{
    // TODO fix these
    bool teams = false;
    int lives = 1;
    bool hazard = false;

    std::vector<Controller *> controllers;
    std::vector<Fighter *> fighters;

    for (size_t i = 0; i < widgets_.size(); i++)
    {
        if (widgets_[i].isActive())
        {
            Controller *controller = 0;
            Fighter *fighter = 0;

            // TODO take into account teams
            widgets_[i].getController(i, lives, sticks[i], &fighter, &controller);

            std::cout << "Fighter: " << fighter << " Controller: " << controller << '\n';

            fighters.push_back(fighter);
            controllers.push_back(controller);
        }
    }


    // If p1 is pressing start, create a new GameState 
    GameState *gs = new InGameState(controllers, fighters, hazard);
    return gs;

    return NULL;
}

