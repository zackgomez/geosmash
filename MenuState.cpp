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
#include "ParamReader.h"

#define FIGHTER_SPAWN_Y 50.0f
#define JOYSTICK_START 7
#define MAX_JOYSTICK_VALUE 32767.0f

MenuState::MenuState()
{
    nplayers_ = 4;
}


void MenuState::update(float)
{
    //nop
}

void MenuState::render(float dt)
{
    // Start with a blank slate
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    setProjectionMatrix(glm::ortho(0.f, 1920.f, 0.f, 1080.f, -1.f, 1.f));
    setViewMatrix(glm::mat4(1.f));

    // Draw the players, highlight the winner
    glm::mat4 transform = 
        glm::scale(
            glm::translate(
                glm::mat4(1.f), 
                glm::vec3(1920.f/10 + 1920.f/5, 1080.f - 1080.f/3/2, 0.1f)), 
            glm::vec3(1.f, 1.f, 1.f));

    // Kills
    transform = glm::scale(
            glm::translate(
                glm::mat4(1.f), 
                glm::vec3(1920.f/10, 1080.f - 1080.f/3 - 1080.f/3/2, 0.1f)), 
            glm::vec3(1.f, 1.f, 1.f));
    //transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    for (unsigned i = 1; i <= 4; i++)
    {
        glm::vec3 theColor = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 selectedColor = glm::vec3(0.5f, 0.5f, 1.0f);
        FontManager::get()->renderNumber(
                glm::scale(transform, 
                    glm::vec3(100.f, 100.f, 1.f)), 
                i == nplayers_ ? theColor : selectedColor,
                i);
        transform = glm::translate(transform, glm::vec3(1920.f/5, 0.f, 0.f));
    }
    SDL_GL_SwapBuffers();
}

GameState* MenuState::processInput(const std::vector<SDL_Joystick*>&stix, float)
{
    assert(stix.size() > 0);
    // Only player one may move through the menus
    SDL_Joystick* p1 = stix[0];

    if (SDL_JoystickGetButton(p1, JOYSTICK_START))
    {
        return newGame(stix);
    }
    // -1 < xval < 1
    float xval = SDL_JoystickGetAxis(p1, 0) / MAX_JOYSTICK_VALUE; 
    if (fabs(xval) < getParam("menu.deadzone"))
    {
        canIncrement_ = true;
    }
    if (xval < -getParam("menu.thresh") && canIncrement_)
    {
        canIncrement_ = false;
        if (nplayers_ > 1)
        {
            nplayers_--;
        }
    }
    if (xval > getParam("menu.thresh") && canIncrement_)
    {
        canIncrement_ = false;
        if (nplayers_ < 4)
        {
            nplayers_++;
        }
    }
    
    float yval = SDL_JoystickGetAxis(p1, 1) / MAX_JOYSTICK_VALUE; 
    return 0;
}


GameState* MenuState::newGame(const std::vector<SDL_Joystick*>&stix)
{

    for (int i = 0; i < nplayers_; i++)
    {
        // create a new fighter

        Fighter *f = new Fighter(0-225.0f+i*150,
                FIGHTER_SPAWN_Y, 
                playerColors[i],
                i);

        // create a controller
        // push the controller onto the list
        controllers.push_back(new Controller(f, stix[i]));
        fighters.push_back(f);
    }
        

    // If p1 is pressing start, create a new GameState 
    GameState *gs = new InGameState(controllers, fighters);
    return gs;
}

