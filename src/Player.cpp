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

std::string Player::getFighterName() const
{
    return fighter_->getFighterName();
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
        float ledgeDist = fighter_->param("ledgeGrab.dist");
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

	cs_.clear();

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

    // We want to spend as little time as possible in the ground state
    if (fighter_->getFrameName() == "GroundWalking" ||
		fighter_->getFrameName() == "GroundNormal")
    {
        // short hop
        cs_.pressy = true;
        return;
    }
    // if we're not in danger of falling off and someone is near us,
    // just attack
    if (fighter_->getFrameName() == "AirNormalHop" || 
		fighter_->getFrameName() == "AirNormal")
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

    // load the Case Base
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

    cs_.clear();
    // Only do updates when we're in an state to perform an action
    if (actionFrames_.count(curframe))
    {
        logger_->debug() << "Looking for state similar to: " << cgs_ << '\n';
        // Search for the closest game state we've seen before
        const CaseAction action = getNextAction();
        
        cs_ = action.cs;//actionMap_.find(action.target)->second;
        // Set the appropriate x direction
        /*
        if (cs_.joyx >
            cs_.joyx *= glm::sign(cgs_.relpos.x * action.dir);
        cs_.joyxv *= glm::sign(cgs_.relpos.x * action.dir);
        logger_->debug() << "reldir " << fabs(cgs_.relpos.x) << " adir " << action.dir
            << " setdir " << cs_.joyx << '\n';
        */
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
        if (line.find("PID:") == std::string::npos)
        {
            CaseAction action;
            std::stringstream ss(line);

            controller_state cs;
            ss >> cs.joyx >> cs.joyxv >> cs.joyy >> cs.joyyv
               >> cs.pressa >> cs.buttona
               >> cs.pressb >> cs.buttonb
               >> cs.pressx >> cs.buttonx
               >> cs.pressy >> cs.buttony
               >> cs.rtrigger >> cs.ltrigger
               >> cs.presslb >> cs.lbumper
               >> cs.pressrb >> cs.rbumper
               >> cs.dpadl >> cs.dpadr >> cs.dpadu >> cs.dpadd;
            action.cs = cs;
            // TODO fix this

            std::cout << "Read controller state from line:\n" << line << '\n';

            // Compute CaseGameState from player states
            CaseGameState cgs = cps2cgs(me, enemy);
            caseBase_[cgs] = action;
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
}

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
    std::cout << "reading cps from '" << line << "'\n";
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

    // TODO make relpos l/r independent

    cgs.abspos = me.pos;
    cgs.relpos = enemy.pos - me.pos;
    cgs.relvel = enemy.vel - me.vel;
    cgs.myframe = me.fname;
    cgs.facing = glm::sign(cgs.relpos.x) == me.dir;
    cgs.enemydamage = enemy.damage;
    cgs.enemyhitbox = enemy.hasAttack;

    // TODO read from file for these vulnerable states
    const std::string enemyFrame = enemy.fname;
    cgs.enemyvulnerable = enemyFrame != "Blocking" && enemyFrame != "GroundRoll"
        && enemyFrame != "StepDodge";

    return cgs;
}

CaseAction GhostAIPlayer::getNextAction() const
{
    CaseAction action;
    action.cs.clear();
    CaseGameState best;
    float bestScore = -HUGE_VAL;
    const float scoreThresh = -HUGE_VAL;

    std::map<CaseGameState, CaseAction>::const_iterator it;
    for (it = caseBase_.begin(); it != caseBase_.end(); it++)
    {
        float curScore = caseHeuristic(cgs_, it->first);
        if (curScore > bestScore && curScore > scoreThresh)
        {
            best = it->first;
            action = it->second;
            bestScore = curScore;
        }
    }

    logger_->debug() << "Found best matching case (" << bestScore << "): " << best << '\n';
    logger_->debug() << "Calculated action: '" << action.cs << '\n';
    
    return action;
}

float GhostAIPlayer::caseHeuristic(const CaseGameState &cur, const CaseGameState &ref)
{
    const float posWeight = 1.0f;
    const float velWeight = 0.3f;//0.03f;
    const float damageWeight = 0.f;//1.0f;
    const float vulnerableWeight = 0.f;//50.f;
    const float hitboxWeight = 0.f;//100.f;

    float score = 0.f;

    // If not the same frame, do not pick
    if (!isSynonymState(cur.myframe, ref.myframe))
        return -HUGE_VAL;

    float relXDist = fabs(cur.relpos.x - ref.relpos.x);
    float relYDist = fabs(cur.relpos.y - ref.relpos.y);

    float relXVel  = fabs(cur.relvel.x - ref.relvel.x);
    float relYVel  = fabs(cur.relvel.y - ref.relvel.y);

    float damageDiff = fabs(cur.enemydamage - ref.enemydamage);
    bool vulnerable = cur.enemyvulnerable == ref.enemyvulnerable;
    bool hitbox = cur.enemyhitbox == ref.enemyhitbox;

    score -= posWeight * (relXDist + relYDist);
    score -= velWeight * (relXVel + relYVel);
    score -= damageWeight * damageDiff;
    score -= vulnerableWeight * vulnerable;
    score -= hitboxWeight * hitbox;

    return score;
}

bool GhostAIPlayer::isSynonymState(const std::string &a, const std::string &b)
{
    if (a == b) return true;

    // Make aa <= bb
    std::string aa = a, bb = b;
    if (aa > bb) std::swap(aa, bb);

    std::pair<std::string, std::string> p(aa, bb);

    std::set<std::pair<std::string, std::string> > synonymstates_;
    synonymstates_.insert(std::pair<std::string, std::string>("GroundNormal", "GroundWalking"));
    synonymstates_.insert(std::pair<std::string, std::string>("Ducking", "GroundWalking"));
    synonymstates_.insert(std::pair<std::string, std::string>("Ducking", "GroundNormal"));

    return synonymstates_.count(p) > 0;
}

std::ostream & operator<<(std::ostream &os, const glm::vec2 &vec)
{
    os << '[' << vec.x << ' ' << vec.y << ']';
    return os;
}

std::ostream & operator<<(std::ostream &os, const CaseGameState &cgs)
{
    os  << /*cgs.abspos << ' ' <<*/ cgs.relpos << ' ' << cgs.relvel << ' '
        << cgs.myframe << ' ' << cgs.facing << ' ' << cgs.enemydamage << ' '
        << cgs.enemyhitbox << ' ' << cgs.enemyvulnerable;

    return os;
}

std::ostream & operator<<(std::ostream &os, const controller_state &cs)
{
    os  << "Stick dir: " << cs.joyx << ' ' << cs.joyy << '\n'
        << "Stick vel: " << cs.joyxv << ' ' << cs.joyyv << '\n'
        << "ButtonA: " << cs.pressa << ' ' << cs.buttona << '\n'
        << "ButtonB: " << cs.pressb << ' ' << cs.buttonb << '\n'
        << "ButtonX: " << cs.pressx << ' ' << cs.buttonx << '\n'
        << "ButtonY: " << cs.pressy << ' ' << cs.buttony << '\n';

    return os;
}

bool CaseGameState::operator<(const CaseGameState &rhs) const
{
    return abspos.x < rhs.abspos.x;
}
