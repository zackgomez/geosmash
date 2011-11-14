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
static int SCREEN_W = 2560;
static int SCREEN_H = 1600;

bool running;
bool teams;
bool muteMusic;
bool criticalMusic = false;
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
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.1, 0.6, 0.1),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.7, 0.7, 0.2)
};
const glm::vec3 teamColors[] =
{
    glm::vec3(0.0, 0.2, 1.0),
    glm::vec3(0.2, 0.6, 0.8),
    glm::vec3(0.8, 0.2, 0.2),
    glm::vec3(0.8, 0.35, 0.1)
};

GLuint backgroundTex = 0;
GLuint groundTex = 0;
const glm::mat4 perspectiveTransform = glm::ortho(-WORLD_W/2, WORLD_W/2, -WORLD_H/2, WORLD_H/2, -1.0f, 1.0f);

Rectangle ground;
const glm::vec3 groundColor(0.5f, 0.5f, 0.5f);

void pause(int playerID);
void unpause(int playerID);

// Returns the partner, or NULL if there is none
Fighter *getPartner(int playerID);

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
    muteMusic = false;
    numPlayers = 1;
    for (int i = 1; i < argc; i++)
    {
        if (i == argc - 1)
            numPlayers = std::min(4, std::max(1, atoi(argv[i])));
        if (strcmp(argv[i], "--no-music") == 0)
            muteMusic = true;
        if (strcmp(argv[i], "--teams") == 0)
            teams = true;
    }
    
    if (teams && numPlayers != 4)
    {
        std::cerr << "Teams only supported with 4 players\n";
        exit(1);
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
        const glm::vec3 *colors = teams ? teamColors : playerColors;
        Fighter *fighter = new Fighter(-225.0f+i*150, -50.f, colors[i], i);
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
    songs.push_back("sfx/geosmash.wav");
    songs.push_back("sfx/hand canyon.wav");
    songs.push_back("sfx/Meat DeFeat.wav");
    AudioManager::get()->setSoundtrack(songs[rand() % songs.size()]);

    if (!muteMusic)
        AudioManager::get()->startSoundtrack();


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
        int startms = SDL_GetTicks();
        processInput();
        update();
        render();

        // XXX fix this
        int endms = SDL_GetTicks();
        int delay = std::max(1, endms - startms);
        //std::cout << "Frame time (ms): " << endms - startms << '\n';
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
            {
                AudioManager::get()->pauseSoundtrack();
                muteMusic = true;
            }
            if (event.key.keysym.sym == SDLK_p)
            {
                AudioManager::get()->startSoundtrack();
                muteMusic = false;
            }
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
    int totalLives = 0;
    AudioManager::get()->update(dt);
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = fighters[i];
        totalLives += fighters[i]->getLives();
        if (fighter->isAlive()) alivePlayers++;

        // Update positions, etc
        fighter->update(controllers[i], dt);

        // dont do other updates when player is dead
        if (!fighter->isAlive()) continue;


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
                // Then resolve collision
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

    // Play the tense music when two players with one life each left
    // XXX this could not work with teams
    if (alivePlayers == 2 && totalLives == 2 && !muteMusic && !criticalMusic)
    {
        criticalMusic = true;
        AudioManager::get()->setSoundtrack("sfx/Critical Stealth.wav");
        AudioManager::get()->startSoundtrack();
    }

    // End the game when no one is left
    if (alivePlayers <= 0)
        running = false;
}

void render()
{
    preRender();

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
    renderRectangle(transform, glm::vec4(groundColor, 0.0f));
    //renderTexturedRectangle(transform, groundTex);

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

    const glm::vec3 *colors = teams ? teamColors : playerColors;
    for (unsigned i = 0; i < numPlayers; i++)
    {
        int lives = fighters[i]->getLives();
        glm::vec2 life_area(-225.0f - 15 + 150*i, -WORLD_H/2 + 150);
        
        // Draw life counts first
        for (int j = 0; j < lives; j++)
        {
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(life_area.x, life_area.y, 0.0f)),
                    glm::vec3(20, 20, 1.0));
            renderRectangle(transform, glm::vec4(0.25f, 0.25f, 0.25f, 0.0f));

            glm::mat4 transform2 = glm::scale(transform, glm::vec3(0.8, 0.8, 1.0f));
            renderRectangle(transform2, glm::vec4(colors[i], 0.0f));

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
        glm::vec2 damageBarMidpoint(-225.0f + 150*i, -WORLD_H/2 + 75);
        glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(damageBarMidpoint.x, damageBarMidpoint.y, 0.0f)),
                    glm::vec3(130, 30, 1.0));
        renderRectangle(transform, glm::vec4(0.25, 0.25, 0.25, 0.0f));

        float maxDamage = 100;

        float damageRatio = fighters[i]->getDamage() / maxDamage;
        float xscalefact = 0.9f * std::min(1.0f, damageRatio - floorf(damageRatio));
        float darkeningFactor = 0.60;
        
        if (!fighters[i]->isAlive())
            continue;

        // Draw the last color bar and then draw on top of it
        glm::mat4 curtransform = glm::scale(
                glm::translate(
                    transform,
                    glm::vec3(0.0f)),
                glm::vec3( 0.9f, 0.9f, 0.0f));
        renderRectangle(curtransform,
                glm::vec4(colors[i] * powf(darkeningFactor, floorf(damageRatio)), 0.0f));
       
        // Now fill it in with a colored bar
        transform = glm::scale(
                glm::translate(
                    transform,
                    glm::vec3(-.5 * xscalefact + 0.5 * 0.9, 0.0f, 0.0f)),
                glm::vec3( xscalefact, 0.9f, 0.0f));
        renderRectangle(transform,
                glm::vec4(colors[i] * powf(darkeningFactor, floorf(damageRatio+1)), 0.0f));
    }


    // Finish
    postRender();
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

    initGLUtils(SCREEN_W, SCREEN_H);
    setPerspective(perspectiveTransform);

    backgroundTex = make_texture("back003.tga");
    groundTex = make_texture("ground.tga");
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
            controller.pressc = true;
            controller.buttonc = true;
        }
        else if (event.jbutton.button == 7)
        {
            if (paused)
                unpause(event.jbutton.which);
            else
            {
                controller.pressstart = true;
                controller.buttonstart = true;
            }
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
            controller.buttonstart = false;
            controller.pressstart = false;
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

    SDL_WM_SetCaption("Geometry Smash 0.4", "geosmash");

    return 1;
}

// XXX this should be moved
float getParam(const std::string &param)
{
    return ParamReader::instance()->get(param);
}

void pause(int playerID)
{
    if (!paused)
    {
        paused = true;
        pausedPlayer = playerID;
        AudioManager::get()->pauseSoundtrack();
    }
}

void unpause(int playerID)
{
    if (paused && pausedPlayer == playerID)
    {
        paused = false;
        pausedPlayer = -1;
        if (!muteMusic)
            AudioManager::get()->startSoundtrack();
    }
}

Fighter *getPartner(int playerID)
{
    if (!teams)
        return NULL;
    // Even, 0, 2 -> 1, 3
    if (playerID % 2 == 0)
        return fighters[playerID + 1];
    // Odd: 1,3 -> 0, 2
    else
        return fighters[playerID - 1];
};
