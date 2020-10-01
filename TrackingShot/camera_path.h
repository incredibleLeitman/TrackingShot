#pragma once

//#include <GL/glew.h> // include glew before gl.h (from glfw3)
//#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

//#include <vector>
#include <deque>

class CameraPath
{
private:
    std::deque<glm::vec3> positions;
    bool loop = true;

public:
    CameraPath() {
    }

    void AddPosition(glm::vec3 pos) {
        positions.push_back(pos);
    }

    glm::vec3 NextPos() {
        glm::vec3 val = positions.front();
        positions.pop_front();
        // for looping re-append first pos
        if (loop) positions.push_back(val);
        return val;
    }

    std::vector<glm::vec3> Positions() {
        std::vector<glm::vec3> ret;
        for (int i = 0; i < positions.size(); ++i) {
            ret.push_back(positions[i]);
        }
        return ret;
    }
};
