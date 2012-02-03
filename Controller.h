#pragma once
#include <SDL/SDL.h>
#include "Logger.h"

class Fighter;

struct controller_state
{
    // The positions [-1, 1] of the main analog stick
    float joyx, joyy;
    // The velocities of the main analog stick over some time period
    float joyxv, joyyv;
    // nonzero if the button is pressed
    bool buttona, buttonb, buttonx, buttony;
	bool buttonstart, buttonback;
    // nonzero if the button was pressed this frame
    bool pressa, pressb, pressx, pressy;
	bool pressstart, pressback;

    float rtrigger, ltrigger;
    bool lbumper, rbumper;
    bool presslb, pressrb;

    // Dpad directions, true if they're press currently
    bool dpadl, dpadr, dpadu, dpadd;

    void clear();
};


class Controller
{
public:
    Controller(int controllerID);
    ~Controller();

    // Does per frame controller updates...
    void update(float dt);
    // Gets the next state
    controller_state getState() const;

	// Clears all the press variables, useful for transitions
	void clearPresses();

    // Gets a unique identifier for this controller, roughly the player #
    // of the controller
    int getControllerID() const;

private:
    int controllerID_;
    SDL_Joystick *joystick_;

    controller_state state_;

    int ltrigAxis_, rtrigAxis_;
	int dpadnsAxis_, dpadweAxis_;

    LoggerPtr logger_;
};

