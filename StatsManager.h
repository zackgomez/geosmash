#pragma once
#include <string>
#include <map>

class StatsManager
{
public:
    static StatsManager * get();

    void setStat(std::string stat, float val);
    void addStat(std::string stat, float delta);

    float getStat(std::string stat);

    void printStats() const;

private:
    StatsManager();
    StatsManager(const StatsManager &);

    std::map<std::string, float> stats_;
};

