#pragma once
#include "GameState.h"
#include <string>
#include <vector>
#include <set>
#include <glm/glm.hpp>
#include "Controller.h"
#include "Logger.h"

class Skeleton;
class GeosmashBoneRenderer;

class MenuWidget
{
public:
    MenuWidget() : enabled_(true) { }
    virtual ~MenuWidget() { }

    virtual void handleInput(float val) = 0;
    virtual void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const = 0;
    bool enabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

protected:
    bool enabled_;
};

class IntegerSelectWidget : public MenuWidget
{
public:
    IntegerSelectWidget(const std::string &name,
            const std::string &varname,
            int min, int max);
    
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

private:
    std::string name_, varname_;
    int min_, max_;
    bool primed_;
};


class StringSelectWidget : public MenuWidget
{
public:
    StringSelectWidget(const std::string &name,
            const std::string &varname,
            const std::vector<std::string> &strings);
    
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

private:
    std::string name_;
    std::string varname_;
    std::string idxname_;
    std::vector<std::string> strings_;
    bool primed_;
};

class BoolSelectWidget : public MenuWidget
{
public:
    BoolSelectWidget(const std::string &name, const std::string &varname);
    void handleInput(float val);
    void render(const glm::vec2 &center,
            const glm::vec2 &size, bool highlight) const;

private:
    std::string name_, varname_;
    bool primed_;
};

class PlayerWidget
{
public:
    explicit PlayerWidget(int playerID);
    ~PlayerWidget();

    bool isActive() const;

    void processInput(Controller *controller, float dt);
    void render(const glm::vec2 &center, const glm::vec2 &size, float dt);

private:
    void renderFighter(const glm::mat4 &transform, const glm::vec3 &color);

    int playerID_;
    std::string prefix_;

    StringSelectWidget *usernameWidget_;
    StringSelectWidget *teamIDWidget_;
    StringSelectWidget *fighterWidget_;
    IntegerSelectWidget *handicapWidget_;

    int widgetIdx_;
    std::vector<MenuWidget*> widgets_;

    bool widgetChangePrimed_;
};

class MenuState : public GameState
{
public:
    MenuState();
    ~MenuState();

    virtual GameState* processInput(const std::vector<Controller*>&, float dt);
    virtual void preFrame();
    virtual void update(float dt);
    virtual void render(float dt);
    virtual void postFrame();

    static glm::vec3 getPlayerColor(int playerID);
    static std::string getPlayerMenuPrefix(int playerID);
    
private:
    std::vector<PlayerWidget*> widgets_;
    std::vector<MenuWidget*> topWidgets_;

    int topMenuController_;
    size_t topWidgetIdx_;

    bool topWidgetChangePrimed_;

    LoggerPtr logger_;

    void handleTopMenu(Controller *controller);
};

