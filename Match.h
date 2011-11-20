#pragma once

class Match
{
public:
    Match(int players, bool teams, bool music);
    ~Match();

    void start();

private:
    bool paused, teams, running;
    bool muteMusic, criticalMusic;

    int numPlayers;
    int pausedPlayer;


    // High level process functions
    void mainLoop();
    void processInput();
    void update();
    void render();

    void renderArrow(const Fighter *fighter);
    void renderHUD();

    void pause(int playerID);
    void unpause(int playerID);

    // Controller functions
    void updateController(Controller &);
    void controllerEvent(Controller &, const SDL_Event &);


    // Initialization/Deconstruction functions
    void initAudio();
    void initGraphics();
};
