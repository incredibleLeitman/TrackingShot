#pragma once

//#include <GL/glew.h> // include glew before gl.h (from glfw3)
//#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

//#include <vector>
#include <deque>

struct CameraWaypoint {
    glm::vec3 position;
    glm::quat rotation;
};

class CameraPath
{
private:
    std::deque<CameraWaypoint> positions;
    bool loop = true;

public:
    CameraPath() {
    }

    void AddPosition(CameraWaypoint pos) {
        positions.push_back(pos);
    }

    CameraWaypoint NextPos() {
        CameraWaypoint val = positions.front();
        positions.pop_front();
        // for looping re-append first pos
        if (loop) positions.push_back(val);
        return val;
    }

    std::vector<CameraWaypoint> Positions() {
        std::vector<CameraWaypoint> ret;
        for (int i = 0; i < positions.size(); ++i) {
            ret.push_back(positions[i]);
        }
        return ret;
    }
};
