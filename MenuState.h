#pragma once
#include "GameState.h"
#include <string>
#include <glm/glm.hpp>
#include "Controller.h"


class MenuState : public GameState
{
public:
    MenuState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*>&, float);
    virtual void update(float);
    virtual void render(float);
    
private:
    int nplayers_;
    // Player one can only change the number of players if they've
    // recently gone back to the dead zone with the controller stick
    bool canIncrement_;
    bool canChangeRow_;
    // True when start will send to next menu
    bool startPrimed_;
    bool teams_;
    int currentRow_;
    int totalRows_;
    GameState* newGame(const std::vector<SDL_Joystick*>&stix);
    std::vector<Controller*> controllers;
    std::vector<Fighter*> fighters;

    void checkNPlayers(float xval);
    void checkRow(float yval);
};

