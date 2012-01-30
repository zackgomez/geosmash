#include "GhostAIRecorder.h"
#include "Fighter.h"
#include <ctime>
#include <sstream>
#include <iostream>

GhostAIRecorder::GhostAIRecorder()
{
    // Create a string "ghostdata.timestamp"
    char buf[100];
    std::stringstream ss;
    time_t curtime = time(NULL);
    strftime(buf, sizeof(buf), "%Y%m%d%M%S", localtime(&curtime));
    ss << "ghostdata." << buf;

    std::cout << "Recording ghost data to " << ss.str() << '\n';

    file_.open(ss.str().c_str());
}

GhostAIRecorder::~GhostAIRecorder()
{
    file_.close();
}

void GhostAIRecorder::update(const std::vector<Fighter*> &fighters)
{
    for (size_t i = 0; i < fighters.size(); i++)
    {
        Fighter *f = fighters[i];

        file_ << "PID: " << f->getPlayerID() 
            << "  Pos: " << f->getPosition().x << ' ' << f->getPosition().y
            << "  Vel: "  << f->getVelocity().x << ' ' << f->getVelocity().y
            << "  FName: " << f->getFrameName()
            << "  Dmg: " << f->getDamage()
            << "  hbox: " << f->hasAttack()
            << "  dir: " << f->getDirection()
            << '\n';
    }
}
