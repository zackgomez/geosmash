#pragma once
#include <string>
#include <map>

class StatsManager
{
public:
    static StatsManager * get();

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

private:
    StatsManager();
    StatsManager(const StatsManager &);

    std::map<std::string, float> stats_;
};

std::string statPrefix(int playerID);
