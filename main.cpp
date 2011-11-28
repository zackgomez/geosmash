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
#include "ParticleManager.h"

void test_random();

static const float MAX_JOYSTICK_VALUE = 32767.0f;
static const float dt = 1.f / 60.f;

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

unsigned numPlayers = 1;

size_t startTime;

Controller controllers[4];
std::vector<Fighter*> fighters;
std::vector<GameEntity *> entities;
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

Rectangle ground;
const glm::vec3 groundColor(0.2f, 0.2f, 0.2f);
mesh cubeMesh;
mesh levelMesh;

void pause(int playerID);
void unpause(int playerID);

void addEntity(GameEntity *entity);

// Returns the partner, or NULL if there is none
Fighter *getPartner(int playerID);

int initJoystick(unsigned numPlayers);
int initGraphics();
int initLibs();
void cleanup();

void mainloop();
void processInput();
void update();
void integrate(float dt);
void collisionDetection();
void render();
void renderHUD();
void renderArrow(const Fighter *fighter);

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


    // Init game state
    ParamReader::get()->loadFile("params.dat");
    
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

    WORLD_W = getParam("worldWidth");
    WORLD_H = getParam("worldHeight");
    for (unsigned i = 0; i < numPlayers; i++)
    {
        const glm::vec3 *colors = teams ? teamColors : playerColors;
        Fighter *fighter = new Fighter(-225.0f+i*150, 50.f, colors[i], i);
        fighter->respawn(false);
        fighters.push_back(fighter);
        entities.push_back(fighter);
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
    songs.push_back("sfx/Pixel Party.wav");
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

        int endms = SDL_GetTicks();
        int delay = 16 - std::min(16, std::max(1, endms - startms));
        /*
        std::cout << "Frame time (ms): " << endms - startms << 
            "   Delay time (ms): " << delay << '\n';
            */
        SDL_Delay(delay);
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

    // Now have the fighters process their input
    if (paused)
        return;
    for (unsigned i = 0; i < numPlayers; i++)
        fighters[i]->processInput(controllers[i], dt);
}

void update()
{
    if (paused)
        return;

    // First remove done GameEntities
    std::vector<GameEntity *>::iterator it;
    for (it = entities.begin(); it != entities.end();)
    {
        if ((*it)->isDone())
            it = entities.erase(it);
        else
            it++;
    }

    // TODO: something fancy (or not so fancy) to make this dt smaller,
    // if necessary (if a velocity is over some threshold)
    integrate(dt);
    collisionDetection();

    AudioManager::get()->update(dt);
    CameraManager::get()->update(dt, fighters);

    int alivePlayers = 0;
    int totalLives = 0;
    for (unsigned i = 0; i < numPlayers; i++)
    {
        totalLives += fighters[i]->getLives();
        if (fighters[i]->isAlive()) alivePlayers++;
    }

    // Play the tense music when two players with one life each left
    if (alivePlayers == totalLives && !muteMusic && !criticalMusic)
    {
        criticalMusic = true;
        AudioManager::get()->setSoundtrack("sfx/Critical Stealth.wav");
        AudioManager::get()->startSoundtrack();
    }

    // End the game when no one is left
    if (alivePlayers <= 0)
        running = false;
}

void integrate(float dt)
{
    for (unsigned i = 0; i < entities.size(); i++)
    {
        entities[i]->update(dt);
    }
}

void collisionDetection()
{
    // First check for hitbox collisions
    for (unsigned i = 0; i < entities.size(); i++)
    {
        GameEntity *entityi = entities[i];
        if (!entityi->hasAttack()) continue;

        const Attack *attacki = entityi->getAttack();

        for (unsigned j = i + 1; j < entities.size(); j++)
        {
            GameEntity *entityj = entities[j];
            if (!entityj->hasAttack()) continue;

            const Attack *attackj = entityj->getAttack();

            if (attacki->getHitbox().overlaps(attackj->getHitbox()))
            {
                entityi->attackCollision(attackj);
                entityj->attackCollision(attacki);
                // Generate a small explosion to show cancelling
                Rectangle hitboxi = attacki->getHitbox();
                Rectangle hitboxj = attackj->getHitbox();
                float x = (hitboxi.x + hitboxj.x) / 2;
                float y = (hitboxi.y + hitboxj.y) / 2;
                ExplosionManager::get()->addExplosion(x, y, 0.1f);
            }
        }
    }

    // Now go through and check for hitboxes hitting game entities
    for (unsigned i = 0; i < entities.size(); i++)
    {
        GameEntity *attacker = entities[i];
        if (!attacker->hasAttack()) continue;

        const Attack *attack = attacker->getAttack();

        for (unsigned j = 0; j < entities.size(); j++)
        {
            // Don't check for hitting themself
            //if (i == j) continue;

            GameEntity *victim = entities[j];
            // If the victim cannot be hit, just quit now
            if (!victim->canBeHit()) continue;

            // Hit occurs when there is overlap and attack can hit
            if (attack->getHitbox().overlaps(victim->getRect()) &&
                attack->canHit(victim))
            {
                victim->hitByAttack(attack);
                attacker->attackConnected(victim);
            }
        }
    }

    // Figher specific checks here
    // Check for fighter death and ground hit
    for (unsigned i = 0; i < numPlayers; i++)
    {
        Fighter *fighter = fighters[i];
        if (!fighter->isAlive()) continue;

        // Respawn condition
        if (fighter->getRect().y < getParam("killbox.bottom") || fighter->getRect().y > getParam("killbox.top")
                || fighter->getRect().x < getParam("killbox.left") || fighter->getRect().x > getParam("killbox.right"))
        {
            std::stringstream ss;
            ss << "Player" << fighter->getID();
            std::string died = ss.str();
            ss.str("");
            // Record the kill if it's not a self destruct
            if (fighter->getLastHitBy() != -1)
            {
                ss << "Player" << fighter->getLastHitBy();
                std::string killer = ss.str();
                StatsManager::get()->addStat(killer + ".kills." + died, 1);
                StatsManager::get()->addStat(killer + ".kills.total", 1);
            }
            else
                StatsManager::get()->addStat(died + ".suicides", 1);
            fighter->respawn(true);
            break;
        }

        // Ground check
        fighter->collisionWithGround(ground,
                fighter->getRect().overlaps(ground));
    }
}

void render()
{
    preRender();

    // Start with a blank slate
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Draw the background
    glm::mat4 backtrans = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -300)),
                glm::vec3(1920.f, 1080.f, 1.f));
    renderTexturedRectangle(backtrans, backgroundTex);

    // Draw the land
    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(ground.x, ground.y, 0.1)),
            glm::vec3(ground.w/2, ground.h/2, getParam("level.d")/2));
    renderMesh(levelMesh, transform, groundColor);

    for (unsigned i = 0; i < entities.size(); i++)
        entities[i]->render(dt);

    // Draw the fighter arrows
    for (unsigned i = 0; i < numPlayers; i++)
        if (fighters[i]->isAlive())
        {
            renderArrow(fighters[i]);
        }

    // Draw any explosions
    ExplosionManager::get()->render(dt * !paused);
    ParticleManager::get()->render(dt * !paused);

    renderHUD();


    // Finish
    postRender();
    SDL_GL_SwapBuffers();
}

void renderHUD()
{
    // Render the overlay interface (HUD)
    glDisable(GL_DEPTH_TEST);
    glm::mat4 pmat = getProjectionMatrix();
    glm::mat4 vmat = getViewMatrix();
    setProjectionMatrix(glm::mat4(1.f));
    setViewMatrix(glm::mat4(1.f));

    const glm::vec3 *colors = teams ? teamColors : playerColors;
    const glm::vec2 hud_center(0, 1.5f/6 - 1);
    const glm::vec2 lifesize = 0.03f * glm::vec2(1.f, 16.f/9.f);
    for (unsigned i = 0; i < numPlayers; i++)
    {
        const glm::vec2 player_hud_center =
            hud_center + 0.3f * glm::vec2(i - 1.5f, 0.f);

        int lives = fighters[i]->getLives();
        // Draw life counts first
        glm::vec2 life_area = player_hud_center - 0.75f * glm::vec2(lifesize.x, -lifesize.y);
        for (int j = 0; j < lives; j++)
        {
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(life_area, 0.f)),
                    glm::vec3(lifesize, 1.0f));
            renderRectangle(transform, glm::vec4(0.25f, 0.25f, 0.25f, 0.0f));

            glm::mat4 transform2 = glm::scale(transform, glm::vec3(0.8, 0.8, 1.0f));
            renderRectangle(transform2, glm::vec4(colors[i], 0.0f));

            if (j % 2 == 0)
                life_area.x += lifesize.x * 1.5;
            else
            {
                life_area.x -= lifesize.x * 1.5;
                life_area.y -= lifesize.y * 1.5;
            }

        }

        // Draw damage bars
        // First, render a dark grey background rect
        glm::vec2 damageBarMidpoint = player_hud_center + glm::vec2(0.f, -0.8f/6.f);
        glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(damageBarMidpoint.x, damageBarMidpoint.y, 0.0f)),
                    glm::vec3(0.15f, .25f * 0.15f * 16.f/9.f, 1.f));
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

    setProjectionMatrix(pmat);
    setViewMatrix(vmat);
}

void renderArrow(const Fighter *f)
{
    glm::vec4 fpos = glm::vec4(f->getRect().x, f->getRect().y, 0.f, 1.f);
    glm::vec4 fndc = getProjectionMatrix() * getViewMatrix() * fpos;
    fndc /= fndc.w;
    if (fabs(fndc.x) > 1 || fabs(fndc.y) > 1)
    {
        std::cout << "DRAWING ARROW\n";

        glm::vec2 dir = glm::vec2(fndc);
        float dist = glm::length(dir);
        dir /= dist;
        // draw arrow
        glm::vec2 side = (fabs(dir.x) > fabs(dir.y))
            ? glm::vec2(dir.x / fabs(dir.x), 0)
            : glm::vec2(0, dir.y / fabs(dir.y));
        glm::vec2 move = (fabs(dir.x) < fabs(dir.y))
            ? glm::vec2(dir.x / fabs(dir.x), 0)
            : glm::vec2(0, dir.y / fabs(dir.y));

        float theta = acos(glm::dot(glm::vec2(-1, 0), dir)) * ((fndc.y > 0) ? -1.f : 1.f);

        glm::vec2 arrowPos = side +
            move * glm::vec2(1,1) * glm::vec2(fabs(cosf(theta)), fabs(sinf(theta)));

        const float arrowsz = 0.005;
        float len = glm::length(arrowPos);
        arrowPos *= (len - 10*arrowsz) / len;

        float scale = 0.002 * ((dist / len - 1.f) * 5.f + 0.5f);
        float rot = theta * 180.f / M_PI;

        std::cout << "Arrow pos: " << arrowPos.x << ' ' << arrowPos.y << '\n';
        glm::mat4 transform =
            glm::rotate(glm::scale(glm::translate(glm::mat4(1.0f),
                            glm::vec3(arrowPos, 0.0f)), 
                        glm::vec3(scale, scale, 1.f)),
                    rot, glm::vec3(0, 0, 1));

        glm::mat4 pmat = getProjectionMatrix();
        glm::mat4 vmat = getViewMatrix();
        setProjectionMatrix(glm::mat4(1.f));
        setViewMatrix(glm::mat4(1.f));

        FrameManager::get()->renderFrame(transform, glm::vec4(f->getColor(), 0.0f), "OffscreenArrow");

        setProjectionMatrix(pmat);
        setViewMatrix(vmat);
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

    glm::vec3 cameraLoc(0.f, 0.f, 425.0f);
    setCamera(cameraLoc);
    CameraManager::get()->setCurrent(cameraLoc);

    backgroundTex = make_texture("back003.tga");
    // Load some animation frames
    FrameManager::get()->loadFile("frames/charlie.frames");

    // Load the ground mesh
    cubeMesh = createMesh("models/cube.obj");
    levelMesh = createMesh("models/level.obj");

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

    StatsManager::get()->printStats();
}

void updateController(Controller &controller)
{
    controller.pressa = false;
    controller.pressb = false;
    controller.pressc = false;
    controller.pressjump = false;
    controller.pressstart = false;
    controller.pressrb = false;
    controller.presslb = false;
    controller.joyxv = 0;
    controller.joyyv = 0;
}

void controllerEvent(Controller &controller, const SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_JOYAXISMOTION:
        // left joy stick X axis
        if (event.jaxis.axis == 0)
        {
            float newPos = event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.joyxv += (newPos - controller.joyx);
            controller.joyx = newPos;
        }
        // left joy stick Y axis
        else if (event.jaxis.axis == 1)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.joyyv += (newPos - controller.joyy);
            controller.joyy = newPos;
        }
        // Left trigger
        else if (event.jaxis.axis == 5)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.ltrigger = newPos;
        }
        // Right trigger
        else if (event.jaxis.axis == 4)
        {
            float newPos = -event.jaxis.value / MAX_JOYSTICK_VALUE;
            controller.rtrigger = newPos;
        }
        // DPAD L/R
        else if (event.jaxis.axis == 6)
        {
            if (event.jaxis.value > 0)
                controller.dpadr = true;
            else if (event.jaxis.value < 0)
                controller.dpadl = true;
            else
                controller.dpadl = controller.dpadr = false;
        }
        // DPAD U/D
        else if (event.jaxis.axis == 7)
        {
            if (event.jaxis.value > 0)
                controller.dpadd = true;
            else if (event.jaxis.value < 0)
                controller.dpadu = true;
            else
                controller.dpadd = controller.dpadu = false;
        }
        else
        {
            std::cout << "Axis event: " << (int) event.jaxis.axis << '\n';
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
        else if (event.jbutton.button == 4)
        {
            controller.presslb = true;
            controller.lbumper = true;
        }
        else if (event.jbutton.button == 5)
        {
            controller.pressrb = true;
            controller.rbumper = true;
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
        else if (event.jbutton.button == 4)
        {
            controller.presslb = false;
            controller.lbumper = false;
        }
        else if (event.jbutton.button == 5)
        {
            controller.pressrb = false;
            controller.rbumper = false;
        }
        else
        {
            std::cout << "Got button number : " << (int) event.jbutton.button << '\n';
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
    return ParamReader::get()->get(param);
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

void addEntity(GameEntity *ent)
{
    // TODO investigate placing these into a temporary buffer that's added
    // at the beginning of each frame to avoid strange behavior.
    entities.push_back(ent);
}
