#define _USE_MATH_DEFINES
// TODO clean up these includes for the love of god
#include "InGameState.h"
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include "glutils.h"
#include "Fighter.h"
#include "AudioManager.h"
#include "ExplosionManager.h"
#include "ParamReader.h"
#include "FrameManager.h"
#include "StatsManager.h"
#include "CameraManager.h"
#include "Attack.h"
#include "PManager.h"
#include "FontManager.h"
#include "StageManager.h"
#include "Controller.h"
#include <cassert>
#include "StatsGameState.h"

std::vector<GameEntity *> getEntitiesToAdd();
void addEntity(GameEntity *ent);

InGameState *InGameState::instance = NULL;

InGameState::InGameState(const std::vector<Controller *> &controllers,
        const std::vector<Fighter*> &fighters, bool makeHazard) :
    controllers_(controllers),
    fighters_(fighters),
    paused_(false),
    pausingController_(-1)
{
    for (unsigned i = 0; i < fighters.size(); i++)
    {
        const float FIGHTER_SPAWN_Y = 50.f;

        Fighter *fighter = fighters[i];
        entities_.push_back(fighter);

        fighter->setRespawnLocation(
                0-225.0f+i*150,
                FIGHTER_SPAWN_Y); 
        fighter->respawn(false);

    }
    // Need this assertion for destructor
    assert(entities_.size() == fighters_.size());

    if (makeHazard)
    {
        HazardEntity *h = new HazardEntity("groundhit");
        entities_.push_back(h);
    }

    // Clear the stats
    StatsManager::get()->clear();

    // Choose random song
    std::vector<std::string> songs;
    songs.push_back("sfx/03 GeoSMASH (loop).ogg");
    songs.push_back("sfx/04 Horrors of the Hidden Levels (loop).ogg");
    songs.push_back("sfx/05 Pharticle Pysics (loop).ogg");
    songs.push_back("sfx/06 Meat DeFeat (loop).ogg");
    songs.push_back("sfx/07 Pixel Party (loop).ogg");
    AudioManager::get()->setSoundtrack(songs[rand() % songs.size()]);
    AudioManager::get()->startSoundtrack();

    glm::vec3 cameraLoc(0.f, 0.f, 425.0f);
    setCamera(cameraLoc);
    CameraManager::get()->setCurrent(cameraLoc);

    // Start of match time
    startTime_ = SDL_GetTicks();

    instance = this;
}

InGameState::~InGameState()
{
    for (size_t i = 0; i < controllers_.size(); i++)
        delete controllers_[i];
    // Delete all non fighter entities, assume the fighters at front of entities list
    for (size_t i = fighters_.size(); i < entities_.size(); i++)
        delete entities_[i];
}

GameState * InGameState::processInput(const std::vector<SDL_Joystick*> &joysticks, float dt)
{
    // Check for player one pressing back (id 6) while paused
    if (SDL_JoystickGetButton(joysticks[0], 6)
            && paused_ && pausingController_ == 0)
    {
        return new StatsGameState(fighters_, -1);
    }

    // First update controllers / frame
    for (unsigned i = 0; i < controllers_.size(); i++)
    {
        controllers_[i]->update(dt);
    }

    // Check for pause toggle from controllers
    for (unsigned i = 0; i < controllers_.size(); i++)
    {
        if (controllers_[i]->wantsPauseToggle())
            togglePause(i);
    }


    // If paused, don't update fighters or ask for next state
    if (paused_)
        return NULL;

    // Now have the fighters process their input
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        controller_state cs = controllers_[i]->nextState();
        fighters_[i]->processInput(cs, dt);
    }

    // Check for state changes
    // change when only 1 or 0 teams left alive
    int alivePlayers = 0;
    int totalLives = 0;
    std::set<int> teamsAlive;
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        totalLives += fighters_[i]->getLives();
        if (fighters_[i]->isAlive())
        {
            alivePlayers++;
            teamsAlive.insert(fighters_[i]->getTeamID());
        }
    }

    // Check for switch to tense music
    if (alivePlayers == totalLives && !criticalMusic_)
    {
        criticalMusic_ = true;
        AudioManager::get()->setSoundtrack("sfx/08 Critical Stealth (loop).ogg");
        AudioManager::get()->startSoundtrack();
    }

    // End the game when zero or one team is left
    if (teamsAlive.size() < 2)
    {
        // Winning team is -1 if no team exists
        int winningTeam = -1;
        if (!teamsAlive.empty())
            winningTeam = *teamsAlive.begin();

        // Set the run time stat
        StatsManager::get()->setStat("MatchLength",
                (SDL_GetTicks() - startTime_) / 1000.0f);

        // Transition to end of game state
        return new StatsGameState(fighters_, winningTeam);
    }

    // No state change
    return NULL;
}

void InGameState::update(float dt)
{
    if (paused_)
        return;

    // Add new GameEntities
    std::vector<GameEntity *> newEntities = getEntitiesToAdd();
    entities_.insert(entities_.end(), newEntities.begin(), newEntities.end());

    // then remove done GameEntities
    std::vector<GameEntity *>::iterator it;
    for (it = entities_.begin(); it != entities_.end();)
    {
        if ((*it)->isDone())
        {
            delete *it;
            it = entities_.erase(it);
        }
        else
            it++;
    }

    // TODO: something fancy (or not so fancy) to make this dt smaller,
    // if necessary (if a velocity is over some threshold)
    integrate(dt);
    collisionDetection();

    AudioManager::get()->update(dt);
    CameraManager::get()->update(dt, fighters_);
}

void InGameState::render(float dt)
{
    // Make dt 0 if paused
    dt = dt * !paused_;

    // Draw the background
    StageManager::get()->renderSphereBackground(dt);
    StageManager::get()->renderStage(dt);

    // Draw all entities
    for (unsigned i = 0; i < entities_.size(); i++)
        entities_[i]->render(dt);

    // Draw the fighter arrows
    for (unsigned i = 0; i < fighters_.size(); i++)
        if (fighters_[i]->isAlive())
        {
            renderArrow(fighters_[i]);
        }

    // Draw any explosions
    ExplosionManager::get()->render(dt);
    ParticleManager::get()->render(dt);

    renderHUD();

    if (paused_)
        renderPause();
}

bool InGameState::stealLife(int teamID)
{
    for (size_t i = 0; i < fighters_.size(); i++)
    {
        if (fighters_[i]->getLives() > 1 && fighters_[i]->getTeamID() == teamID)
        {
            fighters_[i]->stealLife();
            return true;
        }
    }

    // no life found
    return false;
}

void InGameState::renderPause()
{
    glDisable(GL_DEPTH_TEST);
    glm::mat4 pmat = getProjectionMatrix();
    glm::mat4 vmat = getViewMatrix();
    setProjectionMatrix(glm::mat4(1.f));
    setViewMatrix(glm::mat4(1.f));

    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f)),
            glm::vec3(0.03f / 4, 0.04f / 4, 1.f));
    FrameManager::get()->renderFrame(transform,
            glm::vec4(0.6f, 0.6f, 0.6f, 0.3f), "PAUSED");
    //renderRectangle(transform, glm::vec4(0.1, 0.1, 0.1, 0.0));


    setProjectionMatrix(pmat);
    setViewMatrix(vmat);
}

void InGameState::renderHUD()
{
    // Render the overlay interface (HUD)
    glDisable(GL_DEPTH_TEST);
    glm::mat4 pmat = getProjectionMatrix();
    glm::mat4 vmat = getViewMatrix();
    setProjectionMatrix(glm::mat4(1.f));
    setViewMatrix(glm::mat4(1.f));

    const glm::vec2 hud_center(0, 1.5f/6 - 1);
    const glm::vec2 lifesize = 0.03f * glm::vec2(1.f, 16.f/9.f);
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        const glm::vec2 player_hud_center =
            hud_center + 0.3f * glm::vec2(i - 1.5f, 0.f);

        const Fighter *fighter = fighters_[i];
        glm::vec3 color = fighter->getColor();

        int lives = fighter->getLives();
        float damage = fighter->getDamage();
        // Draw life counts first
        if (lives < 5)
        {
            // If 4 or less lives then draw each live as a box individually
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
                renderRectangle(transform2, glm::vec4(color, 0.0f));

                if (j % 2 == 0)
                    life_area.x += lifesize.x * 1.5;
                else
                {
                    life_area.x -= lifesize.x * 1.5;
                    life_area.y -= lifesize.y * 1.5;
                }
            }
        }
        else
        {
            // If 5 or more lives, draw a single life box and a number
            glm::vec2 life_area = player_hud_center - 0.75f * glm::vec2(lifesize.x, 0);
            glm::mat4 transform = glm::scale(
                    glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(life_area, 0.f)),
                    glm::vec3(lifesize, 1.0f));
            renderRectangle(transform, glm::vec4(0.25f, 0.25f, 0.25f, 0.0f));
            transform = glm::scale(transform, glm::vec3(0.8, 0.8, 1.0f));
            renderRectangle(transform, glm::vec4(color, 0.0f));

            // Now draw the numbers
            life_area.x += lifesize.x * 1.5;
            transform = glm::scale( glm::translate( glm::mat4(1.0f), glm::vec3(life_area, 0.f)),
                    glm::vec3(1.5f*lifesize, 1.0f));
            FontManager::get()->renderNumber(transform, color, lives);
        }

        // Draw the damage amount with color scaled towards black as the
        // player has more damage
        glm::vec2 damageBarMidpoint = player_hud_center + glm::vec2(0.f, -0.8f/6.f);
        glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(damageBarMidpoint, 0.f)),
            glm::vec3(0.085f, 0.085f, 1.0f));
        glm::vec3 dmgColor = std::min(1.f, std::max(0.2f, 1.f - damage/200.f)) * color;
        FontManager::get()->renderNumber(transform, dmgColor, floorf(damage));
    }

    setProjectionMatrix(pmat);
    setViewMatrix(vmat);
}

void InGameState::renderArrow(const Fighter *f)
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

        //std::cout << "Arrow pos: " << arrowPos.x << ' ' << arrowPos.y << '\n';
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

void InGameState::integrate(float dt)
{
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        entities_[i]->update(dt);
    }
}

void InGameState::collisionDetection()
{
    // First check for hitbox collisions
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        GameEntity *entityi = entities_[i];
        if (!entityi->hasAttack()) continue;

        const Attack *attacki = entityi->getAttack();

        for (unsigned j = i + 1; j < entities_.size(); j++)
        {
            GameEntity *entityj = entities_[j];
            if (!entityj->hasAttack()) continue;

            const Attack *attackj = entityj->getAttack();

            if (attacki->getHitbox().overlaps(attackj->getHitbox()))
            {
                entityi->attackCollision(attackj);
                entityj->attackCollision(attacki);
                // Generate a small explosion to show cancelling
                rectangle hitboxi = attacki->getHitbox();
                rectangle hitboxj = attackj->getHitbox();
                float x = (hitboxi.x + hitboxj.x) / 2;
                float y = (hitboxi.y + hitboxj.y) / 2;
                ExplosionManager::get()->addExplosion(x, y, 0.1f);
            }
        }
    }

    // Now go through and check for hitboxes hitting game entities
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        GameEntity *attacker = entities_[i];
        if (!attacker->hasAttack()) continue;

        const Attack *attack = attacker->getAttack();

        for (unsigned j = 0; j < entities_.size(); j++)
        {
            // Don't check for hitting themself
            //if (i == j) continue;

            GameEntity *victim = entities_[j];
            // If the victim cannot be hit, just quit now
            if (!victim->canBeHit()) continue;

            // Hit occurs when there is overlap and attack can hit
            if (attack->getHitbox().overlaps(victim->getRect()) &&
                attack->canHit(victim))
            {
                // Let the attacker know, the handle the rest
                attacker->attackConnected(victim);
            }
        }
    }

    // Now check for ground hits
    rectangle ground = StageManager::get()->getGroundRect();
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        GameEntity *entity = entities_[i];
        entity->collisionWithGround(ground,
                entity->getRect().overlaps(ground));
    }

    // Figher specific checks here
    // Check for fighter death
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        Fighter *fighter = fighters_[i];
        if (!fighter->isAlive()) continue;

        // Respawn condition
        if (fighter->getRect().y < getParam("killbox.bottom") || fighter->getRect().y > getParam("killbox.top")
                || fighter->getRect().x < getParam("killbox.left") || fighter->getRect().x > getParam("killbox.right"))
        {
            std::string died = StatsManager::getPlayerName(fighter->getPlayerID());
            // Record the kill if it's not a self destruct
            if (fighter->getLastHitBy() != -1)
            {
                int killerID = fighter->getLastHitBy();
                std::string killer = StatsManager::getPlayerName(killerID);
                StatsManager::get()->addStat(killer+ ".kills." + died, 1);
                StatsManager::get()->addStat(killer+ ".kills.total", 1);
                // Update kill streak
                StatsManager::get()->addStat(killer + ".curKillStreak", 1); // cleared in fighter::respawn
                StatsManager::get()->maxStat(killer + ".maxKillStreak",
                        StatsManager::get()->getStat(killer + ".curKillStreak"));
            }
            else
                StatsManager::get()->addStat(died + ".suicides", 1);
            fighter->respawn(true);
            break;
        }
    }
}

void InGameState::togglePause(int controllerID)
{
    if (!paused_)
    {
        paused_ = true;
        pausingController_ = controllerID;
        AudioManager::get()->pauseSoundtrack();
        AudioManager::get()->playSound("pausein");
    }
    else if (paused_ && pausingController_ == controllerID)
    {
        paused_ = false;
        pausingController_ = -1;
        AudioManager::get()->startSoundtrack();
        AudioManager::get()->playSound("pauseout");
    }
}

// Functions and data structures for entity addition
static std::vector<GameEntity *> entitiesToAdd;

void addEntity(GameEntity *ent)
{
    entitiesToAdd.push_back(ent);
}

std::vector<GameEntity *> getEntitiesToAdd()
{
    std::vector<GameEntity *> ret = entitiesToAdd;
    entitiesToAdd.clear();
    return ret;
}
