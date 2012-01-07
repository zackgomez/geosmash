#include "ParamReader.h"
#include <cassert>

ParamReader * ParamReader::get()
{
    static ParamReader pr;
    return &pr;
}

void ParamReader::loadFile(const char *filename)
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
        // Ignore comments that start with #
        if (!key.empty() && key[0] == '#')
            continue;
        if (ss.fail())
            continue;
        // Fail on double key read
        if (params_.find(key) != params_.end())
        {
            std::cerr << "FATAL ERROR: Overwritting param " << key << '\n';
            assert(false);
        }
        params_[key] = val;
    }
}

float ParamReader::get(const std::string &key) const
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

bool ParamReader::hasParam(const std::string &key) const
{
    return params_.find(key) != params_.end();
}

void ParamReader::printParams() const
{
    std::cout << "Params:\n";
    std::map<std::string, float>::const_iterator it;
    for (it = params_.begin(); it != params_.end(); it++)
    {
        std::cout << it->first << ' ' << it->second << '\n';
    }
}

float getParam(const std::string &param)
{
    return ParamReader::get()->get(param);
}

