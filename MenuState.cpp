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
#include "audio.h"
#include "ParamReader.h"

#define JOYSTICK_START 7
#define MAX_JOYSTICK_VALUE 32767.0f

const glm::vec3 playerColors[] =
{
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.1, 0.6, 0.1),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.7, 0.7, 0.2)
};
const glm::vec3 teamColors[] =
{
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.2, 0.6, 0.8),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.8, 0.35, 0.1)
};

MenuWidget::MenuWidget(const std::string &name, int min, int max, int defval) :
    name_(name), min_(min), max_(max), value_(min), primed_(false)
{
    if (defval != -1)
        value_ = defval;
}

void MenuWidget::handleInput(float val)
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

int numDigits(int num)
{
    int count;
    for (count = 0; num != 0; num /= 10, count++);

    return std::max(count, 1);
}

void MenuWidget::render(const glm::vec2 &center, bool selected) const
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
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * numDigits(value_), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, charsize, 1.f));
    FontManager::get()->renderNumber(
            transform,
            color,
            value_);

}

int MenuWidget::value() const
{
    return value_;
}

MenuState::MenuState() :
    rowChangePrimed_(false),
    startPrimed_(false),
    widgetInd_(0)
{
    // Start the menu soundtrack
    AudioManager::get()->setSoundtrack("sfx/02 Escape Velocity (loop).ogg");
    AudioManager::get()->startSoundtrack();

    widgets.push_back(new MenuWidget("Players", 2, 4));
    widgets.push_back(new MenuWidget("Teams", 0, 1));
    widgets.push_back(new MenuWidget("Lives", 1, 99, 4));
    widgets.push_back(new MenuWidget("Stage Hazard", 0, 1));
}

MenuState::~MenuState()
{
    for (size_t i = 0; i < widgets.size(); i++)
        delete widgets[i];
}

void MenuState::update(float)
{
    // nop
}


void MenuState::render(float dt)
{
    setProjectionMatrix(glm::ortho(0.f, 1920.f, 0.f, 1080.f, -1.f, 1.f));
    setViewMatrix(glm::mat4(1.f));

    glm::vec2 center(1920.f / 2, 1080.f - 1080.f / 8 * 2);

    for (size_t i = 0; i < widgets.size(); i++)
    {
        bool selected = static_cast<int>(i) == widgetInd_;
        widgets[i]->render(center, selected);
        center -= glm::vec2(0, 1080.f / 8);
    }
}

GameState* MenuState::processInput(const std::vector<SDL_Joystick*> &stix, float)
{
    assert(stix.size() > 0);
    // Only player one may move through the menus
    SDL_Joystick* p1 = stix[0];

    bool startbutton = SDL_JoystickGetButton(p1, JOYSTICK_START);
    float xval = SDL_JoystickGetAxis(p1, 0) / MAX_JOYSTICK_VALUE; 
    float yval = SDL_JoystickGetAxis(p1, 1) / MAX_JOYSTICK_VALUE; 

    // Check for transition to InGamestate
    if (startbutton)
    {
        if (startPrimed_)
            return newGame(stix);
    }
    else
        startPrimed_ = true;

    // Check for row change
    if (fabs(yval) > getParam("menu.deadzone") && rowChangePrimed_)
    {
        int sign = yval > 0 ? 1 : -1;
        // Move and clamp
        widgetInd_ += sign;
        widgetInd_ = std::max(std::min(widgetInd_, ((int) widgets.size())-1), 0);
        
        std::cout << "Changing rows: " << widgetInd_ << '\n';

        rowChangePrimed_ = false;
    }
    else if (fabs(yval) < getParam("menu.deadzone"))
        rowChangePrimed_ = true;

    // Check for value change etc
    widgets[widgetInd_]->handleInput(xval);

    // No transition
    return NULL;
}

GameState* MenuState::newGame(const std::vector<SDL_Joystick*> &stix)
{
    int nplayers = widgets[0]->value();
    bool teams = widgets[1]->value();
    int lives = widgets[2]->value();

    const glm::vec3 *colors = teams ? teamColors : playerColors;

    std::vector<Controller *> controllers;
    std::vector<Fighter *> fighters;
    for (int i = 0; i < nplayers; i++)
    {
        int playerID = i;
        int teamID = teams ? (i < 2 ? 0 : 1) : playerID;
        // create a new fighter
        Fighter *f = new Fighter(
                colors[i],
                playerID,
                teamID,
                lives);

        // create a controller
        // push the controller onto the list
        controllers.push_back(new Controller(f, stix[i]));
        fighters.push_back(f);
    }
        

    // If p1 is pressing start, create a new GameState 
    GameState *gs = new InGameState(controllers, fighters);
    return gs;
}

