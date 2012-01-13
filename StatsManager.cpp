#include "StatsManager.h"
#include <iostream>
#include <sstream>
#include <fstream>

StatsManager::StatsManager()
{
    logger_ = Logger::getLogger("StatsManager");
}

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

void StatsManager::readUserFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        if (isspace(line[line.size() - 1]))
            line.erase(line.end() - 1);
        if (line.empty())
            continue;

        std::stringstream ss(line);
        lifetime_stats stats;

        std::string name;
        ss  >> stats.username
            >> stats.kills >> stats.deaths
            >> stats.games_played >> stats.games_won
            >> stats.damage_dealt >> stats.team_damage;

        if (!ss)
        {
            logger_->warning() << "Unable to parse stat file line: '" << line << "'\n";
            continue;
        }

        user_stats_.push_back(stats);
    }
}

std::vector<std::string> StatsManager::getUsernames() const
{
    std::vector<std::string> names;
    names.push_back("GUEST");
    for (size_t i = 0; i < user_stats_.size(); i++)
        names.push_back(user_stats_[i].username);

    return names;
}

// FREE FUNCTIONS
std::string statPrefix(int playerID)
{
    return StatsManager::getStatPrefix(playerID);
}
