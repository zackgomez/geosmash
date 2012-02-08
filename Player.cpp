#include "Player.h"
#include "Controller.h"
#include "Fighter.h"
#include "StageManager.h"
#include "InGameState.h"
#include "ParamReader.h"

Player::~Player()
{
    delete fighter_;
}

Player::Player(const Fighter *f) : fighter_(f) 
{ }

int Player::getPlayerID() const
{
    return fighter_->getPlayerID();
}

int Player::getTeamID() const
{
    return fighter_->getTeamID();
}

std::string Player::getUsername() const
{
    return fighter_->getUsername();
}

glm::vec3 Player::getColor() const
{
    return fighter_->getColor();
}

LocalPlayer::LocalPlayer(Controller *controller, const Fighter *f) :
    Player(f),
    controller_(controller)
{
}

LocalPlayer::~LocalPlayer()
{
    // No required deallocations
}

void LocalPlayer::update(float dt)
{
    // Controller is updated in main, don't need to do anything here
}

controller_state LocalPlayer::getState() const
{
    return controller_->getState();
}

bool LocalPlayer::wantsPauseToggle() const
{
    return fighter_->isAlive() && controller_->getState().pressstart;
}

bool LocalPlayer::wantsLifeSteal() const
{
    return !fighter_->isAlive() && controller_->getState().pressstart;
}

void LocalPlayer::updateListener(const std::vector<Fighter *> &fighters)
{
    // Do nothing
}

//----------------------------
// AI Player
//----------------------------

AIPlayer::AIPlayer(const Fighter *f) : Player(f) { }
controller_state AIPlayer::getState() const
{
    return cs_;
}

void AIPlayer::performGetBack()
{
    
    rectangle r = StageManager::get()->getGroundRect();
    cs_.joyx = glm::sign(r.x - pos.x); 
    // Attempt to side B when we're off the edge and yvel < 0
    if (fighter_->getFrameName() == "AirNormal") 
    {
        glm::vec2 upb_delta;
        // How far from the center of the stage are we?
        float dist = fabs(pos.x - r.x);
        float ledgeDist = getParam("ledgeGrab.dist");
        upb_delta.x = getParam("upSpecialAttack.xvel") * 
            getParam("upSpecialAttack.duration");
        upb_delta.y = getParam("upSpecialAttack.yvel") *
            getParam("upSpecialAttack.duration");
        
   
        if (fighter_->getVelocity().y < 0 && dist > r.w / 2 + 400)
        {
            cs_.pressb = true;
        }
        if (dist > r.w / 2 && dist < r.w / 2 + ledgeDist + upb_delta.x)
        {
            // Pretty good condition for Up-B
            cs_.joyy = 1;
            cs_.joyx *= 0.99;
            cs_.pressb = 1;
        }

    }
}

void AIPlayer::performAttack()
{
    rectangle r = StageManager::get()->getGroundRect();
    cs_.joyx = glm::sign(targetPos.x - pos.x); 
    float dist = fabs(pos.x - r.x);
    if (dist < r.w / 2)
    {
        if (danger) {
            cs_.joyx = 0;
            cs_.pressa = true;
        }
        else {
            cs_.pressb = true;
            cs_.joyx = 0;
        }
    }

}

void AIPlayer::update(float)
{
    // Try to perform _both_ of these two actions
    // if Left/right of the stage midpoint, move to the center
    // If feet are below the top of the stage, attempt to jump

    memset(&cs_, 0 , sizeof(controller_state));

    // First, update some variables. Nothing here should modify cs_.
    pos = fighter_->getPosition();
    rectangle r = StageManager::get()->getGroundRect(); 
    float dist = fabs(pos.x - r.x);

    // If we're flying off, do nothing except get back.
    if (dist > r.w / 2) 
    {
        performGetBack();
        return;
    }
    if (fighter_->getFrameName() == "LedgeGrab") 
    {
        cs_.joyxv = cs_.joyx * (getParam("input.velThresh") + 1);
    }
    // have the guy follow you around, if the target is on the platform
    if (fabs(targetPos.x - pos.x) < r.w / 2) 
    {
        cs_.joyx = glm::sign(targetPos.x - fighter_->getPosition().x);
    }
    if (fighter_->getPosition().y < r.y)
    {
        cs_.pressy = true;
    }

    // We want to spend as little time as possible in the ground state
    if (fighter_->getFrameName() == "GroundNormal")
    {
        // short hop
        cs_.pressy = true;
        return;
    }
    // if we're not in danger of falling off and someone is near us,
    // just attack
    if (fighter_->getFrameName() == "AirNormal")
    {
        performAttack();
    }
}


void AIPlayer::updateListener(const std::vector<Fighter *> &fs)
{
    danger = false;
    for (unsigned int i = 0; i < fs.size(); i++) 
    {
        if (fighter_ == fs[i]) continue;
        if (fabs(pos.x - fs[i]->getPosition().x) < 200)
        {
            danger = true;
        }
    }

    for (unsigned int i = 0; i < fs.size(); i++) 
    {
        if (fighter_ == fs[i]) continue;
        float dist = fabs(pos.x - fs[i]->getPosition().x);
        targetPos = fs[i]->getPosition();
    }
}

//----------------------------
// Ghost AI Player
//----------------------------

GhostAIPlayer::GhostAIPlayer(const Fighter *f) :
    Player(f)
{
    logger_ = Logger::getLogger("GhostAIPlayer");
    cs_.clear();

    // Load the action frames
    std::fstream actionfile("trainingdata/actionframes.txt");
    assert(actionfile);
    std::string line;
    while (std::getline(actionfile, line))
    {
        logger_->debug() << "Adding " << line << " as an action state.\n";
        actionFrames_.insert(line);
    }

    // TODO load the Case Base
    std::fstream casefile("casebase");
    readCaseBase(casefile);
}

GhostAIPlayer::~GhostAIPlayer()
{
}

controller_state GhostAIPlayer::getState() const
{
    return cs_;
}

void GhostAIPlayer::update(float dt)
{
    const std::string curframe = fighter_->getFrameName();
    // Only do updates when we're in an state to perform an action
    if (actionFrames_.count(curframe))
    {
        logger_->debug() << "Looking for state similar to: " << cgs_ << '\n';
        // Search for the closest game state we've seen before

        // Set controller state according to action
        cs_.clear();
    }
}

void GhostAIPlayer::updateListener(const std::vector<Fighter *> &fighters)
{
    assert(fighters.size() == 2);

    Fighter *other = fighters[!fighter_->getPlayerID()];

    CasePlayerState me = fighter2cps(fighter_);
    CasePlayerState enemy = fighter2cps(other);

    cgs_ = cps2cgs(me, enemy);
}

void GhostAIPlayer::readCaseBase(std::istream &is)
{
    CasePlayerState me, enemy;
    CaseGameState cgs;

    std::string line;
    while (std::getline(is, line))
    {
        if (line.empty())
            continue;

        // Then it is an action
        if (line.find("->") != std::string::npos)
        {
            std::stringstream ss(line);
            std::string prevState, nextState, crap;
            ss >> prevState >> crap >> nextState; assert(crap == "->");
            assert(prevState == me.fname);

            // Compute CaseGameState from player states
            CaseGameState cgs = cps2cgs(me, enemy);
            caseBase_[cgs] = nextState;

            logger_->debug() << "Recorded case/action: " << cgs << " // " << nextState << '\n';
        }
        // Some sort of state
        else
        {
            CasePlayerState cps = readCPS(line);
            if (cps.playerid == 0)
                me = cps;
            else if (cps.playerid == 1)
                enemy = cps;
            else
                assert(false && "playerid not recognized");
        }
    }
};

CasePlayerState GhostAIPlayer::fighter2cps(const Fighter *f)
{
    CasePlayerState cps;

    cps.playerid = f->getPlayerID();
    cps.pos = f->getPosition();
    cps.vel = f->getVelocity();
    cps.fname = f->getFrameName();
    cps.damage = f->getDamage();
    cps.hasAttack = f->hasAttack();
    cps.dir = f->getDirection();

    return cps;
}

CasePlayerState GhostAIPlayer::readCPS(const std::string &line)
{
    CasePlayerState cps;
    std::stringstream ss(line);
    std::string header;

    ss >> header >> cps.playerid; assert(header == "PID:");
    ss >> header >> cps.pos.x >> cps.pos.y; assert(header == "Pos:");
    ss >> header >> cps.vel.x >> cps.vel.y; assert(header == "Vel:");
    ss >> header >> cps.fname; assert(header == "FName:");
    ss >> header >> cps.damage; assert(header == "Dmg:");
    ss >> header >> cps.hasAttack; assert(header == "hbox:");
    ss >> header >> cps.dir; assert(header == "dir:");

    assert(ss);

    return cps;
}

CaseGameState GhostAIPlayer::cps2cgs(const CasePlayerState &me,
        const CasePlayerState &enemy)
{
    CaseGameState cgs;

    cgs.abspos = me.pos;
    cgs.relpos = enemy.pos - me.pos;
    cgs.relvel = enemy.vel - me.vel;
    cgs.myframe = me.fname;
    cgs.facing = glm::sign(cgs.relpos.x) == me.dir;
    cgs.enemydamage = enemy.damage;
    cgs.enemyhitbox = enemy.hasAttack;

    const std::string enemyFrame = enemy.fname;
    cgs.enemyvulnerable = enemyFrame != "Blocking" && enemyFrame != "GroundRoll"
        && enemyFrame != "StepDodge";

    return cgs;
}

std::ostream & operator<<(std::ostream &os, const glm::vec2 &vec)
{
    os << '[' << vec.x << ' ' << vec.y << ']';
    return os;
}

std::ostream & operator<<(std::ostream &os, const CaseGameState &cgs)
{
    os  << cgs.abspos << ' ' << cgs.relpos << ' ' << cgs.relvel << ' '
        << cgs.myframe << ' ' << cgs.facing << ' ' << cgs.enemydamage << ' '
        << cgs.enemyhitbox << ' ' << cgs.enemyvulnerable;

    return os;
}

bool CaseGameState::operator<(const CaseGameState &rhs) const
{
    return abspos.x < rhs.abspos.x;
}
