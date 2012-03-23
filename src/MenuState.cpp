#include "MenuState.h"
#include "LevelSelectState.h"
#include "GameState.h"
#include "Fighter.h"
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "Engine.h"
#include "FontManager.h"
#include "AudioManager.h"
#include "StatsManager.h"
#include "StageManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "Player.h"
#include "kiss-skeleton.h"

void checkForJoysticks(unsigned maxPlayers);

static const glm::vec3 playerColors__[] =
{
    glm::vec3(0.0, 0.2, 1.0), // blue
    glm::vec3(0.1, 0.6, 0.1), // green
    glm::vec3(0.2, 0.6, 0.8), // teal
    glm::vec3(0.8, 0.2, 0.2), // red
    glm::vec3(0.7, 0.7, 0.2), // yellow
    glm::vec3(0.8, 0.35, 0.1), // orange
    glm::vec3(0.4, 0.2, 0.9), // purple
    glm::vec3(0.8, 0.2, 0.8), // pink
    glm::vec3(0.6, 0.6, 0.6), // gray
};

std::vector<glm::vec3> playerColors(playerColors__,
        playerColors__ + (sizeof(playerColors__) / sizeof(glm::vec3)));

static const glm::vec3 teamColorsA[] =
{
    glm::vec3(0.0, 0.2, 1.0), // blue
    glm::vec3(0.1, 0.6, 0.1), // green
    glm::vec3(0.8, 0.2, 0.2), // red
    glm::vec3(0.3, 0.1, 0.5), // purple
};

static const glm::vec3 teamColorsB[] =
{
    glm::vec3(0.2, 0.6, 0.8),  // teal
    glm::vec3(0.6, 0.8, 0.2),  // yellow
    glm::vec3(0.8, 0.35, 0.1), // orange
    glm::vec3(0.8, 0.2, 0.8),  // pink
};

glm::vec3 MenuState::getPlayerColor(int playerID)
{
    std::string prefix = getPlayerMenuPrefix(playerID);
    if (getParam("menu.teams"))
    {
        int teamID = getParam(prefix + "teamIDidx");
        assert(teamID >= 0 && teamID < 4);
        // TODO this is a bit naive...
        return teamColorsA[teamID];
    }
    else
    {
        int colorIdx = getParam(prefix + "colorIdx");
        assert(colorIdx >= 0 && colorIdx < playerColors.size());
        return playerColors[colorIdx];
    }
}

std::string MenuState::getPlayerMenuPrefix(int playerID)
{
    std::stringstream ss;
    ss << "menu.player" << playerID << '.';
    return ss.str();
}


static const std::string teamStrs_[] = {"A", "B", "C", "D"};
static const std::string *teamStrs__ = teamStrs_;
static const std::vector<std::string> teamStrs(teamStrs__, teamStrs__ + 4);
static const std::string fighterStrs_[] = {"charlie", "stickman"};
static const std::vector<std::string> fighterStrs(fighterStrs_, fighterStrs_+2); 
static const glm::vec3 widgetSelColor      = glm::vec3(.9, .9, .9);
static const glm::vec3 widgetEnabledColor  = glm::vec3(.3, .3, .3);
static const glm::vec3 widgetDisabledColor = glm::vec3(.1, .1, .1);

void setDefaultState()
{
    if (ParamReader::get()->hasFloat("menu.lives"))
        return;

    ParamReader::get()->setParam("menu.lives", 4);
    ParamReader::get()->setParam("menu.teams", 0);
    ParamReader::get()->setParam("menu.recordStats", 1);
    ParamReader::get()->setParam("menu.handicap", 0);
    ParamReader::get()->setParam("menu.stage",
            StageManager::get()->getStageNames().front());

    // Loop over players 0-3
    for (int i = 0; i < 4; i++)
    {
        std::string prefix = MenuState::getPlayerMenuPrefix(i);

        ParamReader::get()->setParam(prefix + "profileidx", 0);
        ParamReader::get()->setParam(prefix + "fighteridx", 0);
        ParamReader::get()->setParam(prefix + "teamIDidx", i);
        ParamReader::get()->setParam(prefix + "active", 0);
        ParamReader::get()->setParam(prefix + "colorIdx", i);
        ParamReader::get()->setParam(prefix + "handicap", 0);
    }
}

IntegerSelectWidget::IntegerSelectWidget(const std::string &name,
        const std::string &varname,
        int min, int max) :
    name_(name), varname_(varname), min_(min), max_(max), primed_(false)
{
    //nop
}

void IntegerSelectWidget::handleInput(float val)
{
    if (fabs(val) < getParam("menu.deadzone"))
    {
        primed_ = true;
        return;
    }

    if (!primed_)
        return;

    int sign = val > 0 ? 1 : -1;

    int newval = getParam(varname_) + sign;
    newval = glm::clamp(newval, min_, max_);
    setParam(varname_, newval);
    primed_ = false;
}

void IntegerSelectWidget::render(const glm::vec2 &center, const glm::vec2 &size,
        bool selected) const
{
    //if (selected) assert(enabled_);
    const float charsize = size.y;
    glm::mat4 transform;
    glm::vec3 color = selected ? widgetSelColor : widgetEnabledColor;
    color = enabled_ ? color : widgetDisabledColor;

    // Render name
    glm::vec2 strcenter = center - glm::vec2(0.75f * charsize * name_.length()/2.f, 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(strcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            name_);

    // Render value
    int val = getParam(varname_);
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * (FontManager::numDigits(val)/2.f + 1.f), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderNumber(
            transform,
            color,
            val);
}

StringSelectWidget::StringSelectWidget(const std::string &name,
        const std::string &varname,
        const std::vector<std::string> &strings) :
    name_(name), varname_(varname), idxname_(varname), strings_(strings), primed_(false)
{
    idxname_.append("idx");
    setParam(varname_, strings_[getParam(idxname_)]);
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

    int newval = getParam(idxname_) + sign;
    newval = glm::clamp(newval, 0, (int)strings_.size() - 1);
    setParam(idxname_, newval);
    setParam(varname_, strings_[newval]);
    primed_ = false;
}

void StringSelectWidget::render(const glm::vec2 &center, const glm::vec2 &size,
        bool selected) const
{
    if (selected) assert(enabled_);
    const float charsize = size.y;
    glm::mat4 transform;
    glm::vec3 color = selected ? widgetSelColor : widgetEnabledColor;
    color = enabled_ ? color : widgetDisabledColor;

    // Render name
    glm::vec2 strcenter = center - glm::vec2(0.75f * charsize * name_.length()/2.f, 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(strcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            name_);

    // Render value
    const std::string &val = strParam(varname_);
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * (val.length()/2.f + 1.f), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            val);
}

BoolSelectWidget::BoolSelectWidget(const std::string &name,
        const std::string &varname) :
    name_(name), varname_(varname),
    primed_(false)
{
}

void BoolSelectWidget::handleInput(float val)
{
    if (fabs(val) < getParam("menu.deadzone"))
    {
        primed_ = true;
        return;
    }

    if (!primed_)
        return;

    int newval = val > 0 ? 1 : 0;
    setParam(varname_, newval);
    primed_ = false;
}

void BoolSelectWidget::render(const glm::vec2 &center, const glm::vec2 &size,
        bool selected) const
{
    if (selected) assert(enabled_);
    const float charsize = size.y;
    glm::mat4 transform;
    glm::vec3 color = selected ? widgetSelColor : widgetEnabledColor;
    color = enabled_ ? color : widgetDisabledColor;

    // Render name
    glm::vec2 strcenter = center - glm::vec2(0.75f * charsize * name_.length()/2.f, 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(strcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            name_);

    // Render value
    const std::string &val = getParam(varname_) ? "ON" : "OFF";
    glm::vec2 valcenter = center + glm::vec2(0.75f * charsize * (val.length()/2.f + 1.f), 0.f);
    transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(valcenter, 0.f)),
            glm::vec3(charsize, -charsize, 1.f));
    FontManager::get()->renderString(
            transform,
            color,
            val);
}

PlayerWidget::PlayerWidget(int playerID) :
    playerID_(playerID),
    usernameWidget_(NULL),
    teamIDWidget_(NULL),
    widgetIdx_(0),
    widgetChangePrimed_(false)
{
    assert(playerID < 4);
    prefix_ = MenuState::getPlayerMenuPrefix(playerID);

    usernameWidget_ = new StringSelectWidget("Profile ",
            prefix_ + "profile",
            StatsManager::get()->getUsernames());
    fighterWidget_ = new StringSelectWidget("Fighter ",
            prefix_ + "fighter",
            fighterStrs);
    teamIDWidget_ = new StringSelectWidget("Team ",
            prefix_ + "teamID",
            teamStrs);
    handicapWidget_ = new IntegerSelectWidget("Handicap ",
            prefix_ + "handicap",
            0, 9);

    widgets_.push_back(usernameWidget_);
    widgets_.push_back(fighterWidget_);
    widgets_.push_back(teamIDWidget_);
    widgets_.push_back(handicapWidget_);
}

PlayerWidget::~PlayerWidget()
{
    delete usernameWidget_;
    delete fighterWidget_;
    delete teamIDWidget_;
    delete handicapWidget_;
}

bool PlayerWidget::isActive() const
{
    return getParam(prefix_ + "active");
}

void PlayerWidget::processInput(Controller *controller, float dt)
{
    // A to join
    if (controller->getState().pressa)
    {
        setParam(prefix_ + "active", 1);
        return;
    }

    if (!isActive())
        return;

    // B to leave
    if (controller->getState().pressb)
    {
        setParam(prefix_ + "active", 0);
        return;
    }

    // X to change color
    if (controller->getState().pressx)
    {
        int newval = ((int)getParam(prefix_ + "colorIdx") + 1) % playerColors.size();
        setParam(prefix_ + "colorIdx", newval);
    }

    // Left stick
    float xval = controller->getState().joyx;
    float yval = controller->getState().joyy;
    float dir = 1;

    if (fabs(yval) > getParam("menu.thresh") && widgetChangePrimed_)
    {
        dir = -glm::sign(yval);
        widgetIdx_ += dir;
        widgetIdx_ = std::max(std::min(widgetIdx_, (int)widgets_.size() - 1), 0);

        widgetChangePrimed_ = false;
    }
    else if (fabs(yval) <= getParam("menu.thresh"))
        widgetChangePrimed_ = true;

    // ----------------
    // Look for an enabled widget in the sign direction
    for (; widgetIdx_ > 0 && widgetIdx_ < widgets_.size()
            && !widgets_[widgetIdx_]->enabled();
            widgetIdx_ += dir)
    {
        if (widgets_[widgetIdx_]->enabled())
            break;
    }
    widgetIdx_ = std::max(std::min(widgetIdx_, (int)widgets_.size() - 1), 0);
    // Look for an enabled widget in the other direction
    dir = -dir;
    for (; widgetIdx_ > 0 && widgetIdx_ < widgets_.size()
            && !widgets_[widgetIdx_]->enabled();
            widgetIdx_ += dir)
    {
        if (widgets_[widgetIdx_]->enabled())
            break;
    }
    widgetIdx_ = std::max(std::min(widgetIdx_, (int)widgets_.size() - 1), 0);
    assert(widgets_[widgetIdx_]->enabled());
    // ----------------

    widgets_[widgetIdx_]->handleInput(xval);
}

void PlayerWidget::render(const glm::vec2 &center, const glm::vec2 &size, float dt)
{
    glm::mat4 trans;
    const glm::vec3 color = MenuState::getPlayerColor(playerID_);

    // Update widget enabled status
    teamIDWidget_->setEnabled(getParam("menu.teams"));
    handicapWidget_->setEnabled(getParam("menu.handicap"));

    const bool active = getParam(prefix_ + "active");
    if (!active)
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
                glm::vec3(25.f, -25.f, 0.f));
        renderFighter(trans, color);

        glm::vec2 widgetStart = center - glm::vec2(size.x *0.1f, size.y * 0.25f);
        glm::vec2 widgetSize = glm::vec2(0.33f, 0.1f) * size;
        glm::vec2 widgetDelta = glm::vec2(0.f, 1.5f*widgetSize.y);
        for (size_t i = 0; i < widgets_.size(); i++)
        {
            widgets_[i]->render(widgetStart + (float)i * widgetDelta,
                    widgetSize, widgetIdx_ == i);
        }
    }
}

void PlayerWidget::renderFighter(const glm::mat4 &transform, const glm::vec3 &color)
{
    std::string fighter = strParam(prefix_ + "fighter");
    // Scale for each bixel being 5 game units
    if (fighter == "charlie")
    {
        FrameManager::get()->renderFighter(transform, glm::vec4(color, 0.15f), fighter);
    }
    else if (fighter == "stickman")
    {
        FrameManager::get()->renderFighter(transform, glm::vec4(color, 0.55f), fighter);
    }
    else
        assert(false && "Unknown fighter to render");
}

MenuState::MenuState() :
    topMenuController_(-1),
    topWidgetIdx_(0),
    topWidgetChangePrimed_(false)
{
    logger_ = Logger::getLogger("MenuState");

    // Set up params (if necessary)
    setDefaultState();

    // Start the menu soundtrack
    AudioManager::get()->setSoundtrack("sfx/02 Escape Velocity (loop).ogg");
    AudioManager::get()->startSoundtrack();

    std::vector<std::string> boolstrs;
    boolstrs.push_back("no"); boolstrs.push_back("yes");

    for (int i = 0; i < 4; i++)
        widgets_.push_back(new PlayerWidget(i));

    topWidgets_.push_back(new IntegerSelectWidget("Lives", "menu.lives", 1, 99));
    topWidgets_.push_back(new BoolSelectWidget("Teams", "menu.teams"));
    topWidgets_.push_back(new BoolSelectWidget("Stats", "menu.recordStats"));
    topWidgets_.push_back(new BoolSelectWidget("Handicap", "menu.handicap"));

    getProjectionMatrixStack().clear();
    getViewMatrixStack().clear();
    getProjectionMatrixStack().current() = glm::ortho(0.f, 1920.f, 1080.f, 0.f, -1.f, 1.f);
}

MenuState::~MenuState()
{
    for (size_t i = 0; i < widgets_.size(); i++)
        delete widgets_[i];
    for (size_t i = 0; i < topWidgets_.size(); i++)
        delete topWidgets_[i];
}

void MenuState::preFrame()
{
    checkForJoysticks(getParam("maxPlayers"));
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

    float xchange = 1920.f / (topWidgets_.size() + 1);
    for (size_t i = 0; i < topWidgets_.size(); i++)
    {
        glm::vec2 center(xchange * (i + 1), yoffset/2);
        glm::vec2 size(1920.f, yoffset/2 * 0.75);

        topWidgets_[i]->render(center, size, topMenuController_ != -1 && i == topWidgetIdx_);
    }
}

void MenuState::postFrame()
{
    // nop
}

GameState* MenuState::processInput(const std::vector<Controller*> &controllers, float dt)
{
    bool shouldStart = false;
    int startingPlayer = -1;
    for (size_t i = 0; i < widgets_.size() && i < controllers.size(); i++)
    {
        // Check for start
        shouldStart |= controllers[i]->getState().pressstart;
        if (shouldStart && startingPlayer == -1)
            startingPlayer = i;

        // Check for y button to transition to top menu
        if (widgets_[i]->isActive() && controllers[i]->getState().pressy
                && topMenuController_ == -1)
        {
            topMenuController_ = i;
            topWidgetIdx_ = 0;
            continue;
        }

        if (i == topMenuController_)
        {
            handleTopMenu(controllers[i]);
            continue;
        }

        // Finally, if no top menu, process input normally
        widgets_[i]->processInput(controllers[i], dt);
    }


    // start when someone asks to and either at least two teams, or it's debug mode
    // TODO make this require at least 2 teams
    if (shouldStart /* && (atleast2teams) */)
    {
        logger_->debug() << "Starting a new game by from Player " << startingPlayer+1 << "'s request\n";
        return new LevelSelectState(startingPlayer);
    }

    // No transition
    return NULL;
}

void MenuState::handleTopMenu(Controller *controller)
{
    float xval = controller->getState().joyx;
    float yval = controller->getState().joyy;

    // First deal with y button (to leave)
    if (controller->getState().pressy)
    {
        topMenuController_ = -1;
        return;
    }

    // Now deal with changing widgets
    if (fabs(xval) > getParam("menu.thresh") && topWidgetChangePrimed_)
    {
        int sign = glm::sign(xval);
        topWidgetIdx_ = std::max(
                std::min((int)topWidgetIdx_ + sign, (int)topWidgets_.size() - 1),
                0);
        topWidgetChangePrimed_ = false;
    }
    else if (fabs(xval) < getParam("menu.thresh"))
    {
        topWidgetChangePrimed_ = true;
    }

    // Now deal with change value on widget
    topWidgets_[topWidgetIdx_]->handleInput(yval);
}

