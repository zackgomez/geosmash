#pragma once
#include "GameState.h"
#include <string>
#include <glm/glm.hpp>
#include "Controller.h"

class MenuWidget
{
public:
    virtual ~MenuWidget() {}

    virtual int value() const = 0;

    virtual void handleInput(float val) = 0;
    virtual void render(const glm::vec2 &center, bool selected) const = 0;
};


class NumberSelectWidget : public MenuWidget
{
public:
    NumberSelectWidget(const std::string &name, int min, int max, int defval = -1);

    virtual int value() const;

    virtual void handleInput(float val);
    virtual void render(const glm::vec2 &center, bool selected) const;

private:
    std::string name_;
    int min_, max_;
    int value_;

    bool primed_;
};

class StringSelectWidget : public MenuWidget
{
public:
    StringSelectWidget(const std::string &name, const std::vector<std::string> &strs,
            int defval = 0);

    virtual int value() const;

    virtual void handleInput(float val);
    virtual void render(const glm::vec2 &transform, bool selected) const;

private:
    std::string name_;
    const std::vector<std::string> values_;
    int idx_;

    bool primed_;
};


class MenuState : public GameState
{
public:
    MenuState();
    ~MenuState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*>&, float);
    virtual void update(float);
    virtual void render(float);
    
private:
    // Player one can only change the number of players if they've
    // recently gone back to the dead zone with the controller stick
    bool rowChangePrimed_;
    // True when start will send to next menu
    bool startPrimed_;
    // index into widget array
    int widgetInd_;

    std::vector<MenuWidget*> widgets;

    GameState* newGame(const std::vector<SDL_Joystick*>&stix);

};

