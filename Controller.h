#pragma once
#include <SDL/SDL.h>

class Fighter;

struct controller_state
{
    // The positions [-1, 1] of the main analog stick
    float joyx, joyy;
    // The velocities of the main analog stick over some time period
    float joyxv, joyyv;
    // nonzero if the button is pressed
    bool buttona, buttonb, buttonc, jumpbutton, buttonstart;
    // nonzero if the button was pressed this frame
    bool pressa, pressb, pressc, pressjump, pressstart;

    float rtrigger, ltrigger;
    bool lbumper, rbumper;
    bool presslb, pressrb;

    // Dpad directions, true if they're press currently
    bool dpadl, dpadr, dpadu, dpadd;
};


class Controller
{
public:
    Controller(const Fighter *f, SDL_Joystick *joystick);
    virtual ~Controller();

    // Gets the next state
    virtual controller_state nextState();
    virtual controller_state lastState();
    // Returns true if this controller would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const;

    // Does per frame controller updates...
    virtual void update(float dt);

    // For human controllers only - processes an SDL event
    void processEvent(SDL_Event &e);

private:
    const Fighter *fighter_;
    SDL_Joystick *joystick_;
    controller_state state_;
    controller_state last_;
};

