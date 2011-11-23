#include <iostream>
#include "CameraManager.h"
#include "ParamReader.h"

#define MINY -75.0
#define MINX (-100.0f)
#define MAXX (100.0f)

CameraManager::CameraManager()
{}

void CameraManager::update(float dt, const std::vector<Fighter *> &fighters)
{
    updateTarget_(fighters);
    updateCurrent_(dt);
}

void CameraManager::setCurrent(const glm::vec3 &v)
{
    current_ = v;
}

void CameraManager::updateTarget_(const std::vector<Fighter *> &fighters) {
    glm::vec2 totalPos;
    float minX = 1e6, maxX = -1e6;
    float spread = 0;

    for (unsigned i = 0; i < fighters.size(); i++) {
        if (fighters[i]->isAlive()) 
        {
            glm::vec2 v = fighters[i]->getPosition();
            totalPos += v;
            if (v.x < minX) minX = v.x;
            if (v.x > maxX) maxX = v.x;
        }
    }
    totalPos /= fighters.size();
    printf("spread? %f\n", maxX - minX);

    /*
    std::cout << "x " << totalPos.x
              << ", y " << totalPos.y << std::endl;
              */
    if (totalPos.y < MINY) 
        totalPos.y = MINY;
    if (totalPos.x < MINX) 
        totalPos.x = MINX;
    if (totalPos.x > MAXX)
        totalPos.x = MAXX;
    
    float Z = getParam("camera.minZoom");
    float minZ = getParam("camera.minZoom");
    float maxZ = getParam("camera.maxZoom");
    spread = (maxX - minX) / getParam("camera.maxSpread");
    if (spread > 1) 
        spread = 1;
    assert(spread >= 0);
    Z -= (minZ - maxZ) * (1 - spread);
    assert(Z <= minZ);
    assert(Z >= maxZ);
    target_ = glm::vec3(totalPos.x, getParam("camera.yfactor") * totalPos.y, Z);

}



void CameraManager::updateCurrent_(float dt) {
    glm::vec3 dx = target_ - current_;
    glm::vec3 maxV(getParam("camera.maxVx"),
                   getParam("camera.maxVy"),
                   getParam("camera.maxVz"));
    // If we're trying to move faster than we're allowed,
    // clip our velocity in that direction.
    printf("dx.y: %f\n", dx.y);
    if (fabs(dx.x) > maxV.x * dt) {
        dx.x = dx.x < 0 ? 
            -1 * maxV.x * dt : 
                 maxV.x * dt;
    }
    if (fabs(dx.y) > maxV.y * dt) {
        dx.y = dx.y < 0 ? 
            -1 * maxV.y * dt :
                 maxV.y * dt;
    }
    if (fabs(dx.z) > maxV.z * dt)
    {
        dx.z = dx.z < 0 ?
            -1 * maxV.z * dt :
                 maxV.z * dt;
    }

    current_ = current_ + dx;
    setCamera(current_);

}

CameraManager* CameraManager::get()
{
    static CameraManager cm;
    return &cm;
}

