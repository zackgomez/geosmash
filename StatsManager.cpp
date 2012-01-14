#include "StatsManager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "Fighter.h"

StatsManager::StatsManager()
{
    logger_ = Logger::getLogger("StatsManager");
}

StatsManager * StatsManager::get()
{
    static StatsManager sm;
    return &sm;
}

const std::string StatsManager::guest_user = "GUEST";

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
        while (!line.empty() && isspace(line[line.size() - 1]))
            line.erase(line.end() - 1);
        if (line.empty())
            continue;

        std::stringstream ss(line);
        lifetime_stats stats;

        std::string name;
        ss  >> stats.username
            >> stats.kills >> stats.deaths
            >> stats.games_played >> stats.games_won
            >> stats.damage_dealt >> stats.damage_taken >> stats.team_damage;

        if (!ss)
        {
            logger_->warning() << "Unable to parse stat file line: '" << line << "'\n";
            continue;
        }

        logger_->info() << "Read stats for user: '" << stats.username << "'\n";
        user_stats_[stats.username] = stats;
    }
}

std::vector<std::string> StatsManager::getUsernames() const
{
    std::vector<std::string> names;
    names.push_back(guest_user);
    std::map<std::string, lifetime_stats>::const_iterator it;
    for (it = user_stats_.begin(); it != user_stats_.end(); it++)
        names.push_back(it->first);

    return names;
}

void StatsManager::updateUserStats(const std::vector<Fighter*> fighters)
{
    for (size_t i = 0; i < fighters.size(); i++)
    {
        std::string username = fighters[i]->getUsername();
        // Don't keep stats for the guest user
        if (username == guest_user)
            continue;
        assert(user_stats_.find(username) == user_stats_.end());

        lifetime_stats cur = user_stats_[username];

        std::string prefix = statPrefix(fighters[i]->getPlayerID());

        cur.kills += getStat(prefix + ".kills");
        cur.deaths += getStat(prefix + ".deaths");
        cur.games_played += 1;
        cur.games_won += getStat("winningTeam") == fighters[i]->getTeamID() ? 1 : 0;
        cur.damage_dealt += getStat(prefix + ".damageGiven");
        cur.damage_taken += getStat(prefix + ".damageTaken");
        cur.team_damage += getStat(prefix + ".teamDamageGiven");
    }
}

void StatsManager::writeUserStats(const std::string &filename)
{
    std::ofstream ofile(filename.c_str());

    // Now write to a new file
    std::map<std::string, lifetime_stats>::const_iterator it;
    for (it = user_stats_.begin(); it != user_stats_.end(); it++)
    {
        lifetime_stats stats = it->second;
        ofile << stats.username << ' '
           << stats.kills << ' ' << stats.deaths << ' '
           << stats.games_played << ' ' << stats.games_won << ' '
           << stats.damage_dealt << ' ' << stats.damage_taken << ' ' << stats.team_damage
           << '\n';
    }
}

// FREE FUNCTIONS
std::string statPrefix(int playerID)
{
    return StatsManager::getStatPrefix(playerID);
}

