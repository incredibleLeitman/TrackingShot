#pragma once
// Spline helper functions
// provides catmull-rom spline, a type of interpolating spline (a curve that goes through its control points) defined by four control points
// https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

// GLM Mathematics
#include <glm.hpp>

// Calculate the t value for a Catmull–Rom spline
float getKnot(float alpha, float t, glm::vec3 p0, glm::vec3 p1)
{
    float a = pow((p1.x - p0.x), 2.0f) + pow((p1.y - p0.y), 2.0f) + pow((p1.z - p0.z), 2.0f);
    float b = pow(a, alpha * 0.5f);

    return (b + t);
}

// Given four points, calculate an interpolated point between p1 and p2 using a Catmul-Rom spline.
// t specifies the position along the path, with t=0 being p1 and t=2 being p2.
glm::vec3 catmullSpline(float alpha, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
    float t0 = 0.0f;
    float t1 = getKnot(alpha, t0, p0, p1);
    float t2 = getKnot(alpha, t1, p1, p2);
    float t3 = getKnot(alpha, t2, p2, p3);

    // Lerp t to be between t1 and t2
    t = t1 + t * (t2 - t1);

    glm::vec3 A1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
    glm::vec3 A2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
    glm::vec3 A3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

    glm::vec3 B1 = (t2 - t) / (t2 - t0) * A1 + (t - t0) / (t2 - t0) * A2;
    glm::vec3 B2 = (t3 - t) / (t3 - t1) * A2 + (t - t1) / (t3 - t1) * A3;

    glm::vec3 C = (t2 - t) / (t2 - t1) * B1 + (t - t1) / (t2 - t1) * B2;

    return C;
}