#pragma once
#include "GameState.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Controller.h"

class StringSelectWidget
{
public:
    StringSelectWidget(const std::string &name,
            const std::vector<std::string> &strings, int defval);
    
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
    PlayerWidget(const glm::vec3 &col, const glm::vec3 &teamcol, int playerID);
    ~PlayerWidget();

    bool isActive() const;
    bool wantsStart() const;

    void processInput(SDL_Joystick *joystick, float dt);
    void render(const glm::vec2 &center, const glm::vec2 &size, float dt);

    void getController(int teamID, int lives, SDL_Joystick *stick,
        Fighter **outfighter, Controller **outcntrl) const;

private:
    int playerID_;

    bool active_;
    bool holdStart_, pressStart_;

    glm::vec3 col_, teamcol_;

    StringSelectWidget usernameWidget_;
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
    std::vector<PlayerWidget> widgets_;

    GameState* newGame(const std::vector<SDL_Joystick*>&);
};

