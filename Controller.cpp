#include "Controller.h"
#include <SDL/SDL.h>
#include <iostream>
#include "Fighter.h"

void controller_state::clear()
{
    memset(this, 0, sizeof(controller_state));
}

static const float MAX_JOYSTICK_VALUE = 32767.0f;

Controller::Controller(int controllerID) :
    controllerID_(controllerID),
    joystick_(NULL)
{
    // Clear the state
    state_.clear();
    // Get the current state
    update(0);

    std::string joystickName = SDL_JoystickName(controllerID);
    std::cout << "Opening joystick: " << joystickName << '\n';
	joystick_ = SDL_JoystickOpen(controllerID);

    // TODO some config based on joystickName
    // Xbox Gamepad (userspace driver) [#i where i>1]
    // Xbox 360 Wireless Receiver (need to change some axis here)
}

Controller::~Controller()
{
	SDL_JoystickClose(joystick_);
}

controller_state Controller::getState() const
{
    return state_;
}

void Controller::update(float dt)
{
    // No presses
	clearPresses();

    // temporary value for computing velocities
    float newval;

    // Update axis first
    // left stick X
    newval = SDL_JoystickGetAxis(joystick_, 0) / MAX_JOYSTICK_VALUE;
    state_.joyxv = newval - state_.joyx;
    state_.joyx = newval;
    // left stick Y
    newval = -SDL_JoystickGetAxis(joystick_, 1) / MAX_JOYSTICK_VALUE;
    state_.joyyv = newval - state_.joyy;
    state_.joyy = newval;

    // Triggers
    // right trigger id 4
    // NOTE: this value differs depending on the controller driver.  For xboxdrv it is 4.
    // for xpad it is 2, for windows it is ????
    state_.rtrigger = -SDL_JoystickGetAxis(joystick_, 4) / MAX_JOYSTICK_VALUE;
    // left trigger id 5
    state_.ltrigger = -SDL_JoystickGetAxis(joystick_, 5) / MAX_JOYSTICK_VALUE;

    std::cout << "Triggers (L,R): " << state_.ltrigger << ' ' << state_.rtrigger << '\n';

    // Buttons
    bool newstate;
    // A button, id 0
    newstate = SDL_JoystickGetButton(joystick_, 0);
    state_.pressa = newstate && !state_.buttona;
    state_.buttona = newstate;
    // B button, id 1
    newstate = SDL_JoystickGetButton(joystick_, 1);
    state_.pressb = newstate && !state_.buttonb;
    state_.buttonb = newstate;
    // X button, id 2
    newstate = SDL_JoystickGetButton(joystick_, 2);
    state_.pressx = newstate && !state_.buttonx;
    state_.buttonx = newstate;
    // Y button, id 3 AKA jump button
    newstate = SDL_JoystickGetButton(joystick_, 3);
    state_.pressy = newstate && !state_.buttony;;
    state_.buttony = newstate;
    // Start button, id 7
    newstate = SDL_JoystickGetButton(joystick_, 7);
    state_.pressstart = newstate && !state_.buttonstart;
    state_.buttonstart = newstate;
    // Back button, id 6
    newstate = SDL_JoystickGetButton(joystick_, 6);
    state_.pressback = newstate && !state_.buttonback;
    state_.buttonback = newstate;

    // Bumpers
    // left bumper, id 4
    newstate = SDL_JoystickGetButton(joystick_, 4);
    state_.presslb = newstate && !state_.lbumper;
    state_.lbumper = newstate;
    // right bumper, id 5
    newstate = SDL_JoystickGetButton(joystick_, 5);
    state_.pressrb = newstate && !state_.rbumper;
    state_.rbumper = newstate;

    // DPad
    state_.dpadr = SDL_JoystickGetAxis(joystick_, 6) > 0;
    state_.dpadl = SDL_JoystickGetAxis(joystick_, 6) < 0;
    state_.dpadu = SDL_JoystickGetAxis(joystick_, 7) < 0;
    state_.dpadd = SDL_JoystickGetAxis(joystick_, 7) > 0;
}
void Controller::clearPresses()
{
    state_.pressa = false;
    state_.pressb = false;
    state_.pressx = false;
    state_.pressy = false;
    state_.pressstart = false;
    state_.pressrb = false;
    state_.presslb = false;
}

int Controller::getControllerID() const
{
    return controllerID_;
}
