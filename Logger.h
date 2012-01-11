#pragma once
#include <iostream>
#include <string>

class Logger;
typedef std::auto_ptr<Logger> LoggerPtr;

class Logger
{
public:
    static LoggerPtr getLogger(const std::string &prefix)
    {
        LoggerPtr ret(new Logger(prefix));
        return ret;
    }

    std::ostream& debug()
    {
        std::cout << "DEBUG - " << prefix_ << ": ";
        return std::cout;
    }
    std::ostream& info()
    {
        std::cout << "INFO - " << prefix_ << ": ";
        return std::cout;
    }
    std::ostream& warning()
    {
        std::cout << "WARNING - " << prefix_ << ": ";
        return std::cout;
    }
    std::ostream& error()
    {
        std::cout << "ERROR - " << prefix_ << ": ";
        return std::cout;
    }

private:
    Logger(const std::string &prefix) :
        prefix_(prefix)
    { /* Empty */ }

    std::string prefix_;
};

