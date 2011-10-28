#include <GL/glew.h>
#include <SDL/SDL.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "glutils.h"
#include "Fighter.h"
#include <glm/gtc/matrix_transform.hpp>

static const float MAX_JOYSTICK_VALUE = 32767;
static const float dt = 33.0 / 1000.0;

bool running;
SDL_Joystick *joystick;

Controller controller;
Fighter *fighter;


int initJoystick();
int initGraphics();
void cleanup();

void processInput();
void update();
void render();

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << '\n';
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface *screen = SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
    if ( screen == NULL ) {
        fprintf(stderr, "Couldn't set 640x480x8 video mode: %s\n",
                SDL_GetError());
        exit(1);
    }
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit(1);
    }


    if (!initJoystick())
    {
        std::cerr << "Unable to initialize Joystick(s)\n";
        exit(1);
    }
    if (!initGraphics())
    {
        std::cerr << "Unable to initialize graphics resources\n";
        exit(1);
    }

    fighter = new Fighter(Rectangle(0, -375+130, 50, 60));

    running = true;
    while (running)
    {
        processInput();
        update();
        render();

        SDL_Delay(static_cast<int>(dt * 1000.0));
    }

    cleanup();

    return 0;
}

void processInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
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
            // TODO do something with buttons 0-3 (face buttons)
            break;

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
    fighter->update(controller, dt);
}

void render()
{
    // Start with a blank slate
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw the land
    glm::vec3 color(200, 0, 200);
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0, -375.0+50.0, 0.0)),
            glm::vec3(750.0, 100.0f, 1.0f));
    renderRectangle(transform, color);

    fighter->render(dt);

    // Finish
    SDL_GL_SwapBuffers();
}

int initJoystick()
{
    int numJoysticks = SDL_NumJoysticks();
    std::cout << "Available joysticks: " << numJoysticks << '\n';
    for (int i = 0; i < numJoysticks; i++)
        std::cout << "Joystick: " << SDL_JoystickName(i) << '\n';

    if (numJoysticks == 0)
        return 0;

    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);

    return 1;
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
