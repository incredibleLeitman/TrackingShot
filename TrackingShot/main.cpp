//see: docs.gl fpr opengl specification

// define drawing mode for openGL variant:
// CLASSIC_OGL      classic drawing without shaders, vertexArrays or indexBuffers
// MODERN_NO_SHADER modern openGl without shaders
// MODERN_OGL

#include <iostream>

#define PI 3.14159

// static linking -> need to add Linker input glew32s.lib instead of glew32.lib and GLEW_STATIC as preprocessor Definition
#include <GL/glew.h> // include glew before gl.h (from glfw3)
#include <GLFW/glfw3.h>

#include "camera.h"
#include "camera_floating.h"
#include "camera_path.h"
#include "shader.h"
#include "light.h"
#include "spline.h"
//#include "world.h"

#include "errorHandler.h" // use with GLCALL(glfunction());

#include <gtx/string_cast.hpp> // glm::to_string

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const GLint WIDTH = 800, HEIGHT = 600;

// TODO: world
Camera baseCamera(glm::vec3(0.0f, 0.0f, 20.0f)); // camera to overview scene
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f)); // floating camera
CameraFloating cameraFloating; // floating camera
CameraPath cameraPath;
bool editMode = true; // changes beween base and floating camera

// dynamic camera settings
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float camSpeed = SPEED;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int exitWithError(std::string code)
{
    std::cout << "error: " << code << std::endl;
    return EXIT_FAILURE;
}

int main(int argc, char** argv)
{
    // Initialize the library
    if (!glfwInit())
        return exitWithError("could not initialize glfw");

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "TrackingShot", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return exitWithError("could not initialize glfw window");
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        glfwTerminate();
        std::cout << glewGetErrorString(err) << std::endl;
        return exitWithError("Failed to initialize GLEW");
    }

    // Define the viewport dimensions
    //int screenWidth, screenHeight;
    //glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    //glViewport(0, 0, screenWidth, screenHeight);

    // TODO: define world

    int countBases = 20; // Angabe: mindestens 20 Stützpunkte
    float deg = 2 * PI / countBases;
    for (int i = 0; i < countBases; ++i)
    {
        CameraWaypoint camPt;
        camPt.position = glm::vec3(
            10.0f * cosf(i * deg),
            10.0f * sinf(i * deg),
            0);
        cameraPath.AddPosition(camPt);
        std::cout << " add path waypoint: " << glm::to_string(camPt.position) << std::endl;
    }
    //return 0;

    // setup global light
    Light gLight;
    //gLight.position = camera.position();
    gLight.position = glm::vec3(0, 0, 10);
    gLight.intensities = glm::vec3(1, 1, 1); // whiter
    //gLight.intensities = glm::vec3(1, 0, 0); // red

#ifdef MODERN_NO_SHADER
    // vertex data for modern open gl triangle
    float positions[6] = {
        -.5, -.5,
        0, .5,
        .5, -.5
    };

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), positions, GL_STATIC_DRAW);
    // if no shaders is provided -> default shader is used

    // using vertex attributes to draw stuff in modern gl -> needs to be enabled first
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
#endif

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shader programs
    Shader basicShader("lightingShader.vs", "lightingShader.fs");
    basicShader.use();

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
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -5.0f, -2.5f),
        glm::vec3(5.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -8.5f),
        glm::vec3(-1.3f,  3.0f, -1.5f)
    };
    //std::cout << "cubes.length: " << cubePositions->length() << std::endl; // 3 !!!
    //std::cout << "sizeof(cubes): " << sizeof(cubePositions) / sizeof(glm::vec3) << std::endl; // -> correct!

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glm::vec3 newPos = cameraPath.NextPos().position;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //std::cout << "delta time: " << deltaTime << std::endl;

        // input
        processInput(window);

        // Render here
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef CLASSIC_OGL
        // draw old gl triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(-.5, -.5);
        glVertex2f(0, .5);
        glVertex2f(.5, -.5);
        glEnd();
#elif MODERN_NO_SHADER
        // draw modern gl vertex buffer
        glDrawArrays(GL_TRIANGLES, 0, 3);
#endif

        // activate shader
        basicShader.use();

        float dist = glm::distance(camera.Position, newPos);
        //std::cout << "remaining dist from " << glm::to_string(curPos) << " to trackPt " << glm::to_string(newPos) << ": " << dist << std::endl; // wtf? 1.85978e+08
        if (dist < .5) {
            newPos = cameraPath.NextPos().position;
        }

        //glm::mat4 view = camera.lookAtNextPos(newPos);
        camera.lookAtPos(newPos);
        camera.moveTowardNextPos(deltaTime * camSpeed);

        //--------------------------------------------------------------------------------------------------------
        // change camera mode
        Camera cam = (editMode) ? baseCamera : camera;
        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        basicShader.setMat4("projection", projection);

        // camera/view transformation - controlled by mouse
        glm::mat4 view = cam.GetViewMatrix();
        basicShader.setMat4("view", view);
        //basicShader.setMat4("view", glm::inverse(view));

        /* https://www.tomdalling.com/blog/modern-opengl/04-cameras-vectors-and-input/
        glm::vec3 curPos = cameraFloating.position();
        float dist = glm::distance(curPos, newPos);
        std::cout << "remaining dist from " << glm::to_string(curPos) << " to trackPt " << glm::to_string(newPos) << ": " << dist << std::endl; // wtf? 1.85978e+08

        cameraFloating.lookAt(glm::vec3(0, 0, 0));
        //cameraFloating.lookAt(newPos);
        cameraFloating.offsetPosition(deltaTime * 2 * cameraFloating.forward());
        //cameraFloating.offsetPosition(deltaTime * 2 * cameraFloating.forward());

        //glm::mat4 view = cameraFloating.matrix();
        glm::mat4 view = cameraFloating.view();
        basicShader.setMat4("view", view);*/

        //---------------------------------------------------------------------------------------------------------
        // render a cube for floating camera
        basicShader.use(); // bind shader
        glBindVertexArray(VAO);

        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first

        //glm::vec3 pos = cameraFloating.position();
        glm::vec3 pos = camera.Position;
        model = glm::translate(model, pos);
        model = glm::translate(model, glm::vec3(0, 0, 1)); // position slightly behind camera view
        model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
        basicShader.setMat4("model", model);

        // adding additional shader stuff like lighting
        basicShader.setMat4("transform", model);
        basicShader.setVec4("color", glm::vec4(1, 0, 1, 1));
        // setting light params
        basicShader.setVec3("light.position", gLight.position);
        basicShader.setVec3("light.intensities", gLight.intensities);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //--------------------------------------------------------------------------------------------------------
        // render cubes for cameraPath points
        //for (glm::vec3 pos : cameraPath.Positions)
        std::vector<CameraWaypoint> camPos = cameraPath.Positions();
        for (int i = 0; i < camPos.size(); ++i)
        {
            basicShader.use(); // bind shader
            glBindVertexArray(VAO);

            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first

            //glm::vec3 pos = cameraFloating.position();
            model = glm::translate(model, camPos[i].position);
            model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
            basicShader.setMat4("model", model);

            // adding additional shader stuff like lighting
            basicShader.setMat4("transform", model);
            basicShader.setVec4("color", glm::vec4(1, 0, 0, 1));
            // setting light params
            basicShader.setVec3("light.position", gLight.position);
            basicShader.setVec3("light.intensities", gLight.intensities);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        //--------------------------------------------------------------------------------------------------------
        // render funny world cubes
        glBindVertexArray(VAO);
        int totalCubes = sizeof(cubePositions) / sizeof(glm::vec3);
        for (unsigned int i = 0; i < totalCubes; i++)
        {
            basicShader.use(); // bind shader

            // calculate the model matrix for each object and pass it to shader before drawing
            model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            basicShader.setMat4("model", model);

            // adding additional shader stuff like lighting
            basicShader.setMat4("transform", model);
            basicShader.setVec4("color", glm::vec4(0, 0, 1, 1));
            // setting light params
            basicShader.setVec3("light.position", gLight.position);
            basicShader.setVec3("light.intensities", gLight.intensities);

            glDrawArrays(GL_TRIANGLES, 0, 36);

            // TODO: unbind shader?
            //glUseProgram(0);
        }
        //--------------------------------------------------------------------------------------------------------

        // TODO: world->render()

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    //std::cout << "processing input... " << std::endl;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    /*if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        editMode ^= true;
        std::cout << "setting editMode to: " << editMode << std::endl;
    }*/
    /*int state = glfwGetKey(window, GLFW_KEY_SPACE);
    if (state == GLFW_PRESS) {
        editMode ^= true;
        std::cout << "setting editMode to: " << editMode << std::endl;
    }*/

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        editMode = true;
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        editMode = false;

    if (glfwGetKey(window, GLFW_KEY_KP_ADD))
        camSpeed += 0.1f;

    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT))
        camSpeed -= 0.1f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        baseCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        baseCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        baseCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        baseCamera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    //std::cout << "framebuffer callback for " << width << ", " << height << std::endl;
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    //std::cout << "mouse callback on " << xpos << ", " << ypos << std::endl;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    baseCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //std::cout << "scroll callback for " << yoffset << std::endl;
    baseCamera.ProcessMouseScroll(yoffset);
}