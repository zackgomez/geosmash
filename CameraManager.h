#pragma once

#include "glutils.h"
#include <vector>
#include <glm/glm.hpp>
#include "Fighter.h"
class CameraManager
{
public:
    CameraManager();
    static CameraManager* get();

    // Called every frame
    // Perform two functions every time:
    //     - Update our target camera position based on where players are
    //     - Move toward our target camera position
    void update(float dt, const std::vector<Fighter*> &fighters);

    // Set the initial camera position.
    void setCurrent(const glm::vec3 &);

private:
    glm::vec3 target_;
    glm::vec3 current_;

    // Sets where we'd like to move to, based on fighter positions
    void updateTarget_(const std::vector<Fighter *> &fighters);
    // Moves the camera position
    void updateCurrent_();

};

