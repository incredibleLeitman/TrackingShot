#pragma once

/*
 tdogl::Camera
 Copyright 2012 Thomas Dalling - http://tomdalling.com/
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */
#define _USE_MATH_DEFINES

#include <cmath>
#include <gtc/matrix_transform.hpp>

class CameraFloating
{
private:
    /*static*/ const float MaxVerticalAngle = 85.0f; //must be less than 90 to avoid gimbal lock

    glm::vec3 _position;
    float _horizontalAngle;
    float _verticalAngle;
    float _fieldOfView;
    float _nearPlane;
    float _farPlane;
    float _viewportAspectRatio;

public:

    CameraFloating() :
        _position(0.0f, 0.0f, 3.0f),
        _horizontalAngle(0.0f),
        _verticalAngle(0.0f),
        _fieldOfView(50.0f),
        _nearPlane(0.01f),
        _farPlane(100.0f),
        _viewportAspectRatio(4.0f / 3.0f)
    {
    }

    const glm::vec3& position() const {
        return _position;
    }

    void setPosition(const glm::vec3& position) {
        _position = position;
    }

    void offsetPosition(const glm::vec3& offset) {
        _position += offset;
    }

    float fieldOfView() const {
        return _fieldOfView;
    }

    void setFieldOfView(float fieldOfView) {
        assert(fieldOfView > 0.0f && fieldOfView < 180.0f);
        _fieldOfView = fieldOfView;
    }

    float nearPlane() const {
        return _nearPlane;
    }

    float farPlane() const {
        return _farPlane;
    }

    void setNearAndFarPlanes(float nearPlane, float farPlane) {
        assert(nearPlane > 0.0f);
        assert(farPlane > nearPlane);
        _nearPlane = nearPlane;
        _farPlane = farPlane;
    }

    glm::mat4 orientation() const {
        glm::mat4 orientation;
        orientation = glm::rotate(orientation, glm::radians(_verticalAngle), glm::vec3(1, 0, 0));
        orientation = glm::rotate(orientation, glm::radians(_horizontalAngle), glm::vec3(0, 1, 0));
        return orientation;
    }

    void offsetOrientation(float upAngle, float rightAngle) {
        _horizontalAngle += rightAngle;
        _verticalAngle += upAngle;
        normalizeAngles();
    }

    void lookAt(glm::vec3 position) {
        assert(position != _position);
        glm::vec3 direction = glm::normalize(position - _position);
        _verticalAngle = glm::radians(asinf(-direction.y));
        _horizontalAngle = -glm::radians(atan2f(-direction.x, -direction.z));
        normalizeAngles();
    }

    float viewportAspectRatio() const {
        return _viewportAspectRatio;
    }

    void setViewportAspectRatio(float viewportAspectRatio) {
        assert(viewportAspectRatio > 0.0);
        _viewportAspectRatio = viewportAspectRatio;
    }

    glm::vec3 forward() const {
        glm::vec4 forward = glm::inverse(orientation()) * glm::vec4(0, 0, -1, 1);
        return glm::vec3(forward);
    }

    glm::vec3 right() const {
        glm::vec4 right = glm::inverse(orientation()) * glm::vec4(1, 0, 0, 1);
        return glm::vec3(right);
    }

    glm::vec3 up() const {
        glm::vec4 up = glm::inverse(orientation()) * glm::vec4(0, 1, 0, 1);
        return glm::vec3(up);
    }

    glm::mat4 matrix() const {
        return projection() * view();
    }

    glm::mat4 projection() const {
        return glm::perspective(glm::radians(_fieldOfView), _viewportAspectRatio, _nearPlane, _farPlane);
    }

    glm::mat4 view() const {
        return orientation() * glm::translate(glm::mat4(), -_position);
    }

    void normalizeAngles() {
        _horizontalAngle = fmodf(_horizontalAngle, 360.0f);
        //fmodf can return negative values, but this will make them all positive
        if (_horizontalAngle < 0.0f)
            _horizontalAngle += 360.0f;

        if (_verticalAngle > MaxVerticalAngle)
            _verticalAngle = MaxVerticalAngle;
        else if (_verticalAngle < -MaxVerticalAngle)
            _verticalAngle = -MaxVerticalAngle;
    }
};