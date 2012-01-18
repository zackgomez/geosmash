#pragma once
#include "GameState.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Controller.h"

class MenuWidget
{
public:
    virtual ~MenuWidget() { }

    virtual int value() const = 0;
    virtual void handleInput(float val) = 0;
    virtual void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const = 0;

private:
};

class NumberSelectWidget : public MenuWidget
{
public:
    NumberSelectWidget(const std::string &name,
            int min, int max, int defval = 0);
    
    int value() const;
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

private:
    std::string name_;
    int min_, max_;
    int value_;
    bool primed_;
};


class StringSelectWidget : public MenuWidget
{
public:
    StringSelectWidget(const std::string &name,
            const std::vector<std::string> &strings, int defval = -1);
    
    int value() const;
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

    std::string strValue() const;

private:
    std::string name_;
    std::vector<std::string> values_;
    int idx_;
    bool primed_;
};

class PlayerWidget
{
public:
    explicit PlayerWidget(int playerID, const bool *teams);
    ~PlayerWidget();

    bool isActive() const;
    bool wantsStart() const;

    void processInput(SDL_Joystick *joystick, float dt);
    void render(const glm::vec2 &center, const glm::vec2 &size, float dt);

    void getController(int lives, SDL_Joystick *stick, int colorScheme,
        Fighter **outfighter, Controller **outcntrl) const;
    int getTeamID() const;
    int getProfileID() const;

private:
    int playerID_;

    bool active_;
    bool startPrimed_;
    bool wantsStart_;

    StringSelectWidget *usernameWidget_;
    StringSelectWidget *teamIDWidget_;

    int widgetIdx_;
    std::vector<MenuWidget*> widgets_;
    bool widgetChangePrimed_;

    int colorIdx_;
    bool colorChangePrimed_;

    const bool *teams_;

    glm::vec3 getColor(int colorScheme = 0) const;
};

class MenuState : public GameState
{
public:
    MenuState();
    ~MenuState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*>&, float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    
private:
    std::vector<PlayerWidget*> widgets_;
    std::vector<MenuWidget*> topWidgets_;

    bool teams_;
    int topMenuController_;
    int topWidgetIdx_;
    bool topWidgetPrimed_;
    bool topMenuExitPrimed_;
    bool topMenuPrimed_[4];

    GameState* newGame(const std::vector<SDL_Joystick*>&);
    void handleTopMenu(SDL_Joystick *stick);
};

