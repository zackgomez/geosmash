#include "Controller.h"
#include <SDL/SDL.h>
#include <iostream>
#include "Fighter.h"

void controller_state::clear()
{
    memset(this, 0, sizeof(controller_state));
}

static const float MAX_JOYSTICK_VALUE = 32767.0f;

std::vector<KeyboardController*> KeyboardController::keyboardControllers_;

Controller::Controller(int controllerID, bool isKeyboard) :
    controllerID_(controllerID),
	isKeyboard_(isKeyboard),
    joystick_(NULL)
{
    logger_ = Logger::getLogger("Controller");
    // Clear the state
    state_.clear();
    // Get the current state
    update(0);
	if (isKeyboard_) 
	{
		// No further setup is required
		logger_->info() << "Opening joystick: Keyboard Controller\n";
		return;
	}
	assert(controllerID >= 0 && "attempted to ask SDL For a negative joystick!");
    std::string joystickName = SDL_JoystickName(controllerID);
    logger_->info() << "Opening joystick: " << joystickName << '\n';
	joystick_ = SDL_JoystickOpen(controllerID);

    // Switch trigger axis based on driver
    // Xbox Gamepad (userspace driver) [#i where i>1] (xboxdrv) (hydralisk only)
    ltrigAxis_ = 5;
    rtrigAxis_ = 4;
    dpadweAxis_ = 6;
    dpadnsAxis_ = 7;
    // Xbox 360 Wireless Receiver (need to change some axis here)
    if (joystickName == "Xbox 360 Wireless Receiver")
    {
        ltrigAxis_ = 5;
        rtrigAxis_ = 2;
    }
	// windows, set only rtrig axis, as both l and r trig are on this axis,
	// just the +/-
	if (joystickName == "Controller (Xbox 360 Wireless Receiver for Windows)")
	{
		rtrigAxis_ = 2;
	}

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
    state_.rtrigger = -SDL_JoystickGetAxis(joystick_, rtrigAxis_) / MAX_JOYSTICK_VALUE;
    // left trigger id 5
    state_.ltrigger = -SDL_JoystickGetAxis(joystick_, ltrigAxis_) / MAX_JOYSTICK_VALUE;
#ifdef _MSC_VER
	state_.ltrigger = SDL_JoystickGetAxis(joystick_, rtrigAxis_) / MAX_JOYSTICK_VALUE;
#endif


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
#ifdef _MSC_VER
    state_.dpadr = SDL_JoystickGetHat(joystick_, 0) == SDL_HAT_RIGHT;
    state_.dpadl = SDL_JoystickGetHat(joystick_, 0) == SDL_HAT_LEFT;
    state_.dpadu = SDL_JoystickGetHat(joystick_, 0) == SDL_HAT_UP;
    state_.dpadd = SDL_JoystickGetHat(joystick_, 0) == SDL_HAT_DOWN;
#else
    state_.dpadr = SDL_JoystickGetAxis(joystick_, dpadweAxis_) > 0;
    state_.dpadl = SDL_JoystickGetAxis(joystick_, dpadweAxis_) < 0;
    state_.dpadu = SDL_JoystickGetAxis(joystick_, dpadnsAxis_) < 0;
    state_.dpadd = SDL_JoystickGetAxis(joystick_, dpadnsAxis_) > 0;
#endif
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

///////////////////////////////////
// KeyboardController

KeyboardController::KeyboardController(int ctrlID) : 
	Controller(ctrlID, true), shouldAcceptInput_(true)
{  
	for (size_t i = 0; i < keyboardControllers_.size(); i++)
	{
		keyboardControllers_[i]->shouldAcceptInput_ = false;
	}
	keyboardControllers_.push_back(this);
}

void KeyboardController::keyboardClear() 
{
	state_.joyx = 0;
	state_.joyy = 0;
	state_.joyxv = 0;
	state_.joyyv = 0;
}

void KeyboardController::update(float dt)
{
	if (!shouldAcceptInput_) 
	{
		// In this case, the developer needed this controller to create an AI
		// (or some similar scenario) and we're ignoring all input.
		return;
	}

	// Just clear the joystick position. This allows to save parts of 
	// the previous state (in particular, button<whatever> values).
	// We need to set state_.pressa, etc. correctly
	keyboardClear();

    // Ask SDL for keyboard state we care about 
    Uint8 *keystate = SDL_GetKeyState(NULL);

	// A BUTTON
    if ( keystate[SDLK_a] )
    {
        state_.pressa = !state_.buttona;
		state_.buttona = true;
    }
	else
	{
		state_.pressa = state_.buttona = false;
	}


	// B BUTTON
    if ( keystate[SDLK_b] )
    {
        state_.pressb = !state_.buttonb;
		state_.buttonb = true;
    }
	else 
	{
		state_.pressb = state_.buttonb = false;
	}

	// Y BUTTON
    if ( keystate[SDLK_y] )
    {
		state_.pressy = !state_.buttony;
		state_.buttony = true;
    }
	else 
	{
		state_.pressy = state_.buttony = false;
	}
	
	// X BUTTON
    if ( keystate[SDLK_x] )
    {
		state_.pressx = !state_.buttonx;
		state_.buttonx = true;
    }
	else 
	{
		state_.pressx = state_.buttonx = false;
	}
	
	// RETURN
    if (keystate[SDLK_RETURN]) 
    {
		state_.pressstart = !state_.buttonstart;
        state_.buttonstart = true;
    }
	else 
	{
		state_.pressstart = state_.buttonstart = false;
	}

    // BACKSPACE (BACK BUTTON)
    if (keystate[SDLK_BACKSPACE])
    {
        state_.pressback = !state_.buttonback;
        state_.buttonback = true;
    }
    else
    {
        state_.pressback = state_.buttonback = false;
    }

    // N (LEFT BUMPER)
    if (keystate[SDLK_n])
    {
        state_.presslb = !state_.lbumper;
        state_.lbumper = true;
    }
    else
    {
        state_.presslb = state_.lbumper = false;
    }

    // M (RIGHT BUMPER)
    if (keystate[SDLK_m])
    {
        state_.pressrb = !state_.rbumper;
        state_.rbumper = true;
    }
    else
    {
        state_.pressrb = state_.rbumper = false;
    }

    // set controller direction
    if (keystate[SDLK_LEFT])
    {
        state_.joyx = -1;
    }
    else if (keystate[SDLK_RIGHT])
    {
        state_.joyx = 1;
    }
    else if (keystate[SDLK_UP])
    {
        state_.joyy = 1;
    }
    else if (keystate[SDLK_DOWN])
    {
        state_.joyy = -1;
    }
}

bool Controller::keyboardPlayerExists(const std::vector<Controller*> ctrls)
{
	for (size_t i = 0; i < ctrls.size(); i++) 
	{
		if (ctrls[i]->isKeyboard_) 
			return true;
	}
	return false;
}
