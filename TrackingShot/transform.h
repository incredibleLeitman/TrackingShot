#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Transform {
    Transform() = default;

    glm::mat4 matrix = glm::mat4(1.0f); // Initialize as identity

    void translate(glm::vec3 offset) {
        matrix = glm::translate(matrix, offset);
    }

    void uniform_scale(float factor) {
        scale(glm::vec3(factor, factor, factor));
    }

    void scale(glm::vec3 factors) {
        matrix = glm::scale(matrix, factors);
    }

    void rotate(float degrees, glm::vec3 axis) {
        matrix = glm::rotate(matrix, glm::radians(degrees), axis);
    }

    glm::vec3 getPosition() const {
        return matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    glm::vec3 forward() const {
        return matrix * glm::vec4(0.0, 0.0, -1.0, 0.0);
    }

    glm::vec3 up() const {
        return  matrix * glm::vec4(0.0, 1.0, 0.0, 0.0);
    }

    glm::vec3 right() const {
        return matrix * glm::vec4(1.0, 0.0, 0.0, 0.0);
    }
};