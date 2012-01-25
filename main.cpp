#define _USE_MATH_DEFINES
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include "Engine.h"
#include "AudioManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "StatsManager.h"
#include "GameState.h"
#include "MenuState.h"

static const float dt = 1.f / 60.f;

std::vector<Controller*> controllers;
bool running;
GameState *state;

int initJoystick(unsigned numPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void globalEvents();

int main(int argc, char *argv[])
{
    // Init game state
    ParamReader::get()->loadFile("config/global.params");
    ParamReader::get()->loadFile("config/charlie.params");

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

    // Read in lifetime stats/usernames
    StatsManager::get()->readUserFile("user_stats.dat");

    mainloop();

    cleanup();
    return 0;
}

void mainloop()
{
    running = true;
    state = new MenuState();
    while (running)
    {
        // For frame timing
        int startms = SDL_GetTicks();

        // Global events like ESC or mute etc
        globalEvents();

        // Update controllers
        for (size_t i = 0; i < controllers.size(); i++)
            controllers[i]->update(dt);

        GameState *nextState;
        // Process input, checking for new states along the way
        while ((nextState = state->processInput(controllers, dt)))
        {
            delete state;
            state = nextState;
        }

        // Update
        state->update(dt);

        // Render
        preRender();
        glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        state->render(dt);

        postRender();
        SDL_GL_SwapBuffers();

        // Some timing and delay to force framerate
        int endms = SDL_GetTicks();
        int delay = 16 - std::min(16, std::max(0, endms - startms));
        /*
        std::cout << "Frame time (ms): " << endms - startms << 
            "   Delay time (ms): " << delay << '\n';
            */
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
                AudioManager::get()->mute();
                AudioManager::get()->pauseSoundtrack();
            if (event.key.keysym.sym == SDLK_p)
                AudioManager::get()->unmute();
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
        controllers.push_back(new Controller(i));

    return numPlayers;
}

int initGraphics()
{
    // Set the viewport
    glViewport(0, 0, getParam("resolution.x"), getParam("resolution.y"));

    initGLUtils(getParam("resolution.x"), getParam("resolution.y"));

    FrameManager::get()->loadFile("frames/charlie.frames");
    FrameManager::get()->loadFile("frames/global.frames");

    return 0;
}

void cleanup()
{
    std::cout << "Cleaning up...\n";
    for (unsigned i = 0; i < controllers.size(); i++)
        delete controllers[i];

    delete state;

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

    // Seed random number generator
    srand(time(NULL));

    return 1;
}
