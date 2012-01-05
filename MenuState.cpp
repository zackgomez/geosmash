#include "MenuState.h"
#include "InGameState.h"
#include "GameState.h"
#include "Fighter.h"
#include <cassert>

#define FIGHTER_SPAWN_Y 50.0f
#define JOYSTICK_START 7
MenuState::MenuState()
{
    playerCountItems_.push_back(NumPlayersEntry(1, "one"));
    playerCountItems_.push_back(NumPlayersEntry(2, "two"));
    playerCountItems_.push_back(NumPlayersEntry(3, "three"));
    playerCountItems_.push_back(NumPlayersEntry(4, "four"));

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
    return 0;
}


GameState* MenuState::newGame(const std::vector<SDL_Joystick*>&stix)
{

    int np = playerCountCurr_->val;
    for (int i = 0; i < np; i++)
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

