#pragma once
#include <string>
#include <glm/glm.hpp>

class Controller;
struct controller_state;
class Fighter;

class Player
{
public:
    virtual ~Player() { }

    // Gets the next state
    virtual controller_state getState() const = 0;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const = 0;

    // Does per frame player updates
    virtual void update(float dt) = 0;

    virtual int getPlayerID() const = 0;
    virtual int getTeamID() const = 0;
    virtual std::string getUsername() const = 0;
    virtual glm::vec3 getColor() const = 0;
};

class LocalPlayer : public Player
{
public:
    LocalPlayer(Controller *controller, const Fighter *f);
    virtual ~LocalPlayer();

    // Does per frame player updates
    virtual void update(float dt);

    // Gets the next state
    virtual controller_state getState() const;

    // Returns true if this player would like the game to be paused/unpaused
    virtual bool wantsPauseToggle() const;

    virtual int getPlayerID() const;
    virtual int getTeamID() const;
    virtual std::string getUsername() const;
    virtual glm::vec3 getColor() const;

private:
    Controller *controller_;
    const Fighter *fighter_;
};

