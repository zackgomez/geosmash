#pragma once
#include "GameState.h"
#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>

class Fighter;
class Player;
struct rectangle;
struct controller_state;
class tabbed_view;

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

// WIDGETS

class pane_entry
{
public:
    virtual ~pane_entry() { }
    virtual void render(const rectangle &rect) const = 0;
};

class null_pane_entry : public pane_entry
{
public:
    virtual ~null_pane_entry() { }
    virtual void render(const rectangle &rect) const { }
};

class centered_text_entry : public pane_entry
{
public:
    centered_text_entry(const std::string &str);
    virtual ~centered_text_entry() { }

    virtual void render(const rectangle &rect) const;

private:
    std::string text_;
};

class fighter_stat : public pane_entry
{
public:
    fighter_stat(const std::string &sname, const std::string &dname,
            float normfact = 1.f);
    virtual ~fighter_stat() { }

    virtual void render(const rectangle &rect) const;

private:
    std::string stat_name_;
    std::string display_name_;
    float normfact_;
};

class tab_pane
{
public:
    tab_pane() { }
    ~tab_pane();

    void render(const glm::vec2 &topleft, const glm::vec2 &size) const;

    void add_entry(pane_entry* fs);

private:
    std::vector<pane_entry*> stats_;
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

