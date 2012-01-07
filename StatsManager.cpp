#include "StatsManager.h"
#include <iostream>
#include <sstream>

StatsManager::StatsManager()
{ /* Empty */ }

StatsManager * StatsManager::get()
{
    static StatsManager sm;
    return &sm;
}

void StatsManager::setStat(std::string stat, float val)
{
    stats_[stat] = val;
}

void StatsManager::addStat(std::string stat, float delta)
{
    // If not there, just set val
    if (stats_.find(stat) == stats_.end())
        stats_[stat] = delta;
    else
        stats_[stat] += delta;
}

void StatsManager::maxStat(std::string stat, float val)
{
    // If not there, just set val
    if (stats_.find(stat) == stats_.end())
        stats_[stat] = val;
    // If the input is larger, set to input
    else if (val > stats_[stat])
        stats_[stat] = val;
}

float StatsManager::getStat(std::string stat)
{
    if (stats_.find(stat) == stats_.end())
        return 0.f;
    return stats_[stat];
}

void StatsManager::printStats() const
{
    std::cout << " =========\n"
              << " = STATS =\n"
              << " =========\n";

    std::map<std::string, float>::const_iterator it;
    for (it = stats_.begin(); it != stats_.end(); it++)
    {
        std::cout << it->first << ": " << it->second << '\n';
    }
}

void StatsManager::clear()
{
    stats_.clear();
}

std::string StatsManager::getPlayerName(int playerID)
{
    std::stringstream ss;
    ss << "Player" << playerID;
    return ss.str();
}

std::string StatsManager::getStatPrefix(int playerID)
{
    return getPlayerName(playerID) + '.';
}

std::string statPrefix(int playerID)
{
    return StatsManager::getStatPrefix(playerID);
}
