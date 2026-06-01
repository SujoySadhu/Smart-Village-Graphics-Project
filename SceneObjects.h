#ifndef SCENE_OBJECTS_H
#define SCENE_OBJECTS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"
#include "Mesh.h"
#include "BezierSurface.h"

/**
 * Material properties for Phong shading
 */
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    static Material Default() {
        return {
            glm::vec3(0.5f),               // Higher ambient
            glm::vec3(0.8f),
            glm::vec3(0.5f),
            32.0f
        };
    }

    static Material Water() {
        return {
            glm::vec3(0.3f, 0.4f, 0.5f),   // Higher ambient
            glm::vec3(0.2f, 0.4f, 0.6f),
            glm::vec3(0.9f, 0.9f, 0.9f),
            128.0f
        };
    }

    static Material Wood() {
        return {
            glm::vec3(0.5f, 0.4f, 0.35f),  // Higher ambient
            glm::vec3(0.6f, 0.4f, 0.25f),
            glm::vec3(0.2f, 0.15f, 0.1f),
            16.0f
        };
    }

    static Material Mud() {
        return {
            glm::vec3(0.45f, 0.4f, 0.35f), // Higher ambient
            glm::vec3(0.5f, 0.35f, 0.25f),
            glm::vec3(0.1f, 0.08f, 0.05f),
            8.0f
        };
    }

    static Material Metal() {
        return {
            glm::vec3(0.5f, 0.5f, 0.55f),  // Higher ambient
            glm::vec3(0.4f, 0.4f, 0.5f),
            glm::vec3(0.8f, 0.8f, 0.9f),
            64.0f
        };
    }

    static Material Grass() {
        return {
            glm::vec3(0.45f, 0.5f, 0.4f),  // Higher ambient
            glm::vec3(0.3f, 0.5f, 0.2f),
            glm::vec3(0.1f, 0.15f, 0.05f),
            8.0f
        };
    }

    static Material SolarPanel() {
        return {
            glm::vec3(0.3f, 0.3f, 0.4f),   // Higher ambient
            glm::vec3(0.1f, 0.1f, 0.3f),
            glm::vec3(0.6f, 0.6f, 0.8f),
            96.0f
        };
    }
};

/**
 * Windmill class with hierarchical transformation
 * 
 * HIERARCHICAL MODELING EXPLANATION:
 * The windmill consists of:
 * - Base (static) - Parent transform
 * - Tower (static) - Child of base
 * - Hub (rotates) - Child of tower
 * - Blades (4x) - Children of hub, rotate with it
 * 
 * Transformation hierarchy:
 * Model = BaseTransform * TowerTransform * HubRotation * BladeTransform
 */
class Windmill {
public:
    glm::vec3 position;
    float baseRotation;      // Rotation of the whole windmill (Y-axis)
    float bladeRotation;     // Current blade rotation angle
    float bladeSpeed;        // Rotation speed (degrees per second)

    // Cone/tower texture for curvy surface requirement
    static unsigned int coneTexture;
    static bool texturesLoaded;
    static void loadTextures();

    Windmill(glm::vec3 pos, float rotation = 0.0f, float speed = 45.0f);

    void Update(float deltaTime);
    void Draw(Shader& shader);

private:
    Mesh* base;
    Mesh* tower;
    Mesh* hub;
    Mesh* blade;

    void initMeshes();
};

/**
 * Village House with gable roof, double door, windows, solar panel
 *
 * Features:
 * - V-shaped gable roof with ridge along X
 * - Front wall with openings for door and two windows
 * - Back wall with two windows
 * - Double front door with panel details and knobs (O key toggle)
 * - Four windows with frames, sills, cross bars (I key toggle)
 * - Solar panel on front roof slope
 * - Red baseboard around perimeter
 * - Doorstep and ground path
 */
class MudHouse {
public:
    glm::vec3 position;
    float rotation;
    float scale;
    float doorAngle;     // 0 = closed, 90 = open
    float windowAngle;   // 0 = closed, 90 = open
    static bool doorOpen;
    static bool windowOpen;

    // Texture IDs
    static unsigned int bricksTexture;
    static unsigned int roofTexture;
    static unsigned int solarTexture;
    static unsigned int floorTexture;
    static bool texturesLoaded;
    static void loadTextures();

    MudHouse(glm::vec3 pos, float rot = 0.0f, float s = 1.0f);

    void Draw(Shader& shader);
    void Update(float deltaTime);

private:
    Mesh* cubeMesh;      // Shared unit cube for all box draws

    void initMeshes();

    // Internal drawing helpers
    void drawBox(Shader& shader, const glm::mat4& base, glm::vec3 color,
                 glm::vec3 pos, glm::vec3 rot, glm::vec3 scl,
                 unsigned int texId = 0, bool texOnly = false);
    void drawGround(Shader& shader, const glm::mat4& base);
    void drawFloor(Shader& shader, const glm::mat4& base);
    void drawWalls(Shader& shader, const glm::mat4& base);
    void drawDoor(Shader& shader, const glm::mat4& base);
    void drawWindows(Shader& shader, const glm::mat4& base);
    void drawRoof(Shader& shader, const glm::mat4& base);
    void drawSolarPanel(Shader& shader, const glm::mat4& base);
};

/**
 * Boat that moves along the river
 */
class Boat {
public:
    glm::vec3 position;
    glm::vec3 startPos;
    glm::vec3 endPos;
    float speed;
    float progress;  // 0 to 1
    bool movingForward;
    float oarAngle;  // Rowing animation
    
    // New members for procedural boat
    float boatYaw;
    float boatPitch;
    float boatRoll;
    float wavePhase;
    
    // Spline path along the river
    std::vector<glm::vec3> splinePath;

    Boat(glm::vec3 start, glm::vec3 end, float s = 2.0f);
    // Initialize Boat with Spline path
    Boat(std::vector<glm::vec3> path, float s = 2.0f);
    ~Boat(); // Added destructor to clean up GL resources

    void Update(float deltaTime);
    void Draw(Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos);

private:
    // Custom rendering resources
    unsigned int VAO, VBO;
    unsigned int shaderProgram;
    int vertexCount;

    void initMeshes(); // This will now call generateBoat
    void generateBoat();
    
    // Geometry helpers
    void pushVert(std::vector<float>& buffer, glm::vec3 p, glm::vec3 n, glm::vec3 c);
    void addTri(std::vector<float>& buffer, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 color);
    void addQuad(std::vector<float>& buffer, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 color);
    void addBox(std::vector<float>& buffer, glm::vec3 c, glm::vec3 s, glm::vec3 col);
    void addMesh(std::vector<float>& buffer, const class Mesh& mesh, const glm::mat4& transform, glm::vec3 col);
};

/**
 * Streetlight that activates at night
 */
class Streetlight {
public:
    glm::vec3 position;
    glm::vec3 lightColor;
    float lightIntensity;
    bool isOn;

    // Spotlight properties
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    Streetlight(glm::vec3 pos);

    void SetNightMode(bool night);
    void Draw(Shader& shader);
    void ApplyLighting(Shader& shader, int lightIndex);

private:
    Mesh* pole;
    Mesh* lamp;

    void initMeshes();
};

/**
 * Bridge connecting two sides of the river
 */
class Bridge {
public:
    glm::vec3 position;
    float length;
    float width;
    float rotation;

    Bridge(glm::vec3 pos, float len, float wid, float rot = 0.0f);

    void Draw(Shader& shader);

    Mesh* deckTop;
    Mesh* deckBot;
    Mesh* deckSideL;
    Mesh* deckSideR;
    
    Mesh* railTop;
    Mesh* railBot;
    Mesh* railSideL;
    Mesh* railSideR;

    Mesh* support;

    void initMeshes();
};


/**
 * Water Tank / Mosque Dome using Bezier surface of revolution
 */
class WaterTank {
public:
    glm::vec3 position;
    float scale;

    WaterTank(glm::vec3 pos, float s = 1.0f);

    void Draw(Shader& shader);

private:
    Mesh* tank;
    Mesh* base;

    void initMeshes();
};

/**
 * Mosque with dome using Bezier surface
 * Design based on traditional Islamic architecture:
 * - Main rectangular building with large central dome
 * - Arched windows on sides
 * - Front entrance with large arch
 * - Tall minaret tower with small dome on top
 * - Crescent finial on main dome
 */
class MosqueDome {
public:
    glm::vec3 position;
    float scale;

    // Texture IDs
    static unsigned int mosqueWallTexture;
    static unsigned int mosqueFloorTexture;
    static unsigned int mosqueDomeTexture;
    static unsigned int mosqueWindowTexture;
    static bool texturesLoaded;
    static void loadTextures();

    MosqueDome(glm::vec3 pos, float s = 1.0f);

    void Draw(Shader& shader);

private:
    Mesh* mainDome;
    Mesh* smallDome;
    Mesh* mainBuilding;
    Mesh* frontSection;
    Mesh* minaret;
    Mesh* minaretTop;
    Mesh* archWindow;
    Mesh* mainArch;
    Mesh* crescent;
    Mesh* domeBase;

    void initMeshes();
};

/**
 * Terrain with grass/dirt
 */
class Terrain {
public:
    float width;
    float depth;
    glm::vec3 position;

    Terrain(float w, float d, glm::vec3 pos = glm::vec3(0.0f));

    void Draw(Shader& shader);

    // Grass texture
    static unsigned int grassTexture;
    static bool grassTextureLoaded;
    static void loadGrassTexture();

private:
    Mesh* ground;

    void initMesh();
};

/**
 * River with high specular reflection and 3D waves
 */
class River {
public:
    std::vector<glm::vec3> splinePath;
    float width;
    float flowOffset;  // For animated texture
    float waveTime;    // For wave animation

    // Water texture
    static unsigned int waterTexture;
    static bool waterTextureLoaded;
    static void loadWaterTexture();

    River(std::vector<glm::vec3> path, float w);

    void Update(float deltaTime);
    void Draw(Shader& shader);           // Draw banks and bed (opaque)
    void DrawWaterSurface(Shader& shader); // Draw water surface (transparent)

private:
    Mesh* water;

    void initMesh();
};

/**
 * Tree with trunk and textured foliage
 * Uses tree_shade1/2/3.png textures for foliage texture mapping
 */
class Tree {
public:
    glm::vec3 position;
    float scale;
    float rotation;

    Tree(glm::vec3 pos, float s = 1.0f, float rot = 0.0f);

    void Draw(Shader& shader);

    // Static texture loading for tree shade textures
    static unsigned int treeTextures[3];
    static unsigned int leafTexture;
    static bool texturesLoaded;
    static void loadTextures();

private:
    Mesh* trunk;
    Mesh* foliage;
    Mesh* branches;
    int textureIndex; // Which of the 3 textures this tree uses

    void initMeshes();
};

/**
 * Bird struct for individual bird state
 */
struct Bird {
    glm::vec3 position;
    float speed;
    float flapPhase;       // Current wing flap phase
    float flapSpeed;       // Wing flap frequency
    float circleRadius;    // Radius of circular flight path
    float circleAngle;     // Current angle on circular path
    float circleHeight;    // Flight altitude
    float circleSpeed;     // Angular speed around the circle
    float bankAngle;       // Current banking angle for turning
};

/**
 * BirdFlock - Animated flock of birds flying through the sky
 * 
 * Features:
 * - Procedural V-shaped bird geometry with flapping wings
 * - Each bird follows a unique circular path at different heights
 * - Wing flapping animation with natural phase variation
 * - Banking into turns for realistic flight
 */
class BirdFlock {
public:
    std::vector<Bird> birds;
    glm::vec3 flockCenter;   // Center of the flock's flight area

    BirdFlock(glm::vec3 center, int numBirds = 12);

    void Update(float deltaTime);
    void Draw(Shader& shader);

private:
    Mesh* bodyMesh;
    Mesh* wingMesh;

    void initMeshes();
};

#endif
