#include <iostream>
#include "CameraManager.h"

CameraManager::CameraManager()
{}

void CameraManager::update(float dt, const std::vector<Fighter *> &fighters)
{
   
    
    updateTarget_(fighters);
    updateCurrent_();


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

}



void CameraManager::updateCurrent_() {


}

CameraManager* CameraManager::get()
{
    static CameraManager cm;
    return &cm;
}

