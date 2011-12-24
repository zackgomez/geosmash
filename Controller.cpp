#include "Controller.h"
#include <SDL/SDL.h>
#include <iostream>
#include "Fighter.h"

static const float MAX_JOYSTICK_VALUE = 32767.0f;

bool togglepause(int);

Controller::Controller(const Fighter *f, SDL_Joystick *joystick) :
    fighter_(f), joystick_(joystick)
{
    // Clear the state
    memset(&state_, 0, sizeof(controller_state));
}

Controller::~Controller()
{
    // Nothing
}

controller_state Controller::nextState()
{
    return state_;
}

controller_state Controller::lastState()
{
    return state_;
}

void Controller::update(float dt)
{
    // No presses
    state_.pressa = false;
    state_.pressb = false;
    state_.pressc = false;
    state_.pressjump = false;
    state_.pressstart = false;
    state_.pressrb = false;
    state_.presslb = false;

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
    state_.rtrigger = -SDL_JoystickGetAxis(joystick_, 4) / MAX_JOYSTICK_VALUE;
    // left trigger id 5
    state_.ltrigger = -SDL_JoystickGetAxis(joystick_, 5) / MAX_JOYSTICK_VALUE;

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
    state_.pressc = newstate && !state_.buttonc;
    state_.buttonc = newstate;
    // Y button, id 3 AKA jump button
    newstate = SDL_JoystickGetButton(joystick_, 3);
    state_.pressjump = newstate && !state_.jumpbutton;
    state_.jumpbutton = newstate;

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
