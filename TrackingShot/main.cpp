#define USE_THIS
#ifdef USE_THIS
//------------------------------------------------------------------------------------------
// Camera Tracking Shot Project for EZG Master Game Engineering FH Technikum Wien
//------------------------------------------------------------------------------------------
//
// General TODOs:
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
#include "cameraPath.h"
#include "shader.h"
#include "light.h"
#include "spline.h"
#include "world.h"
#include "textureHandler.h"

#include "errorHandler.h" // use with GLCALL(glfunction());

// forward declarations
void framebuffer_size_callback (GLFWwindow* window, int width, int height);
void mouse_callback (GLFWwindow* window, double xpos, double ypos);
void scroll_callback (GLFWwindow* window, double xoffset, double yoffset);
void processInput (GLFWwindow* window);
void renderScene (const Shader& shader);

const GLint WIDTH = 800, HEIGHT = 600;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
const float NEAR = 0.1f;
const float FAR = 30.0f;

// TODO: move to world
Camera baseCamera(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0, -90); // camera to overview scene
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f)); // floating camera
CameraPath cameraPath; // path for waypoints, including rotations
const int CONTROL_POINTS = 20; // Angabe UE1: mindestens 20 Stützpunkte
bool editMode = true; // changes beween base and floating camera

// dynamic camera settings
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float camSpeed = SPEED;

float bumpiness = 0.5f; // Bonus UE3: dynamic setting of bumpiness

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int exitWithError (std::string code)
{
    std::cout << "error: " << code << std::endl;
    return EXIT_FAILURE;
}

int main (int argc, char** argv)
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

    // add defined amount of waypoints to list
    if (CONTROL_POINTS > 0)
    {
        float rad = 8.0f;
        float deg = (float)(2 * PI / CONTROL_POINTS);
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
    gLight.position = glm::vec3(0, 10, 0);
    gLight.color = glm::vec3(1, 1, 1); // white
    lights.push_back(&gLight);

    // TODO: add another light -> emitting from camera
    //gLight.position = camera.position();
    //gLight.color = glm::vec3(1, 0, 0); // red

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
    Shader shader("shaders/lightingShader.vs", "shaders/lightingShader.fs"); // actual shader for world objects
    Shader depthShader("shaders/depthShader.vs", "shaders/depthShader.fs"); // depth shader to shadow map

    // ------------- UE3 normal mapping -------------------------------------------------------------------------------
    // used sources:
    //      https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    //      http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
    //      http://ogldev.atspace.co.uk/www/tutorial26/tutorial26.html

    // init textures
    unsigned int diffuseMap = loadTexture("textures/brickwall.jpg");
    unsigned int normalMap = loadTexture("textures/brickwall_normal.jpg");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalMap);

    shader.use();
    shader.setInt("shadowMap", 0);
    shader.setInt("diffuseMap", 1);
    shader.setInt("normalMap", 2);
    // ------------- UE3 normal mapping -------------------------------------------------------------------------------

    // ------------- UE2 shadow mapping -------------------------------------------------------------------------------
    // used sources:
    //      https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    //      http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/

    // framebuffer for rendering depthMap
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // ------------- UE2 shadow mapping -------------------------------------------------------------------------------

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // fill buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // link vertex attributes
    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // texture coord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // normal attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    /*
    // tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
    // bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    */

    // spline interpolation for position and rotation
    size_t curWayPt = 0; // index of current waypoint to drive to
    float t = 0; // t für spline interpolations
    float s = 1; // distance between points
    CameraWaypoint pt0{}, pt1{}, pt2{}, pt3{};
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
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //std::cout << "delta time: " << deltaTime << std::endl;

        processInput(window);

        float dist = glm::distance(camera.Position, cameraPath.Positions()[(curWayPt + 1) % cameraPath.PositionsSize()].position);
        if (t >= 1)
        {
            size_t size = cameraPath.PositionsSize();
            curWayPt = (curWayPt + 1) % size;

            pt1 = cameraPath.Positions()[curWayPt];
            pt2 = cameraPath.Positions()[(curWayPt + 1) % size];
            pt3 = cameraPath.Positions()[(curWayPt + 2) % size];
            pt0 = cameraPath.Positions()[(curWayPt > 0) ? curWayPt - 1 : size - 1];
            s = glm::distance(camera.Position, pt2.position); // total distance between current position end next waypoint

            /*std::cout << " moving to point #" << curWayPt <<
                " with position: " << glm::to_string(pt2.position) <<
                " and rotation" << glm::to_string(pt2.rotation) <<
                " for t: " << t << std::endl;*/
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

        // TODO make toggle for dynamic light position change
        gLight.position.x = (float)sin(currentFrame * camSpeed * 0.1) * 10.0f;
        gLight.position.z = (float)cos(currentFrame * camSpeed * 0.1) * 10.0f; // rotate around y
        //gLight.position.y = 10.0 + cos(currentFrame * camSpeed * 0.1) * 10.0f; // rotate around z

        // able to inc- / decrease radius
        //gLight.position.x = sin(currentFrame) * 3.0f * camSpeed;
        //gLight.position.z = cos(currentFrame) * 3.0f * camSpeed;
        //gLight.position.y = 5.0 + cos(currentFrame) * 1.0f;
        //std::cout << "gLight.position: " << glm::to_string(gLight.position) << std::endl;

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

        // ------------- UE2 shadow mapping -------------------------------------------------------------------------------
        // 1. render depth of scene to texture (from light's perspective)
        // --------------------------------------------------------------
        glm::mat4 lightProjection, lightView, lightSpace;
        //lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, NEAR, FAR); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, NEAR, FAR);
        lightView = glm::lookAt(gLight.position, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpace = lightProjection * lightView;
        // render scene from light's point of view
        depthShader.use();
        depthShader.setMat4("lightSpace", lightSpace);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        renderScene(depthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // ------------- UE2 shadow mapping -------------------------------------------------------------------------------

        // ------------- UE2 shadow mapping -------------------------------------------------------------------------------
        // 2. render scene as normal using the generated depth/shadow map
        // --------------------------------------------------------------
        glViewport(0, 0, WIDTH, HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        // dynamically allow to set bumpiness
        shader.setFloat("bumpiness", bumpiness);

        // change camera mode (controlled by mouse or auto run)
        Camera cam = (editMode) ? baseCamera : camera;
        // pass projection matrix to shader (in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        // camera/view transformation
        shader.setMat4("view", cam.GetViewMatrix());

        // set light uniforms
        shader.setVec3("viewPos", cam.Position);
        shader.setMat4("lightSpace", lightSpace);
        shader.setVec3("light.position", gLight.position);
        shader.setVec3("light.color", gLight.color);
        //glBindTexture(GL_TEXTURE_2D, diffuseMap);
        //glBindTexture(GL_TEXTURE_2D, normalMap);
        //glBindTexture(GL_TEXTURE_2D, depthMap);
        renderScene(shader);
        // ------------- UE2 shadow mapping -------------------------------------------------------------------------------

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return EXIT_SUCCESS;
}

// renders all scene models with the given shader
void renderScene (const Shader &shader)
{
    // calculate the model matrix for each object and pass it to shader before drawing
    glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    //---------------------------------------------------------------------------------------------------------
    // render a cube for floating camera
    if (editMode)
    {
        model = glm::translate(model, camera.Position);
        model = glm::scale(model, glm::vec3(0.5f));
        model *= glm::toMat4(camera.Rotation); // rotate by quaternion

        shader.setMat4("model", model);
        shader.setVec4("color", glm::vec4(1, 0, 1, 1));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    //---------------------------------------------------------------------------------------------------------
    // render plane
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0, -2, 0));
    model = glm::scale(model, glm::vec3(20, 0.1, 20));
    shader.setMat4("model", model);
    shader.setVec4("color", glm::vec4(0, 1, 0, 1));

    glDrawArrays(GL_TRIANGLES, 0, 36);

    //---------------------------------------------------------------------------------------------------------
    // render the sun \ [T] /
    //std::cout << "shader ID: " << shader.ID << std::endl;
    if (shader.ID != 6) // no depth map for light sources
    {
        for (auto light : lights)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, light->position);
            model = glm::scale(model, glm::vec3(0.2f));
            shader.setMat4("model", model);
            shader.setVec4("color", glm::vec4(1, 1, 1, 1));

            glDrawArrays(GL_TRIANGLES, 0, 36);
            // TODO: would be nice to drawSphere(model);
        }
    }

    //--------------------------------------------------------------------------------------------------------
    // render waypoints
    std::vector<CameraWaypoint> camPos = cameraPath.Positions();
    for (size_t i = 0; i < camPos.size(); ++i)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, camPos[i].position);
        model = glm::scale(model, glm::vec3(0.1f));
        // rotate by fixed rad
        float deg = (float)(2 * PI / CONTROL_POINTS);
        model = glm::rotate(model, -deg * i, glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        shader.setVec4("color", glm::vec4(1, 0, 0, 1));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    //--------------------------------------------------------------------------------------------------------
    // render funny world cubes
    unsigned int totalCubes = 5; //sizeof(cubePositions) / sizeof(glm::vec3);
    for (unsigned int i = 0; i < totalCubes; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[i]);
        model = glm::rotate(model, glm::radians(20.0f * i), glm::vec3(1.0f, 0.3f, 0.5f));
        shader.setMat4("model", model);
        shader.setVec4("color", glm::vec4(0, 0, 1, 1));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
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
        for (unsigned int i = 0; i < wayPos.size(); ++i)
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

    // change bumpiness of normal maps
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP))
        //bumpiness += 0.1f;
        bumpiness = glm::min(1.0f, bumpiness + 0.01f);
    else if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN))
        //bumpiness -= 0.1f;
        bumpiness = glm::max(0.0f, bumpiness - 0.01f);

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
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos); // reversed since y-coordinates go from bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    baseCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //std::cout << "scroll callback for " << yoffset << std::endl;
    baseCamera.ProcessMouseScroll((float)yoffset);
}
#endif