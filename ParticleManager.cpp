#include <iostream>
#include "ParticleManager.h"
#include "ParamReader.h"

ParticleManager::ParticleManager()
{}

void ParticleManager::render(float dt)
{
}

ParticleManager* ParticleManager::get() {
    static ParticleManager pm;
    return &pm;
}
