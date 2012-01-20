#include "Player.h"
#include "Controller.h"
#include "Fighter.h"

LocalPlayer::LocalPlayer(Controller *controller, const Fighter *f) :
    controller_(controller),
    fighter_(f)
{
}

LocalPlayer::~LocalPlayer()
{
    delete fighter_;
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

int LocalPlayer::getPlayerID() const
{
    return fighter_->getPlayerID();
}

int LocalPlayer::getTeamID() const
{
    return fighter_->getTeamID();
}

std::string LocalPlayer::getUsername() const
{
    return fighter_->getUsername();
}

glm::vec3 LocalPlayer::getColor() const
{
    return fighter_->getColor();
}
