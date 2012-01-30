#pragma once
#include <fstream>
#include <vector>
#include "InGameState.h"

class Fighter;

// TODO: make this a game listener

class GhostAIRecorder : public GameListener
{
public:
    explicit GhostAIRecorder(const std::string &filename);
    ~GhostAIRecorder();

    void update(const std::vector<Fighter*> &fighters);

private:
    std::ofstream file_;
};

