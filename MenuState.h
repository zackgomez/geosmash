#pragma once
#include "GameState.h"
#include <string>
#include <glm/glm.hpp>
#include "Controller.h"

class MenuWidget
{
public:
    MenuWidget(const std::string &name, int min, int max);

    int value() const;

    void handleInput(float val);
    void render(const glm::mat4 &transform) const;

private:
    std::string name_;
    int min_, max_;
    int value_;

    bool primed_;
};


class MenuState : public GameState
{
public:
    MenuState();

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

