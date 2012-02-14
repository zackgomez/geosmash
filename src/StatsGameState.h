#pragma once
#include "GameState.h"
#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>

class Fighter;
class Player;
struct rectangle;
struct controller_state;

class fighter_stat
{
public:
    fighter_stat(const std::string &sname, const std::string &dname);
    void render(const rectangle &rect) const;

private:
    std::string stat_name_;
    std::string display_name_;
};

class tab_pane
{
public:
    tab_pane() { }
    ~tab_pane();

    void render(const glm::vec2 &topleft, const glm::vec2 &size) const;

    void add_stat(fighter_stat* fs);

private:
    std::vector<fighter_stat*> stats_;
};

class tabbed_view
{
public:
    tabbed_view();
    ~tabbed_view();

    void add_tab(tab_pane *tab);
    void handle_input(const controller_state &cs);
    void render(const glm::vec2 &topleft, const glm::vec2 &size) const;
    size_t numTabs() const { return tabs_.size(); }

private:
    std::vector<tab_pane *> tabs_;
    int curtab_;
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

    std::vector<tabbed_view*> statTabs_;
    std::vector<int> curTabs_;

    std::vector<bool> ready_;

    GLuint backgroundTex_;
};

