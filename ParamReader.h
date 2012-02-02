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

    void printParams() const;

private:
    ParamReader();

    std::map<std::string, float> params_;
    LoggerPtr logger_;
};

float getParam(const std::string &param);
