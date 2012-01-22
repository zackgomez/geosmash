#pragma once
#include "GameState.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Controller.h"

class MenuWidget
{
public:
    MenuWidget() : enabled_(true) { }
    virtual ~MenuWidget() { }

    virtual int value() const = 0;
    virtual void handleInput(float val) = 0;
    virtual void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const = 0;
    bool enabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

protected:
    bool enabled_;
};

class NumberSelectWidget : public MenuWidget
{
public:
    NumberSelectWidget(const std::string &name,
            int min, int max, int defval = 0);
    
    int value() const;
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

private:
    std::string name_;
    int min_, max_;
    int value_;
    bool primed_;
};


class StringSelectWidget : public MenuWidget
{
public:
    StringSelectWidget(const std::string &name,
            const std::vector<std::string> &strings, int defval = -1);
    
    int value() const;
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

    std::string strValue() const;

private:
    std::string name_;
    std::vector<std::string> values_;
    int idx_;
    bool primed_;
};

class PlayerWidget
{
public:
    explicit PlayerWidget(int playerID, const bool *teams, const bool *handicap);
    ~PlayerWidget();

    bool isActive() const;
    bool wantsStart() const;

    void processInput(Controller *controller, float dt);
    void render(const glm::vec2 &center, const glm::vec2 &size, float dt);

    int getTeamID() const;
    int getProfileID() const;
    int getColorID() const;
    glm::vec3 getColor(int colorScheme = 0) const;
    std::string getUsername() const;
    int getHandicapLives() const;

private:
    int playerID_;

    bool active_;
    bool wantsStart_;

    StringSelectWidget *usernameWidget_;
    StringSelectWidget *teamIDWidget_;
    NumberSelectWidget *handicapWidget_;

    int widgetIdx_;
    std::vector<MenuWidget*> widgets_;
    int colorIdx_;

    bool widgetChangePrimed_;

    const bool *teams_, *handicap_;
};

class MenuState : public GameState
{
public:
    MenuState();
    ~MenuState();

    virtual GameState* processInput(const std::vector<Controller*>&, float dt);
    virtual void update(float dt);
    virtual void render(float dt);
    
private:
    std::vector<PlayerWidget*> widgets_;
    std::vector<MenuWidget*> topWidgets_;

    bool teams_, handicap_;
    int topMenuController_;
    size_t topWidgetIdx_;

    bool topWidgetChangePrimed_;

    GameState* newGame(const std::vector<Controller*>&);
    void handleTopMenu(Controller *controller);
};

