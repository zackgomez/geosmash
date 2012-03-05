#define _USE_MATH_DEFINES
// TODO clean up these includes for the love of god
#include "InGameState.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <cassert>
#include "Engine.h"
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
#include "StatsGameState.h"
#include "Player.h"
#include "GhostAIRecorder.h"

std::vector<GameEntity *> getEntitiesToAdd();
void addEntity(GameEntity *ent);

const std::vector<const Fighter*> InGameState::getFighters() const
{
    std::vector<const Fighter*>ans(fighters_.begin(), fighters_.end());
    return ans;
}

InGameState::InGameState(const std::vector<Player *> &players,
        const std::vector<Fighter*> &fighters, bool keepStats,
        const std::string &stage, GameMode *gameMode) :
    players_(players),
    fighters_(fighters),
    gameMode_(gameMode),
    paused_(false),
    pausingPlayer_(-1),
    keepStats_(keepStats)
{
    StageManager::get()->clear();
    ParticleManager::get()->reset();

    logger_ = Logger::getLogger("InGameState");

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
    assert(fighters_.size() == players_.size());

    // Set up the level
    StageManager::get()->initLevel(stage);
    // Clear the stats
    StatsManager::get()->clear();

    // Set per game stats
    StatsManager::get()->setStat("numPlayers", players_.size());

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
    StatsManager::get()->setStat("startMillis", getCurrentMillis());

    // Add a ghost ai learning listener if specified
    if (getParam("debug.saveghost"))
        listeners_.push_back(new GhostAIRecorder());
    // Add players as listeners
    for (size_t i = 0; i < players_.size(); i++)
        listeners_.push_back(players_[i]);

    std::string replayName = "replays/lastreplay.";
    replayName += getTimeString();
    replayStream_.open(replayName.c_str(), std::ostream::out);
    // Print out the player names for the first line
    for (size_t i = 0; i < players_.size(); i++)
        replayStream_ << players_[i]->getUsername() << ' ';
    replayStream_ << '\n';
    logger_->info() << "Saving replay to " << replayName << '\n';
    assert(replayStream_ && "Couldn't open replay file does replays/ exist?");
}

InGameState::~InGameState()
{
    // Delete all non fighter entities, assume the fighters at front of entities list
    for (size_t i = fighters_.size(); i < entities_.size(); i++)
        delete entities_[i];

    // Fighters/players are passed to StatsGameState and don't need to be destructed

    // Clean up the listeners that want to be cleaned up
    for (size_t i = 0; i < listeners_.size(); i++)
        if (listeners_[i]->removeOnCompletion())
            delete listeners_[i];

    delete gameMode_;

    replayStream_.close();
}

GameState * InGameState::processInput(const std::vector<Controller*> &controllers, float dt)
{
    // Before we do anything, call preframe listener functions
    for (size_t i = 0; i < listeners_.size(); i++)
        listeners_[i]->updateListener(fighters_);

    // First update players pre frame
    for (size_t i = 0; i < players_.size(); i++)
        players_[i]->update(dt);

    // Check for pause toggle from controllers/pause early exit (pause+BACK)
    for (unsigned i = 0; i < players_.size(); i++)
    {
        if (players_[i]->wantsPauseToggle())
            togglePause(i);

        if (paused_ && pausingPlayer_ == i &&
                players_[i]->getState().pressback)
        {
            // Set the run time stat
            StatsManager::get()->setStat("MatchLength",
                    (getCurrentMillis() - StatsManager::get()->getStat("startMillis")) / 1000.0f);
            // Set the places to indicate NO CONTEST
            for (size_t i = 0; i < players_.size(); i++)
                StatsManager::get()->setStat(
                        StatsManager::getStatPrefix(players_[i]->getPlayerID()) + "place", players_.size());
            return new StatsGameState(players_, -1);
        }
    }


    // If paused, don't update fighters or ask for next state
    if (paused_)
        return NULL;
    // Output replay information
    for (size_t i = 0; i < players_.size(); i++)
    {
        const controller_state cs = players_[i]->getState();
        replayStream_ << '[' << i << "] "
            << cs.joyx << ' ' << cs.joyxv << ' '
            << cs.joyy << ' ' << cs.joyyv << ' '
            << cs.pressa << ' ' << cs.buttona << ' '
            << cs.pressb << ' ' << cs.buttonb << ' '
            << cs.pressx << ' ' << cs.buttonx << ' '
            << cs.pressy << ' ' << cs.buttony << ' '
            << cs.rtrigger << ' ' << cs.ltrigger << ' '
            << cs.presslb << ' ' << cs.lbumper << ' '
            << cs.pressrb << ' ' << cs.rbumper << ' '
            << cs.dpadl << ' ' << cs.dpadr << ' ' << cs.dpadu << ' ' << cs.dpadd << '\n';
    }

    // Check for life steal action
    for (size_t i = 0; i < players_.size(); i++)
    {
        if (players_[i]->wantsLifeSteal())
            if (stealLife(players_[i]->getTeamID()))
                fighters_[i]->addLives(1);
    }

    // Now have the fighters process their input
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        controller_state cs = players_[i]->getState();
        fighters_[i]->processInput(cs, dt);
    }

    // Check for state changes
    gameMode_->update(dt, fighters_);
    if (gameMode_->gameOver())
    {
        // Winning team is -1 if no team exists
        int winningTeam = gameMode_->getWinningTeam();

        // Set the run time stat
        StatsManager::get()->setStat("MatchLength",
                (getCurrentMillis() - StatsManager::get()->getStat("startMillis")) / 1000.0f);
        // Set the places of people yet to die
        for (size_t i = 0; i < players_.size(); i++)
            StatsManager::get()->maxStat(
                    StatsManager::getStatPrefix(players_[i]->getPlayerID()) + "place", 1.f);

        // Transition to end of game state
        StatsManager::get()->setStat("winningTeam", winningTeam);
        if (keepStats_)
        {
            StatsManager::get()->updateUserStats(fighters_);
            StatsManager::get()->writeUserStats("user_stats.dat");
        }
        return new StatsGameState(players_, winningTeam);
    }

    // No state change
    return NULL;
}

void InGameState::update(float dt)
{
    if (paused_)
        return;

    // Update level
    StageManager::get()->update(dt);

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
    collisionDetection(dt);

    AudioManager::get()->update(dt);
    CameraManager::get()->update(dt, fighters_);
}

void InGameState::render(float dt)
{
    // Make dt 0 if paused
    dt = dt * !paused_;


    // Draw the background
    StageManager::get()->renderBackground(dt);
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
    ParticleManager::get()->startUpdate(dt);
    ParticleManager::get()->update();
    ParticleManager::get()->render(dt);

    renderHUD();

    if (paused_)
        renderPause();
}

void InGameState::preFrame()
{
    // nop
}

void InGameState::postFrame()
{
    // nop
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
    getProjectionMatrixStack().push();
    getViewMatrixStack().push();
    getProjectionMatrixStack().current() = glm::mat4(1.f);
    getViewMatrixStack().current() = glm::mat4(1.f);

    glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f)),
            glm::vec3(0.03f / 4, 0.04f / 4, 1.f));
    FrameManager::get()->renderFrame(transform,
            glm::vec4(0.6f, 0.6f, 0.6f, 0.3f), "PAUSED");
    //renderRectangle(transform, glm::vec4(0.1, 0.1, 0.1, 0.0));


    getProjectionMatrixStack().pop();
    getViewMatrixStack().pop();
}

void InGameState::renderHUD()
{
    // Render the overlay interface (HUD)
    glDisable(GL_DEPTH_TEST);
    getProjectionMatrixStack().push();
    getViewMatrixStack().push();
    getProjectionMatrixStack().current() = glm::mat4(1.f);
    getViewMatrixStack().current() = glm::mat4(1.f);

    const glm::vec2 hud_center(0, 1.5f/6 - 1);
    const glm::vec2 lifesize = 0.03f * glm::vec2(1.f, 16.f/9.f);
    const glm::vec3 danger_color = glm::vec3(0.8, 0.1, 0.1);
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

        // Draw the damage amount, if the player is over 150% instead render in dange red
        glm::vec2 damageBarMidpoint = player_hud_center + glm::vec2(0.f, -0.8f/6.f);
        glm::mat4 transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(damageBarMidpoint, 0.f)),
            glm::vec3(0.085f, 0.085f, 1.0f));
        FontManager::get()->renderNumber(transform,
                damage > 150.f ? danger_color : color, floorf(damage));

        // Draw the player profile name
        glm::vec2 profileNameMidpoint = player_hud_center + glm::vec2(0.f, -1.2f/6.f);
        transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(profileNameMidpoint, 0.f)),
            glm::vec3(0.045f, 0.045f, 1.0f));
        FontManager::get()->renderString(transform, color, fighter->getUsername());
    }

    getProjectionMatrixStack().pop();
    getViewMatrixStack().pop();
}

void InGameState::renderArrow(const Fighter *f)
{
    glm::vec4 fpos = glm::vec4(f->getRect().x, f->getRect().y, 0.f, 1.f);
    glm::vec4 fndc = getProjectionMatrixStack().current() * getViewMatrixStack().current() * fpos;
    fndc /= fndc.w;
    if (fabs(fndc.x) > 1 || fabs(fndc.y) > 1)
    {
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
        //
        glm::mat4 transform =
            glm::rotate(glm::scale(glm::translate(glm::mat4(1.0f),
                            glm::vec3(arrowPos, 0.0f)), 
                        glm::vec3(scale, scale, 1.f)),
                    rot, glm::vec3(0, 0, 1));

        getProjectionMatrixStack().push();
        getViewMatrixStack().push();
        getProjectionMatrixStack().current() = glm::mat4(1.f);
        getViewMatrixStack().current() = glm::mat4(1.f);

        FrameManager::get()->renderFrame(transform, glm::vec4(f->getColor(), 0.0f), "OffscreenArrow");

        getProjectionMatrixStack().pop();
        getViewMatrixStack().pop();
    }
}

void InGameState::integrate(float dt)
{
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        entities_[i]->update(dt);
    }
}

void InGameState::collisionDetection(float dt)
{
    // First check for ground and platform hits
    rectangle ground = StageManager::get()->getGroundRect();
    std::vector<rectangle> platforms = StageManager::get()->getPlatforms();
    // For each entity, go until you find a collision, if not, tell them no
    // collision with ground
    for (unsigned i = 0; i < entities_.size(); i++)
    {
        GameEntity *entity = entities_[i];
        if (entity->getRect().overlaps(ground))
        {
            entity->collisionWithGround(ground, true, false);
            continue;
        }

        size_t j;
        for (j = 0; j < platforms.size(); j++)
        {
            rectangle pf = platforms[j];
            glm::vec2 pos = entity->getPosition();
            glm::vec2 size = entity->getSize();
            glm::vec2 vel = entity->getVelocity();
            // Overlap, moving down, and was above the platform on the previous frame
            if (entity->getRect().overlaps(pf) &&
                vel.y <= 0 &&
                ((pos.y - vel.y * dt - size.y/2 > pf.y + pf.h/2) ||
                 (pos.y == pf.y + pf.h/2 + size.y/2 - 1)))
            {
                entity->collisionWithGround(pf, true, true);
                goto next_entity;
            }
        }
        // No collisions
        entity->collisionWithGround(ground, false, false);

next_entity:
        continue;
    }

    // Then check for hitbox-hitbox collisions
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

    // Figher specific checks here
    // Check for fighter death
    for (unsigned i = 0; i < fighters_.size(); i++)
    {
        Fighter *fighter = fighters_[i];
        if (!fighter->isAlive()) continue;

        // Respawn condition
        rectangle killbox = StageManager::get()->getKillBox();
        if (fighter->getRect().y < killbox.y - killbox.h / 2 
                || fighter->getRect().y > killbox.y + killbox.h / 2
                || fighter->getRect().x < killbox.x - killbox.w / 2
                || fighter->getRect().x > killbox.x + killbox.w / 2)
        {
            std::string died = StatsManager::getPlayerName(fighter->getPlayerID());
            // Record the kill if it's not a self destruct
            if (fighter->getLastHitBy() >= 0)
            {
                logger_->info() << "Last hit by: " << fighter->getLastHitBy() << std::endl;
                int killerID = fighter->getLastHitBy();
                std::string killer = StatsManager::getPlayerName(killerID);
                StatsManager::get()->addStat(killer+ ".kills." + died, 1);
                // check for team kill
                if (fighter->getTeamID() == getFighterByID(fighter->getLastHitBy())->getTeamID())
                    StatsManager::get()->addStat(killer+ ".kills.team", 1);
                else
                {
                    StatsManager::get()->addStat(killer+ ".kills.total", 1);
                    // Update kill streak
                    StatsManager::get()->addStat(killer + ".curKillStreak", 1); // cleared in fighter::respawn
                    StatsManager::get()->maxStat(killer + ".maxKillStreak",
                            StatsManager::get()->getStat(killer + ".curKillStreak"));
                }
            }
            else
                StatsManager::get()->addStat(died + ".suicides", 1);
            fighter->respawn(true);
            break;
        }
    }
}

Fighter* InGameState::getFighterByID(int playerID)
{
    for (size_t i = 0; i < fighters_.size(); i++)
        if (fighters_[i]->getPlayerID() == playerID)
            return fighters_[i];

    assert(false && "Fighter not found by ID");
}

void InGameState::togglePause(int playerID)
{
    if (!paused_)
    {
        paused_ = true;
        pausingPlayer_ = playerID;
        AudioManager::get()->pauseSoundtrack();
        AudioManager::get()->playSound("pausein");
    }
    else if (paused_ && pausingPlayer_ == playerID)
    {
        paused_ = false;
        pausingPlayer_ = -1;
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

// -----------------
// StockGameMode
// -----------------

StockGameMode::StockGameMode() :
    gameOver_(false), criticalMusic_(false), winningTeam_(-1)
{
}

StockGameMode::~StockGameMode()
{
}

void StockGameMode::update(float dt, const std::vector<Fighter *> &fighters)
{
    // Sanity check
    assert(!gameOver_);

    // Calculate number of players, teams, and total lives
    int alivePlayers = 0;
    int totalLives = 0;
    std::set<int> teamsAlive;
    for (unsigned i = 0; i < fighters.size(); i++)
    {
        totalLives += fighters[i]->getLives();
        if (fighters[i]->isAlive())
        {
            alivePlayers++;
            teamsAlive.insert(fighters[i]->getTeamID());
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
    if (teamsAlive.size() <= 1)
    {
        gameOver_ = true;
        // Winning team is -1 if no team exists
        winningTeam_ = -1;
        // Otherwise, only one team, and that team is winner
        if (!teamsAlive.empty())
            winningTeam_ = *teamsAlive.begin();
    }
}

bool StockGameMode::gameOver() const
{
    return gameOver_;
}

int StockGameMode::getWinningTeam() const
{
    assert(gameOver_);
    return winningTeam_;
}
