#pragma once
#include <string>
#include <map>
#include <vector>
#include "Logger.h"

class Fighter;

struct lifetime_stats
{
    std::string username;
    unsigned kills, deaths, suicides, team_kills;
    unsigned games_played, games_won;
    unsigned damage_dealt, damage_taken, team_damage;
};

class StatsManager
{
public:
    static StatsManager * get();
    static const std::string guest_user;
    static const std::string ai_user;
    static const std::string ghost_ai_user;

    // Sets the value of the passed state
    void setStat(std::string stat, float val);
    // Adds the delta value to the stat, if the stat doesn't exists,
    // sets the value to be delta.
    void addStat(std::string stat, float delta);
    // Makes the value of the stat be the maximum of the current stat and
    // the passed in stat.  If the stat does not exist, then the passed value
    // is set.
    void maxStat(std::string stat, float val);

    float getStat(std::string stat);

    void printStats() const;

    static std::string getStatPrefix(int playerID);
    static std::string getPlayerName(int playerID);

    // Removes all stats
    void clear();

    void readUserFile(const std::string &filename);
    std::vector<std::string> getUsernames() const;
    void updateUserStats(const std::vector<Fighter*> fighters);
    void writeUserStats(const std::string &filename);

private:
    StatsManager();
    StatsManager(const StatsManager &);

    std::map<std::string, float> stats_;
    std::map<std::string, lifetime_stats> user_stats_;

    LoggerPtr logger_;
};

std::string statPrefix(int playerID);

