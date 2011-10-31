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
    glm::vec3(0.2, 0.2, 0.8),
    glm::vec3(0.2, 0.8, 0.2),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.8, 0.8, 0.2)
};

Rectangle ground;
const glm::vec3 groundColor(0.5f, 0.5f, 0.5f);


int initJoystick(unsigned numPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void processInput();
void update();
void render();

void updateController(Controller &controller);
void controllerEvent(Controller &controller, const SDL_Event &event);

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
        Fighter *fighter = new Fighter(Rectangle(0, 0, 50, 60), -225.0f+i*150, -100.f, playerColors[i]);
        fighter->respawn(false);
        fighters.push_back(fighter);
    }
    ground = Rectangle(0, -375+50, 1025, 100);

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
    // First update controllers / frame
    for (unsigned i = 0; i < numPlayers; i++)
    {
        updateController(controllers[i]);
    }

    // Now read new inputs
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
            controllerEvent(controllers[idx], event);

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
    int alivePlayers = 0;
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = fighters[i];
        if (fighter->isAlive()) alivePlayers++;

        // Update positions, etc
        fighter->update(controllers[i], dt);

        // Cache some vals
        Rectangle hitboxi = fighter->getAttackBox();
        bool fiattack = fighter->hasAttack();
        // Check for hitbox collisions
        for (unsigned j = i+1; j < numPlayers; j++)
        {
            Rectangle hitboxj = fighters[j]->getAttackBox();
            bool fjattack = fighters[j]->hasAttack();

            // Hitboxes hit each other?
            if (fiattack && fjattack && hitboxi.overlaps(hitboxj))
            {
                // Then go straight to cooldown
                fighter->attackCollision();
                fighters[j]->attackCollision();

                fiattack = fighter->hasAttack();
                hitboxi = fighter->getAttackBox();
                continue;
            }
            if (fiattack && fighters[j]->getRectangle().overlaps(hitboxi))
            {
                // fighter has hit fighters[j]
                fighters[j]->hitByAttack(hitboxi);
                fighter->hitWithAttack();

                fiattack = fighter->hasAttack();
                hitboxi = fighter->getAttackBox();
            }
            if (fjattack && fighter->getRectangle().overlaps(hitboxj))
            {
                // fighter[j] has hit fighter
                fighter->hitByAttack(hitboxj);
                fighters[j]->hitWithAttack();

                fiattack = fighter->hasAttack();
                hitboxi = fighter->getAttackBox();
            }
        }

        // Respawn condition
        if (fighter->getRectangle().y < -350 - 100.0f)
        {
            fighter->respawn(true);
            break;
        }
        // Ground check
        fighter->collisionWithGround(ground,
                fighter->getRectangle().overlaps(ground));
    }

    // End the game when no one is left
    if (alivePlayers <= 0)
        running = false;
}

void render()
{
    // Start with a blank slate
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground.x, ground.y, 0.0)),
            glm::vec3(ground.w, ground.h, 1.0f));
    renderRectangle(transform, groundColor);

    for (unsigned i = 0; i < numPlayers; i++)
        fighters[i]->render(dt);

    //
    // Render the overlay interface (HUD)
    //

    for (unsigned i = 0; i < numPlayers; i++)
    {
        int lives = fighters[i]->getLives();
        glm::vec2 life_area(-225.0f - 10 + 150*i, ground.y + 15);
        // 10unit border
        // 10unit squares
        
        // Draw life counts first
        for (int j = 0; j < lives; j++)
        {
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(life_area.x, life_area.y, 0.0f)),
                    glm::vec3(10, 10, 1.0));
            renderRectangle(transform, playerColors[i]);

            if (j % 2 == 0)
                life_area.x += 20;
            else
            {
                life_area.x -= 20;
                life_area.y -= 20;
            }

        }

        // Draw damage bars
        // First, render a black background rect
        glm::vec2 damageBarMidpoint(-225.0f + 150*i, ground.y - 25);
        glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(damageBarMidpoint.x, damageBarMidpoint.y, 0.0f)),
                    glm::vec3(100, 20, 1.0));
        renderRectangle(transform, glm::vec3(0, 0, 0));
       
        // Now fill it in with a colored bar
        float maxDamage = 100;
        float xscalefact = std::min(0.9f, fighters[i]->getDamage() / maxDamage);
        transform = glm::scale(
                glm::translate(
                    transform,
                    glm::vec3(0.0f)), //glm::vec3(-.4 * .5 * xscalefact, 0.0f, 0.0f)),
                glm::vec3( xscalefact, 0.9f, 0.0f));
        renderRectangle(transform, playerColors[i]);
    }


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
    glViewport(0, 0, 1920, 1080);

    initGLUtils();

    return 1;
}

void cleanup()
{
    std::cout << "Quiting nicely\n";
    SDL_JoystickClose(0);
    SDL_Quit();
}

void updateController(Controller &controller)
{
    controller.pressa = false;
    controller.pressb = false;
    controller.pressc = false;
    controller.pressjump = false;
    controller.joyxv = 0;
    controller.joyyv = 0;
}

void controllerEvent(Controller &controller, const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_JOYAXISMOTION:
        if (event.jaxis.axis == 0)
        {
            float newPos = event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.joyxv += (newPos - controller.joyx);
            controller.joyx = newPos;
        }
        else if (event.jaxis.axis == 1)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.joyyv += (newPos - controller.joyy);
            controller.joyy = newPos;
        }
        break;

    case SDL_JOYBUTTONDOWN:
        if (event.jbutton.button == 0)
        {
            controller.pressa = true;
            controller.buttona = true;
        }
        else if (event.jbutton.button == 1)
        {
            controller.pressb = true;
            controller.buttonb = true;
        }
        else if (event.jbutton.button == 3)
        {
            controller.pressjump = true;
            controller.jumpbutton = true;
        }
        else if (event.jbutton.button == 2)
        {
            controller.pressc = !controller.buttonc;
            controller.buttonc = true;
        }
        break;

    case SDL_JOYBUTTONUP:
        if (event.jbutton.button == 0)
        {
            controller.buttona = false;
            controller.pressa = false;
        }
        else if (event.jbutton.button == 1)
        {
            controller.buttonb = false;
            controller.pressb = false;
        }
        else if (event.jbutton.button == 3)
        {
            controller.jumpbutton = false;
            controller.pressjump = false;
        }
        else if (event.jbutton.button == 2)
        {
            controller.buttonc = false;
            controller.pressc = false;
        }
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
    SDL_Surface *screen = SDL_SetVideoMode(1920, 1080, 32, SDL_OPENGL);
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
    return 1;
}
