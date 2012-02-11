#define _USE_MATH_DEFINES
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include "Engine.h"
#include "AudioManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "StatsManager.h"
#include "GameState.h"
#include "MenuState.h"
#include "Logger.h"

static const float dt = 1.f / 60.f;

LoggerPtr logger;

std::vector<Controller*> controllers;
bool running;
GameState *state;

void checkForJoysticks(unsigned maxPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void globalEvents();

int main(int argc, char *argv[])
{
    logger = Logger::getLogger("Main");

    // Init game state
    ParamReader::get()->loadFile("config/global.params");
    ParamReader::get()->loadFile("config/charlie.params");
    ParamReader::get()->loadFile("config/stickman.params");

    ParamReader::get()->setParam("debug", 0.f);
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--debug") == 0)
            ParamReader::get()->setParam("debug", 1.f);
    }

    if (!initLibs())
        exit(1);

    if (!initGraphics())
    {
        logger->fatal() << "Unable to initialize graphics resources\n";
        exit(1);
    }

    // Read in lifetime stats/usernames
    StatsManager::get()->readUserFile("user_stats.dat");

    mainloop();

    return 0;
}

void mainloop()
{
    int count = 0;
    running = true;
    state = new MenuState();
    while (running)
    {
        // For frame timing
        int startms = SDL_GetTicks();

        // Global events like ESC or mute etc
        globalEvents();
		
        state->preFrame();

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

        state->postFrame();

        postRender();
        SDL_GL_SwapBuffers();

        // Some timing and delay to force framerate
        int endms = SDL_GetTicks();
        int delay = 16 - std::min(16, std::max(0, endms - startms));
        /*
        logger_->debug() << "Frame time (ms): " << endms - startms << 
            "   Delay time (ms): " << delay << '\n';
            */
        SDL_Delay(delay);
    }
}

void addKeyboardController() 
{
	// Make sure to tell this controller that it's a keyboard ctrl
	if (controllers.size() == getParam("maxPlayers"))
	{
		return;
	}
	Controller *c = new KeyboardController(controllers.size());
	controllers.push_back(c);
}

// Check for global mute, keyboard pause or exit, or if a keyboard player
// is requested.
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

void checkForJoysticks(unsigned maxPlayers)
{
    unsigned numJoysticks = SDL_NumJoysticks();
	Uint8 *keystate = SDL_GetKeyState(NULL);
	static bool apressed = false;
	if (keystate[SDLK_a])
	{
		if (!apressed)
			// A developer requested a keyboard controller
			addKeyboardController();
		apressed = true;
	}
	else
		apressed = false;

    // Are there new joysticks?
    if (numJoysticks > controllers.size())
    {
        // Add some joysticks
        for (unsigned i = controllers.size(); i < numJoysticks && i < maxPlayers; i++)
        {
            logger->info() << "Adding joystick: " << SDL_JoystickName(i) << '\n';
            controllers.push_back(new Controller(i));

        }
    }
}

int initGraphics()
{
    float xres = getParam("debug") ? getParam("debug.resolution.x") : getParam("resolution.x");
    float yres = getParam("debug") ? getParam("debug.resolution.y") : getParam("resolution.y");

    int flags = SDL_OPENGL;
    if (!getParam("debug"))
        flags |= SDL_FULLSCREEN;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen = SDL_SetVideoMode(xres, yres, 32, flags);
    if (screen == NULL)
    {
        logger->fatal() << "Couldn't set video mode: " << SDL_GetError() << '\n';
        return 0;
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        logger->fatal() <<  "Error: " << glewGetErrorString(err) << '\n';
        return 0;
    }

    if (!getParam("debug"))
        SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_SetCaption("GeoSMASH BUILT " __DATE__ " " __TIME__, "geosmash");
    // Set the viewport
    glViewport(0, 0, xres, yres);

    initGLUtils(xres, yres);

    FrameManager::get()->loadFrameFile("models/charlie.frames");
    FrameManager::get()->loadFrameFile("models/global.frames");
    FrameManager::get()->loadPoseFile("models/charlie.poses");

    return 1;
}

void cleanup()
{
    logger->info() << "Cleaning up...\n";
    for (unsigned i = 0; i < controllers.size(); i++)
        delete controllers[i];

    delete state;

    logger->info() << "Quiting nicely\n";
    SDL_Quit();
}

int initLibs()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        logger->fatal() << "Couldn't initialize SDL: " << SDL_GetError() << '\n';
        return 0;
    }

    atexit(cleanup);

    // Get AudioManager singleton to call constructor
    AudioManager::get();
    // Mute on debug by default
    if (getParam("debug"))
        AudioManager::get()->mute();

    // Seed random number generator
    srand(time(NULL));

    return 1;
}
