#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

class ParamReader
{
public:
    ParamReader() {}
    ParamReader(const char *filename)
    {
        readFile(filename);
    }

    float get(const std::string &key, float def = 0.0f) const
    {
        std::map<std::string, float>::const_iterator it = params_.find(key);
        if (it == params_.end())
        {
            assert(false && "Key not found in params");
            return def;
        }
        else
            return it->second;
    }

private:
    std::map<std::string, float> params_;

    void readFile(const char *filename)
    {
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << "UNABLE TO OPEN " << filename << " to read params\n";
            return;
        }

        std::string key;
        float val;

        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            ss >> key >> val;
            if (ss.fail())
                continue;
            params_[key] = val;
        }
    }

    void printParams() const
    {
        std::cout << "Params:\n";
        std::map<std::string, float>::const_iterator it;
        for (it = params_.begin(); it != params_.end(); it++)
        {
            std::cout << it->first << ' ' << it->second << '\n';
        }
    }
};
