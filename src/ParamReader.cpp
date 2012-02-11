#include "ParamReader.h"
#include <cassert>

ParamReader::ParamReader()
{
    logger_ = Logger::getLogger("ParamReader");
}

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
        logger_->fatal() << "UNABLE TO OPEN " << filename << " to read params\n";
        assert(false);
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
            logger_->fatal() << "Overwritting param " << key << '\n';
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
        logger_->fatal() << "Unable to find key " << key << '\n';
        assert(false && "Key not found in params");
    }
    else
        return it->second;
}

void ParamReader::setParam(const std::string &key, float value)
{
    params_[key] = value;
}

bool ParamReader::hasParam(const std::string &key) const
{
    return params_.find(key) != params_.end();
}

void ParamReader::printParams() const
{
    logger_->info() << "Params:\n";
    std::map<std::string, float>::const_iterator it;
    for (it = params_.begin(); it != params_.end(); it++)
    {
        logger_->info() << it->first << ' ' << it->second << '\n';
    }
}

float getParam(const std::string &param)
{
    return ParamReader::get()->get(param);
}

