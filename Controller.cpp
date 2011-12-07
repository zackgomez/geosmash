#include "Controller.h"
#include <SDL/SDL.h>
#include <iostream>
#include "Fighter.h"

static const float MAX_JOYSTICK_VALUE = 32767.0f;

bool togglepause(int);

Controller::Controller(const Fighter *f, int joyWhich) :
    fighter_(f), which_(joyWhich)
{
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
    state_.pressa = false;
    state_.pressb = false;
    state_.pressc = false;
    state_.pressjump = false;
    state_.pressstart = false;
    state_.pressrb = false;
    state_.presslb = false;
    state_.joyxv = 0;
    state_.joyyv = 0;
}

void Controller::processEvent(SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_JOYAXISMOTION:
        if (event.jaxis.which != which_)
            return;
        // left joy stick X axis
        if (event.jaxis.axis == 0)
        {
            float newPos = event.jaxis.value / MAX_JOYSTICK_VALUE;
            state_.joyxv += (newPos - state_.joyx);
            state_.joyx = newPos;
        }
        // left joy stick Y axis
        else if (event.jaxis.axis == 1)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            state_.joyyv += (newPos - state_.joyy);
            state_.joyy = newPos;
        }
        // Left trigger
        else if (event.jaxis.axis == 5)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            state_.ltrigger = newPos;
        }
        // Right trigger
        else if (event.jaxis.axis == 4)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            state_.rtrigger = newPos;
        }
        // DPAD L/R
        else if (event.jaxis.axis == 6)
        {
            if (event.jaxis.value > 0)
                state_.dpadr = true;
            else if (event.jaxis.value < 0)
                state_.dpadl = true;
            else
                state_.dpadl = state_.dpadr = false;
        }
        // DPAD U/D
        else if (event.jaxis.axis == 7)
        {
            if (event.jaxis.value > 0)
                state_.dpadd = true;
            else if (event.jaxis.value < 0)
                state_.dpadu = true;
            else
                state_.dpadd = state_.dpadu = false;
        }
        else
        {
            std::cout << "Axis event: " << (int) event.jaxis.axis << '\n';
        }
        break;

    case SDL_JOYBUTTONDOWN:
        if (event.jbutton.which != which_)
            return;
        if (event.jbutton.button == 0)
        {
            state_.pressa = true;
            state_.buttona = true;
        }
        else if (event.jbutton.button == 1)
        {
            state_.pressb = true;
            state_.buttonb = true;
        }
        else if (event.jbutton.button == 3)
        {
            state_.pressjump = true;
            state_.jumpbutton = true;
        }
        else if (event.jbutton.button == 2)
        {
            state_.pressc = true;
            state_.buttonc = true;
        }
        else if (event.jbutton.button == 4)
        {
            state_.presslb = true;
            state_.lbumper = true;
        }
        else if (event.jbutton.button == 5)
        {
            state_.pressrb = true;
            state_.rbumper = true;
        }
        else if (event.jbutton.button == 7)
        {
            // Control pause when character is alive
            if (fighter_->isAlive())
                togglepause(event.jbutton.which);
            state_.pressstart = true;
            state_.buttonstart = true;
        }
        break;

    case SDL_JOYBUTTONUP:
        if (event.jbutton.which != which_)
            return;
        if (event.jbutton.button == 0)
        {
            state_.buttona = false;
            state_.pressa = false;
        }
        else if (event.jbutton.button == 1)
        {
            state_.buttonb = false;
            state_.pressb = false;
        }
        else if (event.jbutton.button == 3)
        {
            state_.jumpbutton = false;
            state_.pressjump = false;
        }
        else if (event.jbutton.button == 2)
        {
            state_.buttonc = false;
            state_.pressc = false;
        }
        else if (event.jbutton.button == 7)
        {
            state_.buttonstart = false;
            state_.pressstart = false;
        }
        else if (event.jbutton.button == 4)
        {
            state_.presslb = false;
            state_.lbumper = false;
        }
        else if (event.jbutton.button == 5)
        {
            state_.pressrb = false;
            state_.rbumper = false;
        }
        else
        {
            std::cout << "Got button number : " << (int) event.jbutton.button << '\n';
        }
        break;

    default:
        std::cout << "WARNING: Unknown event in Controller::processEvent.\n";
    }
}
