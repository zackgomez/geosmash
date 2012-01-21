#include "Player.h"
#include "Controller.h"
#include "Fighter.h"
#include "StageManager.h"
#include "ParamReader.h"

Player::~Player()
{
    delete fighter_;
}

Player::Player(const Fighter *f) : fighter_(f) 
{ }

int Player::getPlayerID() const
{
    return fighter_->getPlayerID();
}

int Player::getTeamID() const
{
    return fighter_->getTeamID();
}

std::string Player::getUsername() const
{
    return fighter_->getUsername();
}

glm::vec3 Player::getColor() const
{
    return fighter_->getColor();
}

LocalPlayer::LocalPlayer(Controller *controller, const Fighter *f) :
    Player(f),
    controller_(controller)
{
}

LocalPlayer::~LocalPlayer()
{
    // No required deallocations
}

void LocalPlayer::update(float dt)
{
    // Controller is updated in main, don't need to do anything here
}

controller_state LocalPlayer::getState() const
{
    return controller_->getState();
}

bool LocalPlayer::wantsPauseToggle() const
{
    return fighter_->isAlive() && controller_->getState().pressstart;
}

//----------------------------
// AI Player

AIPlayer::AIPlayer(const Fighter *f) : Player(f) { }
controller_state AIPlayer::getState() const
{
    return cs_;
}

void AIPlayer::update(float)
{
    // Try to perform _both_ of these two actions
    // if Left/right of the stage midpoint, move to the center
    // If feet are below the top of the stage, attempt to jump

    memset(&cs_, 0 , sizeof(controller_state));

    rectangle r = StageManager::get()->getGroundRect();
    cs_.joyx = glm::sign(r.x - fighter_->getPosition().x);
    if (fighter_->getFrameName() == "LedgeGrab") 
    {
        cs_.joyxv = cs_.joyx * (getParam("input.velThresh") + 1);
    }
    if (fighter_->getPosition().y < r.y)
    {
        cs_.pressy = true;
    }

    // Attempt to side B when we're off the edge and yvel < 0
    if (fighter_->getFrameName() == "AirNormal") 
    {
        if (fighter_->getVelocity().y < 0 && 
            fabs(fighter_->getPosition().x - r.x) > r.w / 2 + 300)
        {
            cs_.pressb = true;
        }
        glm::vec2 upb_delta;
        float x = fighter_->getPosition().x;
        float dist = fabs(x - r.x);
        float ledgeDist = getParam("ledgeGrab.dist");
        upb_delta.x = getParam("upSpecialAttack.xvel") * 
            getParam("upSpecialAttack.duration");
        upb_delta.y = getParam("upSpecialAttack.yvel") *
            getParam("upSpecialAttack.duration");
        if (dist > r.w / 2 && dist < r.w / 2 + ledgeDist + upb_delta.x)
        {
            // Pretty good condition for Up-B
            cs_.joyy = 1;
            cs_.joyx *= 0.99;
            cs_.pressb = 1;
        }

    }

}

