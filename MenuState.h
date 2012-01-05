#pragma once
#include "GameState.h"
#include <string>
#include <glm/glm.hpp>
#include "Controller.h"


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

struct NumPlayersEntry
{
    NumPlayersEntry(int v, std::string n) : val(v), name(n) {}
    int val;
    std::string name;
};

class MenuState : public GameState
{
public:
    MenuState();

    virtual GameState* processInput(const std::vector<SDL_Joystick*>&, float);
    virtual void update(float);
    virtual void render(float);
    
private:
    GameState* newGame(const std::vector<SDL_Joystick*>&stix);
    std::vector<NumPlayersEntry> playerCountItems_;
    std::vector<NumPlayersEntry>::iterator playerCountCurr_;
    std::vector<SDL_Joystick*> joysticks;
    std::vector<Controller*> controllers;
    std::vector<Fighter*> fighters;
};

