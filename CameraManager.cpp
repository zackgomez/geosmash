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
    const float fov = getParam("camera.fov");
    /*
    updateTarget_(fighters);
    */
    Rectangle rect = getCameraRect_(fighters);

    //std::cout << "Bounding rect - " << rect.x << ' ' << rect.y << ' ' << rect.w << ' ' << rect.h << '\n';
    float z = rect.w / (2 * tanf(fov / 2));
    target_ = glm::vec3(rect.x, rect.y, z);
    //std::cout << "Cam Target: " << target_.x << ' ' << target_.x << ' ' << target_.z << '\n';

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

Rectangle CameraManager::getCameraRect_(const std::vector<Fighter*> &fighters)
{
    // Find the bounding rect
    glm::vec2 min(HUGE_VAL, HUGE_VAL), max(-HUGE_VAL, -HUGE_VAL);
    for (unsigned i = 0; i < fighters.size(); i++)
    {
        if (!fighters[i]->isAlive()) continue;
        glm::vec2 pos = fighters[i]->getPosition();
        if (pos.x < min.x) min.x = pos.x;
        if (pos.y < min.y) min.y = pos.y;
        if (pos.x > max.x) max.x = pos.x;
        if (pos.y > max.y) max.y = pos.y;
    }

    const float margin = getParam("camera.margin");
    min -= glm::vec2(margin, margin / 2);
    max += glm::vec2(margin, 3 * margin / 4);

    // Make sure rect never goes too low
    const float minY = getParam("camera.minY");
    const float maxY = getParam("camera.maxY");
    const float maxXMag = getParam("camera.xrange");
    if (min.y < minY)
        min.y = minY;
    if (max.y > maxY)
        max.y = maxY;
    if (min.x < -maxXMag)
        min.x = -maxXMag;
    if (max.x > maxXMag)
        max.x = maxXMag;

    glm::vec2 pos = (max + min) / 2.f;
    glm::vec2 size = max - min;
    if (size.x > 16.f/9.f * size.y)
        size.y = 9.f / 16.f * size.x;
    else
        size.x = 16.f/9.f * size.y;


    return Rectangle(pos.x, pos.y, size.x, size.y);
}

CameraManager* CameraManager::get()
{
    static CameraManager cm;
    return &cm;
}

