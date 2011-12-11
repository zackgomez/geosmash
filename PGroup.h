
class Particle;
class Emitter;

struct PAction
{
    virtual void operator() (Particle*);

};

class PGroup
{

    void render(float dt);


private:
    std::list<Particle*> particles_;
    std::list<Emitter*> emitters_;
    std::list<PAction*> actions_;

};

