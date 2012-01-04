#include "MenuState.h"
#include "InGameState.h"


MenuState::MenuState()
{
    playersMenuItems_.push_back({1, "one"});

}

GameState* processInput(const std::vector<SDL_Joystick*>&stix)
{
    assert(stix.size() > 0);
    // Only player one may move through the menus
    SDL_Joystick*p1 = stix[0];

    // If p1 is pressing start, create a new GameState 
    GameState *gs = new InGameState();
}
