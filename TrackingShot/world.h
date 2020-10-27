#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

Light gLight;
std::vector<Light * > lights;

// set up vertex data (and buffer(s)) and configure vertex attributes
GLfloat vertices[] = {
    //  X     Y     Z       U     V          Normal
    // bottom
    -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
    1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
    1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
    1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

    // top
    -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
    1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
    1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

    // front
    -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
    1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

    // back
   -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
   -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
    1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
    1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
   -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
    1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

    // left
    -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
    -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

    // right
    1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
    1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
    1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
    1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
    1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
};

// world space positions of our cubes
// x: +right, -left
// y: +up, -down
// z: +near, -far
glm::vec3 cubePositions[] = {
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(2.0f,  5.0f, 5.0f),
    glm::vec3(-1.5f, 2.2f, -2.5f),
    glm::vec3(-3.8f, 2.0f, -8.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -6.5f),
    glm::vec3(1.3f, 7.0f, -2.5f),
    glm::vec3(5.5f,  2.0f, -2.5f),
    glm::vec3(1.5f,  0.2f, -5.5f),
    glm::vec3(-1.3f,  3.0f, -1.5f)
};
//std::cout << "cubes.length: " << cubePositions->length() << std::endl; // 3 !!!
//std::cout << "sizeof(cubes): " << sizeof(cubePositions) / sizeof(glm::vec3) << std::endl; // -> correct!