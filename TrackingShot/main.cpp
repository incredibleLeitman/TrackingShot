//------------------------------------------------------------------------------------------
// Camera Tracking Shot Project for EZG Master Game Engineering FH Technikum Wien
//------------------------------------------------------------------------------------------
//
// General TODOs:
// - combine cameras, only use resulting projection and view matrices
// - structure stuff into classes
// - integrate textures, plains, skybox...
//------------------------------------------------------------------------------------------

// define drawing mode for openGL variant:
// CLASSIC_OGL      classic drawing without shaders, vertexArrays or indexBuffers
// MODERN_NO_SHADER modern openGl without shaders
// MODERN_OGL

#include <iostream>

#define PI 3.14159 // ... TODO: away go stinky constant!

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const GLint WIDTH = 800, HEIGHT = 600;
const int CONTROL_POINTS = 20; // Angabe: mindestens 20 Stützpunkte

// TODO: world
Camera baseCamera(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0, -90); // camera to overview scene
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f)); // floating camera
//CameraFloating cameraFloating; // floating camera
CameraPath cameraPath; // path for waypoints, including rotations
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

    // add camera starting point to waypoints
    /*CameraWaypoint camPt;
    camPt.position = camera.Position;
    camPt.rotation = camera.Rotation;
    cameraPath.AddPosition(camPt);*/

    // add defined amount of waypoints to list
    if (CONTROL_POINTS > 0)
    {
        float rad = 8.0f;
        float deg = 2 * PI / CONTROL_POINTS;
        for (int i = 0; i < CONTROL_POINTS; ++i)
        {
            CameraWaypoint camPt;
            //camPt.position = glm::vec3(rad * cosf(i * deg), rad * sinf(i * deg), 0); // position in circle around center, x/y plane
            camPt.position = glm::vec3(rad * cosf(i * deg), 0, rad * sinf(i * deg)); // position in circle around center, x/z plane
            // set orientation of prev cube to current
            if (i > 0)
            {
                CameraWaypoint& prePt = cameraPath.Positions()[i - 1];
                prePt.rotation = glm::quat(glm::normalize(camPt.position - prePt.position));
                //std::cout << "setting prev rotation to " << glm::to_string(prePt.rotation) << std::endl;
            }
            cameraPath.AddPosition(camPt);
        }
        // set last point orientation to first
        CameraWaypoint& prePt = cameraPath.Positions()[CONTROL_POINTS - 1];
        prePt.rotation = glm::quat(glm::normalize(cameraPath.Positions()[0].position - prePt.position));
        //std::cout << "setting last pt rotation to " << glm::to_string(prePt.rotation) << std::endl;

        // setting starting point for floating camera
        camera.Position = cameraPath.Positions()[0].position;
    }

    // setup global light
    Light gLight;
    //gLight.position = camera.position();
    //gLight.position = glm::vec3(0, 0, 10);
    gLight.position = glm::vec3(0, 0, 0);
    gLight.intensities = glm::vec3(1, 1, 1); // white
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

    // spline interpolation for position and rotation
    size_t curWayPt = 0; // index of current waypoint to drive to
    float t = 0; // t für spline interpolations
    float s = 1; // distance between points
    CameraWaypoint pt0;
    CameraWaypoint pt1;
    CameraWaypoint pt2;
    CameraWaypoint pt3;
    if (cameraPath.PositionsSize() > 0)
    {
        size_t size = cameraPath.PositionsSize();
        pt1 = cameraPath.Positions()[curWayPt];
        pt2 = cameraPath.Positions()[(curWayPt + 1) % size];
        pt3 = cameraPath.Positions()[(curWayPt + 2) % size];
        pt0 = cameraPath.Positions()[(curWayPt > 0) ? curWayPt - 1 : size - 1];
        s = glm::distance(camera.Position, pt2.position); // total distance between current position end next waypoint
    }

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

        float dist = glm::distance(camera.Position, cameraPath.Positions()[(curWayPt + 1) % cameraPath.PositionsSize()].position);
        //std::cout << " dist: " << dist << std::endl;
        if (/*dist < .1f ||*/ t >= 1)
        {
            size_t size = cameraPath.PositionsSize();
            curWayPt = (curWayPt + 1) % size;

            pt1 = cameraPath.Positions()[curWayPt];
            pt2 = cameraPath.Positions()[(curWayPt + 1) % size];
            pt3 = cameraPath.Positions()[(curWayPt + 2) % size];
            pt0 = cameraPath.Positions()[(curWayPt > 0) ? curWayPt - 1 : size - 1];
            s = glm::distance(camera.Position, pt2.position); // total distance between current position end next waypoint

            std::cout << " moving to point #" << curWayPt <<
                " with position: " << glm::to_string(pt2.position) <<
                " and rotation" << glm::to_string(pt2.rotation) <<
                " for t: " << t << std::endl;
            t -= (int)t;
        }

        // setting position calculated by catmull spline function
        camera.Position = catmullSpline(0.5, pt0.position, pt1.position, pt2.position, pt3.position, t);
        t += deltaTime * camSpeed / s;

        // setting rotation view defined by SQUAD (SLERP) algorithm
        // Compute a point on a path according squad equation -> q1 and q2 are control points, s1 and s2 are intermediate control points
        camera.updateRotation(glm::squad(pt1.rotation, pt2.rotation,
            glm::intermediate(pt0.rotation, pt1.rotation, pt2.rotation),
            glm::intermediate(pt1.rotation, pt2.rotation, pt3.rotation), t));

        //--------------------------------------------------------------------------------------------------------
        // change camera mode (controlled by mouse or auto run)
        Camera cam = (editMode) ? baseCamera : camera;
        // pass projection matrix to shader (in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        basicShader.setMat4("projection", projection);

        // camera/view transformation
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

        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        //---------------------------------------------------------------------------------------------------------
        // render a cube for floating camera
        if (editMode)
        {
            //glm::vec3 pos = cameraFloating.position();
            model = glm::translate(model, camera.Position);
            model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));

            // rotate by quaternion
            /*glm::vec3 euler = glm::eulerAngles(camera.Rotation);
            // no visible rotation?
            //model = glm::rotate(model, glm::radians(euler.x), glm::vec3(1.0f, 0.0f, 0.0f));
            //model = glm::rotate(model, glm::radians(euler.y), glm::vec3(0.0f, 1.0f, 0.0f));
            //model = glm::rotate(model, glm::radians(euler.z), glm::vec3(0.0f, 0.0f, 1.0f));
            // little rotation but wrong
            model = glm::rotate(model, euler.x, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, euler.z, glm::vec3(0.0f, 0.0f, 1.0f));*/

            glm::mat4 rotMat = glm::toMat4(camera.Rotation);
            model *= rotMat;

            basicShader.setMat4("model", model);

            // adding additional shader stuff like lighting
            basicShader.setMat4("transform", model);
            basicShader.setVec4("color", glm::vec4(1, 0, 1, 1));
            // setting light params
            basicShader.setVec3("light.position", gLight.position);
            basicShader.setVec3("light.intensities", gLight.intensities);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        //---------------------------------------------------------------------------------------------------------
        // render the sun \ [T] /
        model = glm::mat4(1.0f);
        model = glm::translate(model, gLight.position);
        model = glm::scale(model, glm::vec3(0.2, 0.2, 0.2));
        basicShader.setMat4("model", model);

        // adding additional shader stuff like lighting
        basicShader.setMat4("transform", model);
        basicShader.setVec4("color", glm::vec4(1, 1, 0, 1));

        // setting light params
        basicShader.setVec3("light.position", gLight.position);
        basicShader.setVec3("light.intensities", gLight.intensities);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //--------------------------------------------------------------------------------------------------------
        // render waypoints
        //for (glm::vec3 pos : cameraPath.Positions)
        std::vector<CameraWaypoint> camPos = cameraPath.Positions();
        for (size_t i = 0; i < camPos.size(); ++i)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, camPos[i].position);
            model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
            // roate by fixed rad
            float deg = 2 * PI / CONTROL_POINTS;
            model = glm::rotate(model, -deg * i, glm::vec3(0.0f, 1.0f, 0.0f));
            // rotate by quaternion
            //glm::vec3 euler = glm::eulerAngles(camPos[i].rotation);
            // TODO: find out if already rad?
            //model = glm::rotate(model, glm::radians(euler.x), glm::vec3(1.0f, 0.0f, 0.0f));
            //model = glm::rotate(model, euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
            //model = glm::rotate(model, euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
            //model = glm::rotate(model, euler.z, glm::vec3(0.0f, 0.0f, 1.0f));
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
        int totalCubes = sizeof(cubePositions) / sizeof(glm::vec3);
        for (unsigned int i = 0; i < totalCubes; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::rotate(model, glm::radians(20.0f * i), glm::vec3(1.0f, 0.3f, 0.5f));
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

    // add camera waypoint
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        // prevent adding a new point direclty at / near existing point
        bool add = true;
        glm::vec3 camPos = baseCamera.Position;
        //for (CameraWaypoint pt : cameraPath.Positions) { // illegal indirection TODO: wtf?
        //for (int i = 0; i < cameraPath.Positions.size(); ++i)
        std::vector<CameraWaypoint> wayPos = cameraPath.Positions();
        for (int i = 0; i < wayPos.size(); ++i)
        {
            if (glm::distance(cameraPath.Positions()[i].position, camPos) < 1)
            {
                add = false;
                break;
            }
        }

        if (add)
        {
            CameraWaypoint camPt;
            camPt.position = camPos;
            camPt.rotation = baseCamera.Rotation;
            //std::cout << "Added waypoint #" << cameraPath.Positions().size() <<
            //    " at " << glm::to_string(camPt.position) <<
            //    " with rotation: " << glm::to_string(camPt.rotation) << std::endl;
            cameraPath.AddPosition(camPt);
        }
    }

    // change edit and view mode
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        editMode = true;
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        editMode = false;

    // speed up / slow down
    if (glfwGetKey(window, GLFW_KEY_KP_ADD))
        camSpeed += 0.1f;
    else if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT))
        camSpeed -= 0.1f;

    // move base camera
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