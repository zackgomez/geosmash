#include <GL/glew.h>
#include <SDL/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include "glutils.h"
#include "Fighter.h"

static const float MAX_JOYSTICK_VALUE = 32767;
static const float dt = 33.0 / 1000.0;

bool running;
SDL_Joystick *joystick;

unsigned numPlayers = 1;

Controller controllers[4];
std::vector<Fighter*> fighters;
const glm::vec3 playerColors[] =
{
    glm::vec3(0.2, 0.2, 1.0),
    glm::vec3(0.2, 1.0, 0.2),
    glm::vec3(1.0, 0.2, 0.2),
    glm::vec3(1.0, 1.0, 0.2)
};

Rectangle ground;


int initJoystick(unsigned numPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void processInput();
void update();
void render();

void updateController(Controller &controller, const SDL_Event &event);

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cout << "usage: " << argv[0] << " [nplayers]\n";
        exit(1);
    }
    if (argc == 2)
    {
        numPlayers = std::min(4, std::max(1, atoi(argv[1])));
    }

    if (!initLibs())
        exit(1);

    if ((initJoystick(numPlayers)) == 0)
    {
        std::cerr << "Unable to initialize Joystick(s)\n";
        exit(1);
    }
    if (!initGraphics())
    {
        std::cerr << "Unable to initialize graphics resources\n";
        exit(1);
    }

    // Init game state
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = new Fighter(Rectangle(0, 0, 50, 60), playerColors[i]);
        fighter->respawn();
        fighters.push_back(fighter);
    }
    ground = Rectangle(0, -375+50, 750, 100);

    mainloop();

    cleanup();

    return 0;
}

void mainloop()
{
    running = true;
    while (running)
    {
        processInput();
        update();
        render();

        SDL_Delay(static_cast<int>(dt * 1000.0));
    }
}

void processInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        int idx = -1;
        switch (event.type)
        {
        case SDL_JOYAXISMOTION:
            idx = idx == -1 ? event.jaxis.which : idx;
        case SDL_JOYBUTTONDOWN:
            idx = idx == -1 ? event.jbutton.which : idx;
        case SDL_JOYBUTTONUP:
            idx = idx == -1 ? event.jbutton.which : idx;
            updateController(controllers[idx], event);

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            break;
        case SDL_QUIT:
            running = false;
            break;
        }
    }
}

void update()
{
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = fighters[i];

        // Update positions, etc
        fighter->update(controllers[i], dt);

        // Respawn condition
        if (fighter->getRectangle().y < -350 - 100.0f)
            fighter->respawn();
        // Collision detection
        fighter->collisionWithGround(ground,
                fighter->getRectangle().overlaps(ground));
    }
}

void render()
{
    // Start with a blank slate
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw the land
    glm::vec3 color(200/255.0, 0, 200/255.0);
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground.x, ground.y, 0.0)),
            glm::vec3(ground.w, ground.h, 1.0f));
    renderRectangle(transform, color);

    for (unsigned i = 0; i < numPlayers; i++)
        fighters[i]->render(dt);

    // Finish
    SDL_GL_SwapBuffers();
}

int initJoystick(unsigned numPlayers)
{
    unsigned numJoysticks = SDL_NumJoysticks();
    std::cout << "Available joysticks: " << numJoysticks << '\n';
    for (unsigned i = 0; i < numJoysticks; i++)
        std::cout << "Joystick: " << SDL_JoystickName(i) << '\n';

    if (numJoysticks == 0)
        return 0;

    SDL_JoystickEventState(SDL_ENABLE);

    unsigned i;
    for (i = 0; i < numJoysticks && i < numPlayers; i++)
        joystick = SDL_JoystickOpen(i);

    if (i != numPlayers)
        return 0;

    return numPlayers;
}

int initGraphics()
{
    // Set the viewport
    glViewport(0, 0, 640, 480);

    initGLUtils();

    return 1;
}

void cleanup()
{
    std::cout << "Quiting nicely\n";
    SDL_JoystickClose(0);
    SDL_Quit();
}

void updateController(Controller &controller, const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_JOYAXISMOTION:
        if (event.jaxis.axis == 0)
            controller.joyx = event.jaxis.value / MAX_JOYSTICK_VALUE;
        else if (event.jaxis.axis == 1)
            controller.joyy = -event.jaxis.value / MAX_JOYSTICK_VALUE;
        break;

    case SDL_JOYBUTTONDOWN:
        if (event.jbutton.button == 0)
            controller.buttona = true;
        else if (event.jbutton.button == 1)
            controller.buttonb = true;
        else if (event.jbutton.button == 3)
            controller.jumpbutton = true;
        else if (event.jbutton.button == 2)
            controller.buttonc = true;
        break;

    case SDL_JOYBUTTONUP:
        if (event.jbutton.button == 0)
            controller.buttona = false;
        else if (event.jbutton.button == 1)
            controller.buttonb = false;
        else if (event.jbutton.button == 3)
            controller.jumpbutton = false;
        else if (event.jbutton.button == 2)
            controller.buttonc = false;
        break;

    default:
        std::cout << "WARNING: Unknown event in updateController.\n";
    }
}

int initLibs()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << '\n';
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen = SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
    if ( screen == NULL ) {
        fprintf(stderr, "Couldn't set 640x480x8 video mode: %s\n",
                SDL_GetError());
        return 0;
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return 0;
    }
    return 1;
}
