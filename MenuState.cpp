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
    glm::vec3(0.0, 0.2, 1.0), // blue
    glm::vec3(0.1, 0.6, 0.1), // green
    glm::vec3(0.8, 0.2, 0.2), // red
    glm::vec3(0.7, 0.7, 0.2)  // yellow
};
static const glm::vec3 alternateColors[] =
{
    glm::vec3(0.2, 0.6, 0.8), // teal
    glm::vec3(0.2, 0.6, 0.8), // teal
    glm::vec3(0.8, 0.35, 0.1), // orange
    glm::vec3(0.8, 0.35, 0.1) // orange
};
static const std::string teamStrs_[] = {"A", "B", "C", "D"};
static const std::string *teamStrs__ = teamStrs_;
static const std::vector<std::string> teamStrs(teamStrs__, teamStrs__ + 4);
static int deflives = 4;

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

void NumberSelectWidget::render(const glm::vec2 &center, const glm::vec2 &size,
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
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * FontManager::numDigits(value_), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderNumber(
            transform,
            color,
            value_);
}

int NumberSelectWidget::value() const
{
    return value_;
}

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

PlayerWidget::PlayerWidget(int playerID) :
    playerID_(playerID),
    active_(false), holdStart_(false), pressStart_(false),
    usernameWidget_(NULL),
    teamIDWidget_(NULL),
    widgetIdx_(0),
    widgetChangePrimed_(false)
{
    usernameWidget_ = new StringSelectWidget("Profile ", StatsManager::get()->getUsernames(), 0);
    teamIDWidget_ = new StringSelectWidget("Team ", teamStrs, playerID);
    widgets_.push_back(usernameWidget_);
    widgets_.push_back(teamIDWidget_);
}

PlayerWidget::~PlayerWidget()
{
    delete usernameWidget_;
    delete teamIDWidget_;
}

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
    float yval = SDL_JoystickGetAxis(joystick, 1) / MAX_JOYSTICK_VALUE;

    if (fabs(yval) > getParam("menu.thresh") && widgetChangePrimed_)
    {
        widgetIdx_ += glm::sign(yval);
        widgetIdx_ = std::max(std::min(widgetIdx_, (int)widgets_.size() - 1), 0);
    }
    else if (fabs(yval) <= getParam("menu.thresh"))
        widgetChangePrimed_ = true;

    widgets_[widgetIdx_]->handleInput(xval);
}

glm::vec3 PlayerWidget::getColor(int colorScheme) const
{
    if (colorScheme == 0)
        return playerColors[teamIDWidget_->value()];
    else if (colorScheme == 1)
        return alternateColors[teamIDWidget_->value()];
    else
        assert(false && "Unknown color scheme to PlayerWidget::getColor");
}

void PlayerWidget::render(const glm::vec2 &center, const glm::vec2 &size, float dt)
{
        glm::mat4 trans;
        const glm::vec3 color = getColor();

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
            FrameManager::get()->renderFrame(trans, glm::vec4(color, 0.15f), "GroundNormal");

            // XXX: use loop here over widgets_ instead
            usernameWidget_->render(center - glm::vec2(size.x * 0.1, size.y * 0.25),
                    size * glm::vec2(0.33f, 0.1), widgetIdx_ == 0);
            teamIDWidget_->render(center - glm::vec2(size.x * 0.1, 2 * 0.25 + size.y * .1),
                    size * glm::vec2(0.33f, 0.1), widgetIdx_ == 1);
        }

}

void PlayerWidget::getController(int lives, SDL_Joystick *stick, int colorScheme,
        Fighter **outfighter, Controller **outcntrl) const
{
    glm::vec3 color = getColor(colorScheme);
    *outfighter = new Fighter(
            color,
            playerID_,
            teamIDWidget_->value(),
            lives,
            usernameWidget_->strValue());

    *outcntrl = new Controller(*outfighter, stick);
}

int PlayerWidget::getTeamID() const
{
    return teamIDWidget_->value();
}

MenuState::MenuState()
{
    // Start the menu soundtrack
    AudioManager::get()->setSoundtrack("sfx/02 Escape Velocity (loop).ogg");
    AudioManager::get()->startSoundtrack();

    std::vector<std::string> boolstrs;
    boolstrs.push_back("no"); boolstrs.push_back("yes");

    for (int i = 0; i < 4; i++)
        widgets_.push_back(new PlayerWidget(i));

    topWidgets_.push_back(new NumberSelectWidget("Lives", 1, 99, deflives));

    getProjectionMatrixStack().clear();
    getViewMatrixStack().clear();
    getProjectionMatrixStack().current() = glm::ortho(0.f, 1920.f, 1080.f, 0.f, -1.f, 1.f);

    topSelected_[0] = topSelected_[1] = topSelected_[2] = topSelected_[3] = false;
}

MenuState::~MenuState()
{
    for (size_t i = 0; i < widgets_.size(); i++)
        delete widgets_[i];
    for (size_t i = 0; i < topWidgets_.size(); i++)
        delete topWidgets_[i];
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
    for (size_t i = 0; i < widgets_.size(); i++)
    {
        int x = i % 2;
        int y = i / 2;
        glm::vec2 center(xoffset + xsize * (x + 0.5f), yoffset + ysize * (y + 0.5f));
        glm::vec2 size(xsize, ysize);


        widgets_[i]->render(center, size, dt);
    }

    for (size_t i = 0; i < topWidgets_.size(); i++)
    {
        glm::vec2 center(1920.f/2, yoffset/2);
        glm::vec2 size(1920.f, yoffset/2 * 0.75);

        topWidgets_[i]->render(center, size, dt);
    }
}

GameState* MenuState::processInput(const std::vector<SDL_Joystick*> &stix, float dt)
{
    assert(stix.size() >= widgets_.size());

    bool shouldStart = false;
    int numPlayers = 0;
    for (size_t i = 0; i < widgets_.size(); i++)
    {
        // Check for y button to check for top selected
        if (SDL_JoystickGetButton(stix[i], 3))
        {
            float xval = SDL_JoystickGetAxis(stix[i], 0) / MAX_JOYSTICK_VALUE;;
            topWidgets_[0]->handleInput(xval);
            continue;
        }

        widgets_[i]->processInput(stix[i], dt);
        shouldStart |= widgets_[i]->wantsStart();
        if (widgets_[i]->isActive())
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
    int lives = topWidgets_[0]->value();
    bool hazard = false;

    std::vector<Controller *> controllers;
    std::vector<Fighter *> fighters;

    int teamCounts[4] = {0, 0, 0, 0};

    for (size_t i = 0; i < widgets_.size(); i++)
    {
        if (widgets_[i]->isActive())
        {
            Controller *controller = 0;
            Fighter *fighter = 0;

            widgets_[i]->getController(lives, sticks[i], teamCounts[widgets_[i]->getTeamID()],
                    &fighter, &controller);

            std::cout << "Fighter: " << fighter << " Controller: " << controller << '\n';

            fighters.push_back(fighter);
            controllers.push_back(controller);

            teamCounts[fighter->getTeamID()]++;
        }
    }


    // If p1 is pressing start, create a new GameState 
    GameState *gs = new InGameState(controllers, fighters, hazard);
    return gs;

    deflives = lives;

    return NULL;
}

