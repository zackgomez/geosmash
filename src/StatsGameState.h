#pragma once
#include "GameState.h"
#include <GL/glew.h>
#include <string>

class Fighter;
class Player;
struct rectangle;

struct fighter_stat
{
    std::string stat_name;
    std::string display_name;

    fighter_stat(const std::string &sname, const std::string &dname);
    void render(const rectangle &rect, int playerID);
};

class StatsGameState : public GameState
{
public:
    StatsGameState(const std::vector<Player*> players, int winningTeam);
    virtual ~StatsGameState();

    virtual GameState* processInput(const std::vector<Controller*> &controllers,
            float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    virtual void preFrame();
    virtual void postFrame();

private:
    std::vector<Player *>  players_;
    int winningTeam_;

    std::vector<fighter_stat *> stats_;
    std::vector<bool> ready_;

    GLuint backgroundTex_;
};

