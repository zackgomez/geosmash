#pragma once
#include <fstream>
#include <vector>
#include "InGameState.h"

class Fighter;

class GhostAIRecorder : public GameListener
{
public:
    explicit GhostAIRecorder();
    ~GhostAIRecorder();

    void updateListener(const std::vector<Fighter*> &fighters);

    bool removeOnCompletion() const { return true; }

private:
    std::ofstream file_;
};

