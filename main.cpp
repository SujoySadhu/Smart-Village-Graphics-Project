/**
 * 3D Smart Village Simulation
 * 
 * A comprehensive OpenGL 3.3 Core Profile demonstration featuring:
 * - Hierarchical Modeling (Windmill with rotating blades)
 * - Phong Illumination Model (Ambient + Diffuse + Specular)
 * - Day/Night Cycle with dynamic lighting
 * - Bezier Surface of Revolution (Water Tank / Mosque Dome)
 * - Texture Mapping and Material Properties
 * - Flying Camera Navigation
 * - Animated Objects (Boat, Windmill blades, Water flow)
 * 
 * ============================================================================
 * KEYBOARD CONTROLS:
 * ============================================================================
 * 
 * CAMERA MOVEMENT:
 *   W/S         - Move forward/backward
 *   A/D         - Move left/right
 *   Space       - Move up
 *   Left Ctrl   - Move down
 *   Shift+WASD  - Fast movement
 *   1/2         - Decrease/Increase camera speed
 *   C           - Reset camera to default position
 * 
 * ROTATION:
 *   R           - Rotate scene clockwise (around Y-axis)
 *   T           - Rotate scene anti-clockwise (around Y-axis)
 * 
 * BOAT CONTROL:
 *   Arrow Up    - Move boat forward
 *   Arrow Down  - Move boat backward
 *   Arrow Left  - Move boat left
 *   Arrow Right - Move boat right
 * 
 * LIGHTING:
 *   N           - Toggle Day/Night mode
 *   L           - Toggle all streetlights (manual override)
 * 
 * ANIMATIONS:
 *   P           - Pause/Resume all animations
 *   B           - Pause/Resume boat movement
 *   V           - Pause/Resume windmill rotation
 *   +/-         - Increase/Decrease windmill speed
 *   [/]         - Increase/Decrease boat speed
 * 
 * RENDERING:
 *   F           - Toggle wireframe mode
 * 
 * OTHER:
 *   H           - Print help (controls) to console
 *   ESC         - Exit application
 * 
 * ============================================================================
 * 
 * Author: Graphics Project
 * Libraries: GLFW, GLAD, GLM
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "BezierSurface.h"
#include "SceneObjects.h"

// Window dimensions
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 10.0f, 30.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
glm::vec3 defaultCameraPos = glm::vec3(0.0f, 10.0f, 30.0f);
float defaultYaw = -90.0f;
float defaultPitch = 0.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Day/Night mode
bool isNight = false;

// Animation controls
bool animationsPaused = false;
bool boatPaused = false;
bool windmillsPaused = false;
float windmillSpeedMultiplier = 1.0f;
float boatSpeedMultiplier = 1.0f;

// Rendering options
bool wireframeMode = false;
// Lighting override
bool streetlightsOverride = false;
bool streetlightsManualOn = false;

// ============================================================================
// TEXTURE CONTROLS (Keys 1-5)
// ============================================================================
bool enableTextures = true;      // Key 1: Toggle all textures ON/OFF
int textureBlendMode = 0;        // Key 2/3/4: 0=blended(fragment), 1=simple, 2=blended(vertex)
float textureBlendFactor = 0.5f; // [/] keys: 0.0=all color, 1.0=all texture
bool enableCurvyTexture = true;  // Key 5: Textured curvy surface (dome+cone)
bool enableWaterTexture = true;  // Key 6: Water texture

// ============================================================================
// LIGHTING CONTROLS (Keys 6-8)
// ============================================================================
bool enableDirLight = true;      // Key 7: Directional Light (Sun)
bool enablePointLights = true;   // Key 8: Point Lights (Lamps)
bool enableSpotLights = true;    // Key 9: Spot Lights
bool enableAmbient = true;
bool enableDiffuse = true;
bool enableSpecular = true;

// 4-Viewport mode toggle
bool fourViewportMode = false;   // Key 0: Toggle 4-viewport split

// Scene rotation angle (controlled by R/T keys)
float sceneRotation = 0.0f;
// ============================================================================
// ADVANCED RENDERING GLOBALS
// ============================================================================

// Shadow Mapping
unsigned int depthMapFBO;
unsigned int depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // High resolution for sharp shadows
const float SHADOW_NEAR = 1.0f, SHADOW_FAR = 100.0f;

// Planar Reflection (Water)
unsigned int reflectionFBO;
unsigned int reflectionTexture;
unsigned int reflectionRBO; // For depth/stencil buffer
const unsigned int REFLECTION_WIDTH = 1200, REFLECTION_HEIGHT = 900; // Match screen roughly

// Key state tracking (for toggle keys)
struct KeyStates {
    bool N = false;  // Day/Night
    bool P = false;  // Pause all
    bool B = false;  // Pause boat
    bool V = false;  // Pause windmills
    bool C = false;  // Reset camera
    bool L = false;  // Streetlights
    bool H = false;  // Help
    bool Plus = false;
    bool Minus = false;
    bool BracketLeft = false;
    bool BracketRight = false;
    // Texture controls (Keys 1-6)
    bool Key1 = false;  // Toggle textures
    bool Key2 = false;  // Simple texture mode
    bool Key3 = false;  // Blended (fragment) mode
    bool Key4 = false;  // Blended (vertex) mode
    bool Key5 = false;  // Curvy surface texture
    bool Key6 = false;  // Water texture
    // Lighting & viewport (Keys 7-9, 0)
    bool Key7 = false;  // Directional Light
    bool Key8 = false;  // Point Lights
    bool Key9 = false;  // Spot Lights
    bool Key0 = false;  // 4-Viewport mode
} keyStates;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, std::vector<Windmill>& windmills /*, Boat& boat */);
void setupLighting(Shader& shader);
void printHelp();


glm::mat4 customLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
    // Calculate the forward vector (Z-axis of camera, pointing backwards)
    glm::vec3 f = glm::normalize(center - eye);  // Forward direction
    
    // Calculate the right vector (X-axis of camera)
    glm::vec3 r = glm::normalize(glm::cross(f, up));
    
    // Recalculate up vector (Y-axis of camera, ensures orthogonality)
    glm::vec3 u = glm::cross(r, f);
    
    // Construct the view matrix
    // This is a rotation matrix combined with a translation
    // View = R * T where R rotates world to camera space, T translates to origin
    glm::mat4 result(1.0f);
    
    // First row (right vector)
    result[0][0] = r.x;
    result[1][0] = r.y;
    result[2][0] = r.z;
    
    // Second row (up vector)
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    
    // Third row (negative forward, OpenGL looks down -Z)
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    
    // Translation component (dot products for camera position)
    result[3][0] = -glm::dot(r, eye);
    result[3][1] = -glm::dot(u, eye);
    result[3][2] = glm::dot(f, eye);
    
    return result;
}


glm::mat4 customPerspective(float fovY, float aspect, float nearPlane, float farPlane)
{
    // Calculate the tangent of half the field of view
    float tanHalfFovy = tan(fovY / 2.0f);
    
    // Initialize to zero matrix
    glm::mat4 result(0.0f);
    
    // The perspective projection matrix transforms from view space to clip space
    // Formula derived from frustum equations:
    // x' = x * (1 / (aspect * tan(fov/2))) / -z
    // y' = y * (1 / tan(fov/2)) / -z
    // z' = -(far + near) / (far - near) - 2*far*near / ((far - near) * z)
    
    // Column 0
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    
    // Column 1
    result[1][1] = 1.0f / tanHalfFovy;
    
    // Column 2
    result[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result[2][3] = -1.0f;  // This causes the perspective divide
    
    // Column 3
    result[3][2] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    
    return result;
}

int main()
{
    // ========================================================================
    // GLFW INITIALIZATION
    // ========================================================================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Smart Village - Press H for Help", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ========================================================================
    // GLAD INITIALIZATION
    // ========================================================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Print controls on startup
    printHelp();

    // ========================================================================
    // SHADER SETUP
    // ========================================================================
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    shader.use();
    shader.setBool("useTexture", false);
    shader.setFloat("alpha", 1.0f);
    shader.setInt("texture_diffuse", 0);
    
    // ========================================================================
    // SHADOW MAPPING SETUP
    // ========================================================================
    glGenFramebuffers(1, &depthMapFBO);
    // Create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // Attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Shadow mapping shader (depth pass)
    Shader shadowShader("shaders/shadow_depth.vs", "shaders/shadow_depth.fs");

    // ========================================================================
    // PLANAR REFLECTION SETUP (WATER)
    // ========================================================================
    glGenFramebuffers(1, &reflectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
    // Color attachment texture
    glGenTextures(1, &reflectionTexture);
    glBindTexture(GL_TEXTURE_2D, reflectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, REFLECTION_WIDTH, REFLECTION_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);
    // RBO for depth/stencil
    glGenRenderbuffers(1, &reflectionRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, REFLECTION_WIDTH, REFLECTION_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, reflectionRBO);
    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Reflection Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ========================================================================
    // SCENE OBJECTS CREATION
    // ========================================================================
    
    // Terrain
    Terrain terrain(100.0f, 100.0f);

    // Spline path for River and Boat
    std::vector<glm::vec3> riverPath = {
        glm::vec3(-60.0f, 0.0f, 5.0f),  // P0 (control)
        glm::vec3(-40.0f, 0.0f, 2.0f),  // P1 (start)
        glm::vec3(-20.0f, 0.0f, -1.0f), // P2
        glm::vec3(0.0f, 0.0f, 1.0f),    // P3
        glm::vec3(20.0f, 0.0f, -2.0f),  // P4
        glm::vec3(40.0f, 0.0f, 3.0f),   // P5 (end)
        glm::vec3(60.0f, 0.0f, 6.0f)    // P6 (control)
    };

    // River (flows through the middle of the scene following curve)
    River river(riverPath, 8.0f);

    // Bridge over the river
    Bridge bridge(glm::vec3(0.0f, 0.5f, 0.0f), 12.0f, 4.0f, 90.0f);

    // Windmill Farm (3 windmills with hierarchical modeling)
    std::vector<Windmill> windmills;
    windmills.push_back(Windmill(glm::vec3(-20.0f, 0.0f, -20.0f), 0.0f, 45.0f));
    windmills.push_back(Windmill(glm::vec3(-10.0f, 0.0f, -25.0f), 30.0f, 60.0f));
    windmills.push_back(Windmill(glm::vec3(0.0f, 0.0f, -20.0f), -15.0f, 50.0f));

    // Village Houses with Solar Panels - repositioned for the larger design
    std::vector<MudHouse> houses;
    houses.push_back(MudHouse(glm::vec3(20.0f, 0.0f, -18.0f), 0.0f, 1.0f));
    houses.push_back(MudHouse(glm::vec3(-25.0f, 0.0f, 18.0f), 90.0f, 1.0f));
    houses.push_back(MudHouse(glm::vec3(-25.0f, 0.0f, -18.0f), -30.0f, 0.9f));

    // Mosque with Bezier Dome - centered nicely
    MosqueDome mosque(glm::vec3(30.0f, 0.0f, 20.0f), 1.2f);

    // Water Tank (Bezier Surface of Revolution)
    WaterTank waterTank(glm::vec3(-25.0f, 0.0f, 15.0f), 1.0f);

    // Boat on the river following the exact same spline path
    // Need to lift the path slightly for the boat's Y origin
    std::vector<glm::vec3> boatPath = riverPath;
    for(auto& p : boatPath) p.y += 0.2f;
    Boat boat(boatPath, 3.0f);


    // Trees along the riverbanks and scattered around the scene
    std::vector<Tree> trees;
    // Trees along left bank of river
    trees.push_back(Tree(glm::vec3(-35.0f, 0.0f, -6.0f), 1.0f, 0.0f));
    trees.push_back(Tree(glm::vec3(-25.0f, 0.0f, -6.5f), 1.2f, 45.0f));
    trees.push_back(Tree(glm::vec3(-15.0f, 0.0f, -5.8f), 0.9f, 90.0f));
    trees.push_back(Tree(glm::vec3(-5.0f, 0.0f, -6.2f), 1.1f, 135.0f));
    trees.push_back(Tree(glm::vec3(5.0f, 0.0f, -6.0f), 1.0f, 180.0f));
    trees.push_back(Tree(glm::vec3(15.0f, 0.0f, -6.5f), 1.15f, 225.0f));
    trees.push_back(Tree(glm::vec3(25.0f, 0.0f, -5.9f), 0.95f, 270.0f));
    trees.push_back(Tree(glm::vec3(35.0f, 0.0f, -6.3f), 1.05f, 315.0f));
    
    // Trees along right bank of river
    trees.push_back(Tree(glm::vec3(-30.0f, 0.0f, 6.0f), 1.1f, 30.0f));
    trees.push_back(Tree(glm::vec3(-20.0f, 0.0f, 6.5f), 0.95f, 75.0f));
    trees.push_back(Tree(glm::vec3(-10.0f, 0.0f, 5.8f), 1.0f, 120.0f));
    trees.push_back(Tree(glm::vec3(0.0f, 0.0f, 6.2f), 1.2f, 165.0f));
    trees.push_back(Tree(glm::vec3(10.0f, 0.0f, 6.0f), 0.9f, 210.0f));
    trees.push_back(Tree(glm::vec3(20.0f, 0.0f, 6.5f), 1.05f, 255.0f));
    trees.push_back(Tree(glm::vec3(30.0f, 0.0f, 5.9f), 1.15f, 300.0f));
    
    // Trees scattered in background
    trees.push_back(Tree(glm::vec3(-30.0f, 0.0f, -15.0f), 1.3f, 60.0f));
    trees.push_back(Tree(glm::vec3(-35.0f, 0.0f, -20.0f), 1.1f, 120.0f));
    // trees.push_back(Tree(glm::vec3(35.0f, 0.0f, 15.0f), 1.2f, 180.0f)); // Unnecessary tree
    trees.push_back(Tree(glm::vec3(40.0f, 0.0f, 10.0f), 0.95f, 240.0f));
    trees.push_back(Tree(glm::vec3(-40.0f, 0.0f, 20.0f), 1.1f, 300.0f));

    // Streetlights on land (river is around Z=0, banks at ~±5)
    // Placing on land areas away from trees
    std::vector<Streetlight> streetlights;
    streetlights.push_back(Streetlight(glm::vec3(0.0f, 0.0f, -12.0f)));   // South of bridge
    streetlights.push_back(Streetlight(glm::vec3(15.0f, 0.0f, -12.0f)));  // Near mosque
    streetlights.push_back(Streetlight(glm::vec3(-15.0f, 0.0f, -12.0f))); // Near windmills
    streetlights.push_back(Streetlight(glm::vec3(0.0f, 0.0f, 12.0f)));    // North of bridge

    // Bird flock flying through the sky
    BirdFlock birdFlock(glm::vec3(0.0f, 0.0f, 0.0f), 15);

    // Print controls to console at startup
    printHelp();

    // ========================================================================
    // RENDER LOOP
    // ========================================================================
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window, windmills /*, boat */);

        // ====================================================================
        // UPDATE PHASE
        // ====================================================================
        
        // Update windmill blade rotations (hierarchical animation)
        if (!animationsPaused && !windmillsPaused)
        {
            for (auto& windmill : windmills)
            {
                windmill.Update(deltaTime * windmillSpeedMultiplier);
            }
        }

        // Update boat position using Spline
        if (!animationsPaused && !boatPaused)
        {
            float originalSpeed = boat.speed;
            boat.speed = originalSpeed * boatSpeedMultiplier;
            boat.Update(deltaTime);
            boat.speed = originalSpeed;
        }

        // Update river flow animation
        if (!animationsPaused)
        {
            river.Update(deltaTime);
        }

        // Update house door/window animations
        for (auto& house : houses) house.Update(deltaTime);

        // Update bird flock animation
        if (!animationsPaused)
        {
            birdFlock.Update(deltaTime);
        }

        // Update streetlight states based on day/night or manual override
        for (auto& light : streetlights)
        {
            if (streetlightsOverride)
            {
                light.SetNightMode(streetlightsManualOn);
            }
            else
            {
                light.SetNightMode(isNight);
            }
        }

        // ====================================================================
        // RENDER PHASE
        // ====================================================================

        // Get actual window size for correct aspect ratio and viewport scaling
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1; // Prevent divide by zero

        // Calculate view and projection matrices at the start of frame
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)currentWidth / (float)currentHeight,
            0.1f, 500.0f
        );
        glm::mat4 view = camera.GetViewMatrix();

        // Define Scene Drawing Logic (Lambda to avoid duplication)
        // We capture objects like terrain, river, boat, etc. by reference
        auto DrawScene = [&](Shader& sh, glm::mat4 v, glm::mat4 p, glm::vec3 pos, bool drawWater = false) {
            // Set vertex color mode uniforms (Key 4: color computed on vertex)
            bool vertexMode = (textureBlendMode == 2);
            sh.setBool("computeVertexColor", vertexMode);
            sh.setBool("useVertexColor", vertexMode);
            sh.setFloat("textureBlendFactor", textureBlendFactor);
            if (vertexMode) {
                sh.setVec3("vertexLightDir", glm::vec3(0.5f, 1.0f, 0.3f)); // Sun direction
            }

            // Draw terrain
            terrain.Draw(sh);

            // Draw river banks and bed (opaque parts only)
            river.Draw(sh);
            sh.setFloat("textureOffset", 0.0f); 

            // Draw bridge
            bridge.Draw(sh);

            // Draw windmills
            for (auto& windmill : windmills) windmill.Draw(sh);

            // Draw mud houses
            for (auto& house : houses) house.Draw(sh);

            // Draw mosque
            mosque.Draw(sh);
            
            // Campfire removed

            // Water tank removed
            // waterTank.Draw(sh);

            // Draw boat 
            boat.Draw(sh, v, p, pos);
            
            // Restore shader after boat (boat uses its own internal shader)
            sh.use(); 
            sh.setFloat("alpha", 1.0f); 
            sh.setBool("useTexture", false);
            sh.setInt("texture_diffuse", 0);

            // Draw trees (with texture mapping - trunk and foliage)
            for (auto& tree : trees) tree.Draw(sh);
            
            // Reset texture state after trees
            sh.setBool("useTexture", false);

            // Draw streetlights - set isOn based on lamp state so they glow
            bool lampsOn = streetlightsOverride ? streetlightsManualOn : isNight;
            for (auto& light : streetlights) {
                light.isOn = lampsOn && enablePointLights;  // Glow when point lights are on
                light.Draw(sh);
            }

            // Draw birds flying through the sky
            birdFlock.Draw(sh);
            
            // Draw Transparent Water (Only in Main Pass)
            if(drawWater) {
                 river.DrawWaterSurface(sh);
                 sh.setFloat("alpha", 1.0f);
            }
        };


        // --------------------------------------------------------------------
        // 1. SHADOW PASS (Render Depth of Scene from Light's Perspective)
        // --------------------------------------------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Configure Light Space Matrix
        // Sun is at Directional Light Position (which is direction * -1)
        // We'll pick a position far enough to cover the scene
        glm::vec3 lightPos = glm::vec3(-20.0f, 50.0f, -10.0f); // Roughly matching the sun direction (0.5, 1.0, 0.3)
        // Ideally we should extract this from dirLight direction
        // Direction is (0.5, 1.0, 0.3) -> Pos is (-50, -100, -30) relative?
        // Let's stick to a fixed position that covers the map
        lightPos = glm::vec3(-20.0f, 60.0f, -20.0f); 
        
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, SHADOW_NEAR, SHADOW_FAR);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        
        // Draw scene (no water for shadow casting usually, or yes?)
        // Water is flat, it receives shadows but doesn't cast much important shadow
        // Pass light matrices and position for Boat::Draw
        DrawScene(shadowShader, lightView, lightProjection, lightPos, false);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // --------------------------------------------------------------------
        // 2. REFLECTION PASS (Render Mirrored Scene from Water Plane)
        // --------------------------------------------------------------------
        glViewport(0, 0, REFLECTION_WIDTH, REFLECTION_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Reflection Camera Calculation
        // Water plane is at Y=0 roughly (actually river bed is lower, surface is at Y=0?)
        // Let's assume water surface is at Y=0.05 (based on river code)
        float waterHeight = 0.05f;
        float distance = 2.0f * (camera.Position.y - waterHeight);
        glm::vec3 reflectionCamPos = camera.Position;
        reflectionCamPos.y -= distance; // Move camera under water
        
        // Invert Pitch (careful with Yaw, Yaw stays same for planar reflection on XZ)
        // We actually need to Construct a View Matrix that is "Mirrored"
        camera.Position.y -= distance; // Temporarily move camera
        camera.Pitch = -camera.Pitch;  // Invert looking direction
        camera.ProcessMouseMovement(0, 0, false); // Update vectors
        
        glm::mat4 reflectionView = camera.GetViewMatrix();
        
        shader.use(); // Use Normal Shader (not Shadow Shader)
        shader.setMat4("view", reflectionView);
        shader.setMat4("projection", projection);
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix); // Needed for shadows in reflection too!
        shader.setVec3("viewPos", camera.Position);
        shader.setBool("isNight", isNight);
        setupLighting(shader); // Bind lights
        
        // Clipping Plane (Remove things below water)
        // Plane Eq: Ax + By + Cz + D = 0. Y > 0.05 -> 0x + 1y + 0z - 0.05 > 0
        shader.setVec4("clipPlane", glm::vec4(0.0, 1.0, 0.0, -waterHeight + 0.1f)); // Slight offset to fix artifacts
        
        // Bind Shadow Map for Reflections too (Reflections can be shadowed!)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader.setInt("shadowMap", 1);
        
        DrawScene(shader, reflectionView, projection, reflectionCamPos, false); // Don't draw water in reflection
        
        // Restore Camera
        camera.Pitch = -camera.Pitch;
        camera.Position.y += distance;
        camera.ProcessMouseMovement(0, 0, false);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // --------------------------------------------------------------------
        // 3. MAIN PASS (Render Final Scene)
        // Supports 4-Viewport Mode (Key 4)
        // --------------------------------------------------------------------
        
        // Clear buffers (full screen)
        glViewport(0, 0, currentWidth, currentHeight);
        if (isNight) glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        else glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (wireframeMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        // Lambda to render a viewport with given view/projection
        auto RenderViewport = [&](int x, int y, int w, int h, glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos) {
            glViewport(x, y, w, h);
            glScissor(x, y, w, h);
            glEnable(GL_SCISSOR_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);  // Clear depth for this viewport
            
            shader.use();
            shader.setMat4("view", viewMat);
            shader.setMat4("projection", projMat);
            shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shader.setVec3("viewPos", camPos);
            shader.setBool("isNight", isNight);
            shader.setVec4("clipPlane", glm::vec4(0.0, 1.0, 0.0, 10000.0f));
            
            setupLighting(shader);
            
            // Update spot light position to boat's searchlight
            glm::mat4 boatModel = glm::mat4(1.0f);
            boatModel = glm::translate(boatModel, boat.position);
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatYaw), glm::vec3(0, 1, 0));
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatPitch), glm::vec3(0, 0, 1));
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatRoll), glm::vec3(1, 0, 0));
            
            glm::vec3 spotPos = glm::vec3(boatModel * glm::vec4(1.8f, 1.2f, 0.0f, 1.0f)); // Front of boat
            glm::vec3 spotDir = glm::normalize(glm::vec3(boatModel * glm::vec4(1.0f, -0.15f, 0.0f, 0.0f))); // Pointing forward and slightly down
            shader.setVec3("spotLights[0].position", spotPos);
            shader.setVec3("spotLights[0].direction", spotDir);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            shader.setInt("shadowMap", 1);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, reflectionTexture);
            shader.setInt("reflectionTexture", 2);
            
            DrawScene(shader, viewMat, projMat, camPos, true);
            
            glDisable(GL_SCISSOR_TEST);
        };
        
        if (fourViewportMode)
        {
            // ================================================================
            // 4-VIEWPORT MODE (Assignment Requirement)
            // Split window into 4 equal viewports
            // Uses customPerspective() - Assignment Requirement #5
            // ================================================================
            int halfW = currentWidth / 2;
            int halfH = currentHeight / 2;
            float aspectRatio = (float)halfW / (float)halfH;
            // Use CUSTOM perspective projection instead of glm::perspective
            glm::mat4 projHalf = customPerspective(glm::radians(45.0f), aspectRatio, 0.1f, 200.0f);
            
            // --- Top-Left: Isometric/General View ---
            // Uses custom lookAt implementation
            glm::vec3 isoEye = glm::vec3(40.0f, 30.0f, 40.0f);
            glm::vec3 isoCenter = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::mat4 isoView = customLookAt(isoEye, isoCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            RenderViewport(0, halfH, halfW, halfH, isoView, projHalf, isoEye);
            
            // --- Top-Right: Top View (Bird's Eye) ---
            glm::vec3 topEye = glm::vec3(0.0f, 80.0f, 0.1f);  // Slightly offset Z to avoid gimbal lock
            glm::vec3 topCenter = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::mat4 topView = customLookAt(topEye, topCenter, glm::vec3(0.0f, 0.0f, -1.0f));  // Use Z as up for top view
            RenderViewport(halfW, halfH, halfW, halfH, topView, projHalf, topEye);
            
            // --- Bottom-Left: Front View ---
            glm::vec3 frontEye = glm::vec3(0.0f, 10.0f, 60.0f);
            glm::vec3 frontCenter = glm::vec3(0.0f, 5.0f, 0.0f);
            glm::mat4 frontView = customLookAt(frontEye, frontCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            RenderViewport(0, 0, halfW, halfH, frontView, projHalf, frontEye);
            
            // --- Bottom-Right: Inside/First-Person View (Uses main camera) ---
            RenderViewport(halfW, 0, halfW, halfH, view, projHalf, camera.Position);
        }
        else
        {
            // ================================================================
            // SINGLE VIEWPORT MODE (Default)
            // ================================================================
            glViewport(0, 0, currentWidth, currentHeight);
            
            shader.use();
            shader.setMat4("view", view);
            shader.setMat4("projection", projection);
            shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shader.setVec3("viewPos", camera.Position);
            shader.setBool("isNight", isNight);
            shader.setVec4("clipPlane", glm::vec4(0.0, 1.0, 0.0, 10000.0f));
            
            setupLighting(shader);
            
            // Update spot light position to boat's searchlight
            glm::mat4 boatModel = glm::mat4(1.0f);
            boatModel = glm::translate(boatModel, boat.position);
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatYaw), glm::vec3(0, 1, 0));
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatPitch), glm::vec3(0, 0, 1));
            boatModel = glm::rotate(boatModel, glm::radians(boat.boatRoll), glm::vec3(1, 0, 0));
            
            glm::vec3 spotPos = glm::vec3(boatModel * glm::vec4(1.8f, 1.2f, 0.0f, 1.0f)); // Front of boat
            glm::vec3 spotDir = glm::normalize(glm::vec3(boatModel * glm::vec4(1.0f, -0.15f, 0.0f, 0.0f))); // Pointing forward and slightly down
            shader.setVec3("spotLights[0].position", spotPos);
            shader.setVec3("spotLights[0].direction", spotDir);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            shader.setInt("shadowMap", 1);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, reflectionTexture);
            shader.setInt("reflectionTexture", 2);

            DrawScene(shader, view, projection, camera.Position, true);
        }

        // Render loop complete
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ============================================================================
// LIGHTING SETUP
// ============================================================================

/**
 * LIGHTING SETUP (Full Phong Model with Toggles)
 * 
 * Sets up directional light (Sun/Moon), point lights (lamps), and spot lights.
 * Also sends toggle uniforms for Keys 1-3 (light sources) and 5-7 (components).
 */
void setupLighting(Shader& shader)
{
    // ========================================================================
    // SEND TOGGLE UNIFORMS (Keys 1-3 for light sources, 5-7 for components)
    // ========================================================================
    shader.setBool("enableDirLight", enableDirLight);       // Key 1
    shader.setBool("enablePointLights", enablePointLights); // Key 2
    shader.setBool("enableSpotLights", enableSpotLights);   // Key 3
    shader.setBool("enableAmbient", enableAmbient);         // Key 5
    shader.setBool("enableDiffuse", enableDiffuse);         // Key 6
    shader.setBool("enableSpecular", enableSpecular);       // Key 7
    
    // Set default emissive to zero (objects can override if self-illuminating)
    shader.setVec3("material.emissive", glm::vec3(0.0f));
    
    // ========================================================================
    // DIRECTIONAL LIGHT (Sun/Moon) - Key 1
    // ========================================================================
    if (isNight)
    {
        // Night lighting - moonlight
        shader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        shader.setVec3("dirLight.ambient", glm::vec3(0.15f, 0.15f, 0.2f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.3f, 0.3f, 0.4f));
        shader.setVec3("dirLight.specular", glm::vec3(0.1f, 0.1f, 0.15f));
    }
    else
    {
        // Day lighting - bright sun
        shader.setVec3("dirLight.direction", glm::vec3(-0.5f, -1.0f, -0.3f));
        shader.setVec3("dirLight.ambient", glm::vec3(0.5f, 0.5f, 0.45f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.9f, 0.9f, 0.85f));
        shader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));
    }

    // ========================================================================
    // POINT LIGHTS (Street Lamps) - Key 2
    // Attenuation: F_att = 1.0 / (Kc + Kl*d + Kq*d^2)
    // ========================================================================
    int numPoints = 0;
    bool lampsOn = streetlightsOverride ? streetlightsManualOn : isNight;
    
    if (lampsOn) {
        // Point lights at streetlight lamp positions (height 4.2 = top of pole)
        // Streetlights: (0,0,-12), (15,0,-12), (-15,0,-12), (0,0,12)
        
        // Lamp 1 - South of bridge
        shader.setVec3("pointLights[0].position", glm::vec3(0.0f, 4.2f, -12.0f));
        shader.setVec3("pointLights[0].color", glm::vec3(1.0f, 0.9f, 0.7f));
        shader.setFloat("pointLights[0].constant", 1.0f);
        shader.setFloat("pointLights[0].linear", 0.09f);
        shader.setFloat("pointLights[0].quadratic", 0.032f);
        numPoints++;
        
        // Lamp 2 - Near mosque
        shader.setVec3("pointLights[1].position", glm::vec3(15.0f, 4.2f, -12.0f));
        shader.setVec3("pointLights[1].color", glm::vec3(1.0f, 0.9f, 0.7f));
        shader.setFloat("pointLights[1].constant", 1.0f);
        shader.setFloat("pointLights[1].linear", 0.09f);
        shader.setFloat("pointLights[1].quadratic", 0.032f);
        numPoints++;
        
        // Lamp 3 - Near windmills
        shader.setVec3("pointLights[2].position", glm::vec3(-15.0f, 4.2f, -12.0f));
        shader.setVec3("pointLights[2].color", glm::vec3(1.0f, 0.95f, 0.8f));
        shader.setFloat("pointLights[2].constant", 1.0f);
        shader.setFloat("pointLights[2].linear", 0.09f);
        shader.setFloat("pointLights[2].quadratic", 0.032f);
        numPoints++;
        
        // Lamp 4 - North of bridge
        shader.setVec3("pointLights[3].position", glm::vec3(0.0f, 4.2f, 12.0f));
        shader.setVec3("pointLights[3].color", glm::vec3(1.0f, 0.9f, 0.7f));
        shader.setFloat("pointLights[3].constant", 1.0f);
        shader.setFloat("pointLights[3].linear", 0.09f);
        shader.setFloat("pointLights[3].quadratic", 0.032f);
        numPoints++;
        
        // CAMPFIRE LIGHT (Index 4)
        // Position matches the emissive object: (12.0, 0.5, -14.0)
        // Warm orange color, stronger falloff for local glow
        shader.setVec3("pointLights[4].position", glm::vec3(12.0f, 1.0f, -14.0f));  // Slightly above fire
        shader.setVec3("pointLights[4].color", glm::vec3(1.0f, 0.4f, 0.0f));     // Deep orange
        shader.setFloat("pointLights[4].constant", 1.0f);
        shader.setFloat("pointLights[4].linear", 0.14f);      // Faster falloff
        shader.setFloat("pointLights[4].quadratic", 0.07f);   // Localized light
        numPoints++;
        
        // Mosque inside light (Index 5)
        shader.setVec3("pointLights[5].position", glm::vec3(30.0f, 3.0f, 20.0f));
        shader.setVec3("pointLights[5].color", glm::vec3(1.0f, 0.95f, 0.8f));
        shader.setFloat("pointLights[5].constant", 1.0f);
        shader.setFloat("pointLights[5].linear", 0.09f);
        shader.setFloat("pointLights[5].quadratic", 0.032f);
        numPoints++;
        
        // Mosque exterior light (Index 6)
        shader.setVec3("pointLights[6].position", glm::vec3(30.0f, 5.0f, 20.0f));
        shader.setVec3("pointLights[6].color", glm::vec3(0.8f, 0.8f, 1.0f));
        shader.setFloat("pointLights[6].constant", 1.0f);
        shader.setFloat("pointLights[6].linear", 0.07f);
        shader.setFloat("pointLights[6].quadratic", 0.017f);
        numPoints++;
    }
    shader.setInt("numPointLights", numPoints);

    // ========================================================================
    // SPOT LIGHTS (Boat Searchlight) - Key 3
    // Always configured, controlled by enableSpotLights toggle (Key 3)
    // Point light with direction and cutoff angle (cos phi)
    // ========================================================================
    int numSpots = 1;  // Always have at least 1 spot light available
    // Boat searchlight - wider cone and lower attenuation for better visibility
    shader.setVec3("spotLights[0].position", glm::vec3(0.0f, 2.0f, 0.0f));
    shader.setVec3("spotLights[0].direction", glm::vec3(0.0f, -0.5f, 1.0f));
    shader.setVec3("spotLights[0].color", glm::vec3(1.5f, 1.5f, 1.3f));  // Brighter
    shader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(25.0f)));      // Wider inner cone
    shader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(35.0f))); // Wider outer cone
    shader.setFloat("spotLights[0].constant", 1.0f);
    shader.setFloat("spotLights[0].linear", 0.045f);    // Lower = less falloff
    shader.setFloat("spotLights[0].quadratic", 0.0075f); // Lower = reaches further
    shader.setInt("numSpotLights", numSpots);
}

// ============================================================================
// HELP FUNCTION
// ============================================================================

void printHelp()
{
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "           3D SMART VILLAGE - KEYBOARD CONTROLS\n";
    std::cout << "============================================================\n";
    std::cout << "\n";
    std::cout << "  TEXTURE EFFECTS (Keys 1-6):\n";
    std::cout << "    1             Toggle ALL textures ON/OFF\n";
    std::cout << "    2             Simple texture (no surface color)\n";
    std::cout << "    3             Blended texture (color on FRAGMENT - Phong)\n";
    std::cout << "    4             Blended texture (color on VERTEX - Gouraud)\n";
    std::cout << "    5             Toggle textured curvy surface (dome+cone)\n";
    std::cout << "    6             Toggle Ambient Light\n";
    std::cout << "    [/]           Decrease/Increase blend factor\n";
    std::cout << "\n";
    std::cout << "  LIGHTING (Keys 7-9):\n";
    std::cout << "    7             Toggle Directional Light (Sun/Moon)\n";
    std::cout << "    8             Toggle Point Lights (Street Lamps & Mosque Light)\n";
    std::cout << "    9             Toggle Spot Light\n";
    std::cout << "\n";
    std::cout << "  VIEWPORT:\n";
    std::cout << "    0             Toggle 4-Viewport split mode\n";
    std::cout << "\n";
    std::cout << "  INTERACTIVE OBJECTS:\n";
    std::cout << "    O             Open/Close house door\n";
    std::cout << "    I             Open/Close house windows\n";
    std::cout << "\n";
    std::cout << "  CAMERA MOVEMENT:\n";
    std::cout << "    W/S           Move forward/backward\n";
    std::cout << "    A/D           Move left/right\n";
    std::cout << "    Space         Move up\n";
    std::cout << "    Left Ctrl     Move down\n";
    std::cout << "    Left Shift    Fast movement\n";
    std::cout << "    C             Reset camera position\n";
    std::cout << "    R/T           Rotate scene right/left\n";
    std::cout << "\n";
    std::cout << "  SCENE:\n";
    std::cout << "    N             Toggle Day/Night mode\n";
    std::cout << "    L             Toggle streetlights\n";
    std::cout << "\n";
    std::cout << "  OTHER:\n";
    std::cout << "    H             Show this help message\n";
    std::cout << "    ESC           Exit application\n";
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "\n";
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

void processInput(GLFWwindow* window, std::vector<Windmill>& windmills /*, Boat& boat */)
{
    // ESC to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // ========================================================================
    // CAMERA MOVEMENT
    // ========================================================================
    
    // Check for shift key (fast movement)
    bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float speedMultiplier = shiftPressed ? 3.0f : 1.0f;
    float adjustedDelta = deltaTime * speedMultiplier;

    // Flying camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, adjustedDelta);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, adjustedDelta);

    // House door/window controls (toggle with O and I)
    static bool oKeyPrev = false, iKeyPrev = false;
    bool oKey = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
    bool iKey = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (oKey && !oKeyPrev) MudHouse::doorOpen = !MudHouse::doorOpen;
    if (iKey && !iKeyPrev) MudHouse::windowOpen = !MudHouse::windowOpen;
    oKeyPrev = oKey;
    iKeyPrev = iKey;

    // ========================================================================
    // TEXTURE CONTROLS (Keys 1-6)
    // ========================================================================

    // Key 1: Toggle ALL Textures ON/OFF
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    {
        if (!keyStates.Key1)
        {
            enableTextures = !enableTextures;
            std::cout << "[Key 1] Textures: " << (enableTextures ? "ON" : "OFF") << std::endl;
            keyStates.Key1 = true;
        }
    }
    else keyStates.Key1 = false;

    // Key 2: Simple Texture (no surface color)
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    {
        if (!keyStates.Key2)
        {
            if (enableTextures && textureBlendMode == 1) { // Toggle off to no textures
                enableTextures = false;
                std::cout << "[Key 2] Toggled OFF -> NO TEXTURES" << std::endl;
            } else {
                enableTextures = true;
                textureBlendMode = 1;
                std::cout << "[Key 2] Texture Mode: SIMPLE (texture only, no surface color)" << std::endl;
            }
            keyStates.Key2 = true;
        }
    }
    else keyStates.Key2 = false;

    // Key 3: Blended Texture - color computed on FRAGMENT (Phong)
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
    {
        if (!keyStates.Key3)
        {
            if (enableTextures && textureBlendMode == 0) { // Toggle off to no textures
                enableTextures = false;
                std::cout << "[Key 3] Toggled OFF -> NO TEXTURES" << std::endl;
            } else {
                enableTextures = true;
                textureBlendMode = 0;
                if (textureBlendFactor <= 0.0f) textureBlendFactor = 0.5f; // Restore visibility
                std::cout << "[Key 3] Texture Mode: BLENDED - color on FRAGMENT (Phong per-pixel)" << std::endl;
            }
            keyStates.Key3 = true;
        }
    }
    else keyStates.Key3 = false;

    // Key 4: Blended Texture - color computed on VERTEX (Gouraud)
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
    {
        if (!keyStates.Key4)
        {
            if (enableTextures && textureBlendMode == 2) { // Toggle off to no textures
                enableTextures = false;
                std::cout << "[Key 4] Toggled OFF -> NO TEXTURES" << std::endl;
            } else {
                enableTextures = true;
                textureBlendMode = 2;
                if (textureBlendFactor <= 0.0f) textureBlendFactor = 0.5f; // Restore visibility
                std::cout << "[Key 4] Texture Mode: BLENDED - color on VERTEX (Gouraud interpolated)" << std::endl;
            }
            keyStates.Key4 = true;
        }
    }
    else keyStates.Key4 = false;

    // Key 5: Toggle Textured Curvy Surface (dome sphere + windmill cone)
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
    {
        if (!keyStates.Key5)
        {
            enableCurvyTexture = !enableCurvyTexture;
            std::cout << "[Key 5] Textured Curvy Surface (Dome+Cone): " << (enableCurvyTexture ? "ON" : "OFF") << std::endl;
            keyStates.Key5 = true;
        }
    }
    else keyStates.Key5 = false;

    // Key 6: Toggle Ambient Light
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
    {
        if (!keyStates.Key6)
        {
            enableAmbient = !enableAmbient;
            std::cout << "[Key 6] Ambient Light: " << (enableAmbient ? "ON" : "OFF") << std::endl;
            keyStates.Key6 = true;
        }
    }
    else keyStates.Key6 = false;

    // ========================================================================
    // LIGHTING (Keys 7-9)
    // ========================================================================

    // Key 7: Toggle Directional Light
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
    {
        if (!keyStates.Key7)
        {
            enableDirLight = !enableDirLight;
            std::cout << "[Key 7] Directional Light: " << (enableDirLight ? "ON" : "OFF") << std::endl;
            keyStates.Key7 = true;
        }
    }
    else keyStates.Key7 = false;

    // Key 8: Toggle Point Lights
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
    {
        if (!keyStates.Key8)
        {
            enablePointLights = !enablePointLights;
            std::cout << "[Key 8] Point Lights: " << (enablePointLights ? "ON" : "OFF") << std::endl;
            keyStates.Key8 = true;
        }
    }
    else keyStates.Key8 = false;

    // Key 9: Toggle Spot Lights
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS)
    {
        if (!keyStates.Key9)
        {
            enableSpotLights = !enableSpotLights;
            std::cout << "[Key 9] Spot Lights: " << (enableSpotLights ? "ON" : "OFF") << std::endl;
            keyStates.Key9 = true;
        }
    }
    else keyStates.Key9 = false;

    // Key 0: Toggle 4-Viewport Mode
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
    {
        if (!keyStates.Key0)
        {
            fourViewportMode = !fourViewportMode;
            std::cout << "[Key 0] 4-Viewport Mode: " << (fourViewportMode ? "ON" : "OFF") << std::endl;
            keyStates.Key0 = true;
        }
    }
    else keyStates.Key0 = false;


    // Reset camera (C key)
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        if (!keyStates.C)
        {
            camera.Position = defaultCameraPos;
            camera.Yaw = defaultYaw;
            camera.Pitch = defaultPitch;
            camera.ProcessMouseMovement(0, 0); // Update vectors
            sceneRotation = 0.0f; // Also reset scene rotation
            std::cout << "Camera Reset to Default Position" << std::endl;
            keyStates.C = true;
        }
    }
    else keyStates.C = false;

    // ========================================================================
    // SCENE ROTATION CONTROLS
    // ========================================================================

    // Rotate Camera Right (R key) - Look Right
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(6.0f, 0.0f); // Simulate mouse movement right
    }

    // Rotate Camera Left (T key) - Look Left
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(-6.0f, 0.0f); // Simulate mouse movement left
    }

    // ========================================================================
    // LIGHTING CONTROLS
    // ========================================================================

    // Day/Night toggle (N key)
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        if (!keyStates.N)
        {
            isNight = !isNight;
            streetlightsOverride = false; // Reset manual override when toggling day/night
            std::cout << (isNight ? "Night Mode" : "Day Mode") << std::endl;
            keyStates.N = true;
        }
    }
    else keyStates.N = false;

    // Streetlights manual toggle (L key)
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        if (!keyStates.L)
        {
            if (!streetlightsOverride)
            {
                streetlightsOverride = true;
                streetlightsManualOn = !isNight; // Start with opposite of current state
            }
            else
            {
                streetlightsManualOn = !streetlightsManualOn;
            }
            std::cout << "Streetlights: " << (streetlightsManualOn ? "ON" : "OFF") << " (Manual Override)" << std::endl;
            keyStates.L = true;
        }
    }
    else keyStates.L = false;

    // ========================================================================
    // ANIMATION CONTROLS
    // ========================================================================

    // Pause all animations (P key)
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        if (!keyStates.P)
        {
            animationsPaused = !animationsPaused;
            std::cout << "All Animations: " << (animationsPaused ? "PAUSED" : "RUNNING") << std::endl;
            keyStates.P = true;
        }
    }
    else keyStates.P = false;

    // Pause boat (B key)
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
    {
        if (!keyStates.B)
        {
            boatPaused = !boatPaused;
            std::cout << "Boat: " << (boatPaused ? "PAUSED" : "MOVING") << std::endl;
            keyStates.B = true;
        }
    }
    else keyStates.B = false;

    // Pause windmills (V key)
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    {
        if (!keyStates.V)
        {
            windmillsPaused = !windmillsPaused;
            std::cout << "Windmills: " << (windmillsPaused ? "PAUSED" : "ROTATING") << std::endl;
            keyStates.V = true;
        }
    }
    else keyStates.V = false;

    // Windmill speed adjustment (+/- keys)
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
    {
        if (!keyStates.Plus)
        {
            windmillSpeedMultiplier = std::min(5.0f, windmillSpeedMultiplier + 0.25f);
            std::cout << "Windmill Speed: " << (windmillSpeedMultiplier * 100) << "%" << std::endl;
            keyStates.Plus = true;
        }
    }
    else keyStates.Plus = false;

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
    {
        if (!keyStates.Minus)
        {
            windmillSpeedMultiplier = std::max(0.0f, windmillSpeedMultiplier - 0.25f);
            std::cout << "Windmill Speed: " << (windmillSpeedMultiplier * 100) << "%" << std::endl;
            keyStates.Minus = true;
        }
    }
    else keyStates.Minus = false;

    // Boat speed adjustment ([ / ] keys)
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        if (!keyStates.BracketRight)
        {
            textureBlendFactor = std::min(1.0f, textureBlendFactor + 0.1f);
            std::cout << "[Key ]] Blend Factor: " << textureBlendFactor << std::endl;
            keyStates.BracketRight = true;
        }
    }
    else keyStates.BracketRight = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        if (!keyStates.BracketLeft)
        {
            textureBlendFactor = std::max(0.0f, textureBlendFactor - 0.1f);
            std::cout << "[Key [] Blend Factor: " << textureBlendFactor << std::endl;
            keyStates.BracketLeft = true;
        }
    }
    else keyStates.BracketLeft = false;

    // ========================================================================
    // RENDERING CONTROLS
    // ========================================================================



    // ========================================================================
    // BOAT MANUAL CONTROL (commented out)
    // ========================================================================

    // Arrow keys for Boat Control (Steering)
    // float boatMoveSpeed = 5.0f * deltaTime;
    // float boatTurnSpeed = 90.0f * deltaTime;
    // 
    // if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    // {
    //     boat.position.x += sin(glm::radians(boat.boatYaw)) * boatMoveSpeed;
    //     boat.position.z += cos(glm::radians(boat.boatYaw)) * boatMoveSpeed;
    // }
    // 
    // if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    // {
    //     boat.position.x -= sin(glm::radians(boat.boatYaw)) * boatMoveSpeed;
    //     boat.position.z -= cos(glm::radians(boat.boatYaw)) * boatMoveSpeed;
    // }
    // 
    // if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    // {
    //     boat.boatYaw += boatTurnSpeed;
    // }
    // 
    // if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    // {
    //     boat.boatYaw -= boatTurnSpeed;
    // }

    // ========================================================================
    // HELP
    // ========================================================================

    // Print help (H key)
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        if (!keyStates.H)
        {
            printHelp();
            keyStates.H = true;
        }
    }
    else keyStates.H = false;
}

// ============================================================================
// CALLBACK FUNCTIONS
// ============================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed: y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
