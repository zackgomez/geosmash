#define _USE_MATH_DEFINES
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include "glutils.h"
#include "Fighter.h"
#include "audio.h"
#include "explosion.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "StatsManager.h"
#include "CameraManager.h"
#include "Attack.h"
#include "PManager.h"
#include "FontManager.h"
#include "StageManager.h"
#include "Controller.h"

std::vector<SDL_Joystick*> joysticks;
bool running;
GameState *gameState;

int initJoystick(unsigned numPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void globalEvents();

int main(int argc, char *argv[])
{
    // Init game state
    ParamReader::get()->loadFile("params.dat");

    if (!initLibs())
        exit(1);

    if ((initJoystick(4)) == 0)
    {
        std::cerr << "Unable to initialize Joystick(s)\n";
        exit(1);
    }
    if (initGraphics())
    {
        std::cerr << "Unable to initialize graphics resources\n";
        exit(1);
    }

    running = true;
    mainloop();

    cleanup();
    return 0;
}

void mainloop()
{
    running = true;
    while (running)
    {
        int startms = SDL_GetTicks();

        globalEvents();

        // TODO call state functions here

        int endms = SDL_GetTicks();
        int delay = 16 - std::min(16, std::max(0, endms - startms));
        std::cout << "Frame time (ms): " << endms - startms << 
            "   Delay time (ms): " << delay << '\n';
        SDL_Delay(delay);
    }
}

void globalEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            if (event.key.keysym.sym == SDLK_m)
                AudioManager::get()->pauseSoundtrack();
            if (event.key.keysym.sym == SDLK_p)
                AudioManager::get()->startSoundtrack();
            break;
        case SDL_QUIT:
            running = false;
            break;
        }
    }
}

int initJoystick(unsigned numPlayers)
{
    unsigned numJoysticks = SDL_NumJoysticks();
    std::cout << "Available joysticks: " << numJoysticks << '\n';
    for (unsigned i = 0; i < numJoysticks; i++)
        std::cout << "Joystick: " << SDL_JoystickName(i) << '\n';

    if (numJoysticks == 0)
        return 0;

    unsigned i;
    for (i = 0; i < numJoysticks && i < numPlayers; i++)
        joysticks.push_back(SDL_JoystickOpen(i));

    if (i != numPlayers)
        return 0;

    return numPlayers;
}

int initGraphics()
{
    // Set the viewport
    glViewport(0, 0, getParam("resolution.x"), getParam("resolution.y"));

    initGLUtils(getParam("resolution.x"), getParam("resolution.y"));

    return 0;
}

void cleanup()
{
    std::cout << "Cleaning up...\n";
    for (unsigned i = 0; i < joysticks.size(); i++)
        SDL_JoystickClose(joysticks[i]);

    std::cout << "Quiting nicely\n";
    SDL_Quit();
}

int initLibs()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << '\n';
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen = SDL_SetVideoMode(getParam("resolution.x"),
            getParam("resolution.y"), 32, SDL_OPENGL);
    if ( screen == NULL ) {
        fprintf(stderr, "Couldn't set video mode: %s\n",
                SDL_GetError());
        return 0;
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return 0;
    }

    SDL_WM_SetCaption("Geometry Smash 0.6", "geosmash");

    return 1;
}


// XXX this should be moved
float getParam(const std::string &param)
{
    return ParamReader::get()->get(param);
}

