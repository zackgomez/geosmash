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
const std::string StatsManager::ai_user = "AI";
const std::string StatsManager::ghost_ai_user = "GHOST";

void StatsManager::setStat(const std::string &stat, float val)
{
    stats_[stat] = val;
}

void StatsManager::addStat(const std::string &stat, float delta)
{
    // If not there, just set val
    if (stats_.find(stat) == stats_.end())
        stats_[stat] = delta;
    else
        stats_[stat] += delta;
}

void StatsManager::maxStat(const std::string &stat, float val)
{
    // If not there, just set val
    if (stats_.find(stat) == stats_.end())
        stats_[stat] = val;
    // If the input is larger, set to input
    else if (val > stats_[stat])
        stats_[stat] = val;
}

float StatsManager::getStat(const std::string &stat) const
{
    const std::string lifetimePrefix = "lifetime.";
    // If the stat starts with "lifetime." then refer to lifetime stats
    if (stat.find(lifetimePrefix) == 0)
    {
        size_t nextDotInd = stat.find(".", lifetimePrefix.size()+1);
        const std::string username = stat.substr(lifetimePrefix.size()+1,
                nextDotInd);
        const std::string statname = stat.substr(nextDotInd+1);
                
        return getLifetimeStat(username, statname);
    }
    if (stats_.find(stat) == stats_.end())
        return 0.f;
    return stats_.find(stat)->second;
}

float StatsManager::getLifetimeStat(const std::string &username,
        const std::string &stat) const
{
    logger_->debug() << "Asked for lifetime stat '" << stat << "' for user " << username << '\n';
    return 0.f;
}

void StatsManager::printStats() const
{
    // Write the stats to the console
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
            >> stats.damage_dealt >> stats.damage_taken >> stats.team_damage
            >> stats.suicides >> stats.team_kills;

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
    names.push_back(ai_user);
    names.push_back(ghost_ai_user);
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
        // Don't keep stats for the guest/ai users
        if (username == guest_user || username == ai_user || username == ghost_ai_user)
            continue;
        assert(user_stats_.find(username) != user_stats_.end());

        lifetime_stats& cur = user_stats_[username];

        std::string prefix = statPrefix(fighters[i]->getPlayerID());

        cur.kills += getStat(prefix + "kills.total");
        cur.deaths += getStat(prefix + "deaths");
        cur.suicides += getStat(prefix + "suicides");
        cur.games_played += 1;
        cur.games_won += getStat("winningTeam") == fighters[i]->getTeamID() ? 1 : 0;
        cur.damage_dealt += getStat(prefix + "damageGiven");
        cur.damage_taken += getStat(prefix + "damageTaken");
        cur.team_damage += getStat(prefix + "teamDamageGiven");
        cur.team_kills += getStat(prefix + "kills.team");
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
           << ' ' << stats.suicides << ' ' << stats.team_kills
           << '\n';
    }
}

// FREE FUNCTIONS
std::string statPrefix(int playerID)
{
    return StatsManager::getStatPrefix(playerID);
}

