#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

class ParamReader
{
public:
    static ParamReader *get()
    {
        static ParamReader pr;
        return &pr;
    }

    void loadFile(const char *filename)
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

    float get(const std::string &key) const
    {
        std::map<std::string, float>::const_iterator it = params_.find(key);
        if (it == params_.end())
        {
            std::cerr << "Unable to find key " << key << '\n';
            assert(false && "Key not found in params");
        }
        else
            return it->second;
    }

    bool hasParam(const std::string &key) const
    {
        return params_.find(key) != params_.end();
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

private:
    ParamReader() {}

    std::map<std::string, float> params_;
};

float getParam(const std::string &param);
