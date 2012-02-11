#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Logger.h"

class ParamReader
{
public:
    static ParamReader *get();

    void loadFile(const char *filename);

    float get(const std::string &key) const;

    bool hasParam(const std::string &key) const;

    // Sets a given parameter, overwrites if the param already exits
    void setParam(const std::string &key, float value);

    void printParams() const;

private:
    ParamReader();

    std::map<std::string, float> params_;
    LoggerPtr logger_;
};

float getParam(const std::string &param);
