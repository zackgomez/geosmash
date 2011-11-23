#include <iostream>
#include "CameraManager.h"

#define MINY -75.0
#define MINX (-100.0f)
#define MAXX (100.0f)

CameraManager::CameraManager()
{}

void CameraManager::update(float dt, const std::vector<Fighter *> &fighters)
{
    updateTarget_(fighters);
    updateCurrent_();
    //setCamera(glm::vec3(-100, 100, 500));
}

void CameraManager::setCurrent(const glm::vec3 &v)
{
    current_ = v;
}

void CameraManager::updateTarget_(const std::vector<Fighter *> &fighters) {
    glm::vec2 totalPos;

    for (unsigned i = 0; i < fighters.size(); i++) {
        totalPos += fighters[i]->getPosition();
    }
    totalPos /= fighters.size();

    std::cout << "x " << totalPos.x
              << ", y " << totalPos.y << std::endl;
    if (totalPos.y < MINY) {
        totalPos.y = MINY;
    }
    if (totalPos.x < MINX) 
        totalPos.x = MINX;
    if (totalPos.x > MAXX)
        totalPos.x = MAXX;
    target_ = glm::vec3(totalPos.x, totalPos.y, 425);

}



void CameraManager::updateCurrent_() {

    current_ = target_;
    setCamera(current_);

}

CameraManager* CameraManager::get()
{
    static CameraManager cm;
    return &cm;
}

