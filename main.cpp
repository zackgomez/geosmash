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

static const float MAX_JOYSTICK_VALUE = 32767.0f;
static const float dt = 33.0f / 1000.0f;

static float WORLD_W = 1500.0f;
static float WORLD_H = 750.0f;
static int SCREEN_W = 1920;
static int SCREEN_H = 1080;

bool running;
SDL_Joystick *joystick;

bool paused;
int pausedPlayer = -1;
int kills[4] = {0, 0, 0, 0};

unsigned numPlayers = 1;

size_t startTime;

Controller controllers[4];
std::vector<Fighter*> fighters;
const glm::vec3 playerColors[] =
{
    glm::vec3(0.2, 0.2, 0.8),
    glm::vec3(0.1, 0.6, 0.1),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.8, 0.8, 0.2)
};

GLuint backgroundTex = 0;
const glm::mat4 perspectiveTransform = glm::ortho(-WORLD_W/2, WORLD_W/2, -WORLD_H/2, WORLD_H/2, -1.0f, 1.0f);

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

void printstats();

void updateController(Controller &controller);
void controllerEvent(Controller &controller, const SDL_Event &event);

int main(int argc, char **argv)
{
    bool muteMusic = false;
    numPlayers = 1;
    for (int i = 1; i < argc; i++)
    {
        if (i == argc - 1)
            numPlayers = std::min(4, std::max(1, atoi(argv[i])));
        if (strcmp(argv[i], "--no-music") == 0)
            muteMusic = true;
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
    ParamReader::instance()->loadFile("params.dat");
    WORLD_W = getParam("worldWidth");
    WORLD_H = getParam("worldHeight");
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = new Fighter(-225.0f+i*150, -50.f, playerColors[i], i);
        fighter->respawn(false);
        fighters.push_back(fighter);
    }
    ground = Rectangle(
            getParam("level.x"),
            getParam("level.y"),
            getParam("level.w"),
            getParam("level.h"));

    // Remove initial joystick events
    processInput();

    paused = false;


    srand(time(NULL));
    std::vector<std::string> songs;
    songs.push_back("sfx/geosmash_2.wav");
    //songs.push_back("sfx/smash.aif");
    //songs.push_back("sfx/hand canyon.wav");

    if (!muteMusic)
        start_song(songs[rand() % songs.size()].c_str());


    startTime = SDL_GetTicks();
    mainloop();

    printstats();

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
            if (event.key.keysym.sym == SDLK_m)
                stop_song();
            if (event.key.keysym.sym == SDLK_p)
                play_song();
            break;
        case SDL_QUIT:
            running = false;
            break;
        }
    }
}

void update()
{
    if (paused)
        return;

    int alivePlayers = 0;
    AudioManager::get()->update(dt);
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = fighters[i];
        if (fighter->isAlive()) alivePlayers++;
        else continue;

        // Update positions, etc
        fighter->update(controllers[i], dt);

        // Cache some vals
        const Attack *attacki = fighter->getAttack();
        // Check for hitbox collisions
        for (unsigned j = i+1; j < numPlayers; j++)
        {
            const Attack *attackj = fighters[j]->getAttack();

            // Hitboxes hit each other?
            if (fighter->hasAttack() && fighters[j]->hasAttack()
                    && attacki->getHitbox().overlaps(attackj->getHitbox()))
            {
                // Then go straight to cooldown
                fighter->attackCollision(fighters[j]->getAttack());
                fighters[j]->attackCollision(fighter->getAttack());

                // Generate small explosion
                Rectangle hitboxi = attacki->getHitbox();
                Rectangle hitboxj = attackj->getHitbox();
                float x = (hitboxi.x + hitboxj.x) / 2;
                float y = (hitboxi.y + hitboxj.y) / 2;
                ExplosionManager::get()->addExplosion(x, y, 0.1f);

                // Cache values
                attacki = fighter->getAttack();
                continue;
            }
            if (fighter->hasAttack() && fighters[j]->getRectangle().overlaps(attacki->getHitbox())
                    && attacki->canHit(fighters[j]) && fighters[j]->canBeHit())
            {
                // fighter has hit fighters[j]
                fighters[j]->hitByAttack(fighter, attacki);
                fighter->hitWithAttack(fighters[j]);

                // Cache values
                attacki = fighter->getAttack();
            }
            if (fighters[j]->hasAttack() && fighter->getRectangle().overlaps(attackj->getHitbox())
                    && attackj->canHit(fighter) && fighter->canBeHit())
            {
                // fighter[j] has hit fighter
                fighter->hitByAttack(fighters[j], attackj);
                fighters[j]->hitWithAttack(fighter);

                // Cache values
                attacki = fighter->getAttack();
            }
        }

        // Respawn condition
        if (fighter->getRectangle().y < getParam("killbox.bottom") || fighter->getRectangle().y > getParam("killbox.top")
                || fighter->getRectangle().x < getParam("killbox.left") || fighter->getRectangle().x > getParam("killbox.right"))
        {
            // Record the kill if it's not a self destruct
            if (fighter->getLastHitBy() != -1)
                kills[fighter->getLastHitBy()] += 1;
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

    // Draw the background
    glm::mat4 backtrans = glm::scale(glm::mat4(1.0f), glm::vec3(1500.0f, 750.0f, 1.0f));
    renderTexturedRectangle(backtrans, backgroundTex);

    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground.x, ground.y, 0.0)),
            glm::vec3(ground.w, ground.h, 1.0f));
    renderRectangle(transform, groundColor);

    // Draw the fighters
    for (unsigned i = 0; i < numPlayers; i++)
        if (fighters[i]->isAlive())
            fighters[i]->render(dt);

    // Draw any explosions
    ExplosionManager::get()->render(dt * !paused);

    //
    // Render the overlay interface (HUD)
    //
    //

    for (unsigned i = 0; i < numPlayers; i++)
    {
        int lives = fighters[i]->getLives();
        glm::vec2 life_area(-225.0f - 15 + 150*i, -WORLD_H/2 + 115);
        // 10unit border
        // 10unit squares
        
        // Draw life counts first
        for (int j = 0; j < lives; j++)
        {
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(life_area.x, life_area.y, 0.0f)),
                    glm::vec3(20, 20, 1.0));
            renderRectangle(transform, glm::vec3(0.25, 0.25, 0.25));

            glm::mat4 transform2 = glm::scale(transform, glm::vec3(0.8, 0.8, 1.0f));
            renderRectangle(transform2, glm::vec3(playerColors[i]));

            if (j % 2 == 0)
                life_area.x += 30;
            else
            {
                life_area.x -= 30;
                life_area.y -= 30;
            }

        }

        // Draw damage bars
        // First, render a dark grey background rect
        glm::vec2 damageBarMidpoint(-225.0f + 150*i, -WORLD_H/2 + 50);
        glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(damageBarMidpoint.x, damageBarMidpoint.y, 0.0f)),
                    glm::vec3(130, 30, 1.0));
        renderRectangle(transform, glm::vec3(0.25, 0.25, 0.25));

        float maxDamage = 100;

        float damageRatio = fighters[i]->getDamage() / maxDamage;
        float xscalefact = 0.9f * std::min(1.0f, damageRatio - floorf(damageRatio));
        float darkeningFactor = 0.60;

        // Draw the last color bar and then draw on top of it
        if (damageRatio >= 1.0f)
        {
            transform = glm::scale(
                    glm::translate(
                        transform,
                        glm::vec3(0.0f)), //glm::vec3(-.4 * .5 * xscalefact, 0.0f, 0.0f)),
                glm::vec3( 0.9f, 0.9f, 0.0f));
            renderRectangle(transform, playerColors[i] * powf(darkeningFactor, floorf(damageRatio - 1)));
        }
       
        // Now fill it in with a colored bar
        transform = glm::scale(
                glm::translate(
                    transform,
                    glm::vec3(0.0f)), //glm::vec3(-.4 * .5 * xscalefact, 0.0f, 0.0f)),
                glm::vec3( xscalefact, 0.9f, 0.0f));
        renderRectangle(transform, playerColors[i] * powf(darkeningFactor, floorf(damageRatio)));
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
    glViewport(0, 0, SCREEN_W, SCREEN_H);

    initGLUtils(perspectiveTransform);

    backgroundTex = make_texture("back003.tga");
    // Load some animation frames
    FrameManager::get()->loadFile("frames/charlie.frames");

    return 1;
}

void cleanup()
{
    std::cout << "Quiting nicely\n";
    SDL_JoystickClose(0);
    SDL_Quit();
}

void printstats()
{
    std::cout << "Run time (s): " << (SDL_GetTicks() - startTime) / 1000.0f << '\n';
    for (int i = 0; i < numPlayers; i++)
    {
        std::cout << "Player " << i+1 << " kills: " << kills[i] << '\n';
    }
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
        else if (event.jbutton.button == 7)
        {
            if (paused && pausedPlayer == event.jbutton.which)
            {
                paused = false;
                pausedPlayer = -1;
            }
            else if (!paused)
            {
                paused = true;
                pausedPlayer = event.jbutton.which;
            }
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
    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_OPENGL);
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

    SDL_WM_SetCaption("Geometry Smash 0.2", "geosmash");

    return 1;
}

// XXX this should be moved
float getParam(const std::string &param)
{
    return ParamReader::instance()->get(param);
}
