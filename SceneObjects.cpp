#include "SceneObjects.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration for texture loading helper (defined in Tree section)
static unsigned int loadSingleTexture(const char* path);

// Global texture toggle from main.cpp (Key 1)
extern bool enableTextures;
// Global texture blend mode from main.cpp (Key 2/3): 0=blended, 1=simple
extern int textureBlendMode;
// Global curvy surface texture toggle (Key 4)
extern bool enableCurvyTexture;
// Global water texture toggle (Key 5)
extern bool enableWaterTexture;
// Global texture blend factor ([/] keys)
extern float textureBlendFactor;

// ============================================================================
// WINDMILL IMPLEMENTATION
// ============================================================================

static Mesh* s_windmillBase = nullptr;
static Mesh* s_windmillTower = nullptr;
static Mesh* s_windmillHub = nullptr;
static Mesh* s_windmillBlade = nullptr;

unsigned int Windmill::coneTexture = 0;
bool Windmill::texturesLoaded = false;

void Windmill::loadTextures() {
    if (texturesLoaded) return;
    stbi_set_flip_vertically_on_load(true);
    coneTexture = loadSingleTexture("src/bricks_texture2.png");
    texturesLoaded = true;
}

Windmill::Windmill(glm::vec3 pos, float rotation, float speed)
    : position(pos), baseRotation(rotation), bladeRotation(0.0f), bladeSpeed(speed) {
    initMeshes();
    loadTextures();
}

void Windmill::initMeshes() {
    // Create shared meshes only once
    if (!s_windmillBase) {
        s_windmillBase = new Mesh(Mesh::CreateCylinder(1.5f, 0.5f, 24));
    }
    if (!s_windmillTower) {
        s_windmillTower = new Mesh(Mesh::CreateCylinder(0.8f, 6.0f, 32));
    }
    if (!s_windmillHub) {
        s_windmillHub = new Mesh(Mesh::CreateSphere(0.5f, 24, 24));
    }
    if (!s_windmillBlade) {
        // Create a blade shape (elongated box)
        std::vector<Vertex> bladeVerts;
        std::vector<unsigned int> bladeInds;
        
        // Blade dimensions
        float length = 3.0f;
        float width = 0.3f;
        float thickness = 0.1f;
        
        // Create a tapered blade
        bladeVerts = {
            // Front face
            {{0, -thickness, -width/2}, {0, 0, -1}, {0, 0}},
            {{length, -thickness/2, -width/4}, {0, 0, -1}, {1, 0}},
            {{length, thickness/2, -width/4}, {0, 0, -1}, {1, 1}},
            {{0, thickness, -width/2}, {0, 0, -1}, {0, 1}},
            // Back face
            {{0, -thickness, width/2}, {0, 0, 1}, {0, 0}},
            {{length, -thickness/2, width/4}, {0, 0, 1}, {1, 0}},
            {{length, thickness/2, width/4}, {0, 0, 1}, {1, 1}},
            {{0, thickness, width/2}, {0, 0, 1}, {0, 1}},
            // Top face
            {{0, thickness, -width/2}, {0, 1, 0}, {0, 0}},
            {{length, thickness/2, -width/4}, {0, 1, 0}, {1, 0}},
            {{length, thickness/2, width/4}, {0, 1, 0}, {1, 1}},
            {{0, thickness, width/2}, {0, 1, 0}, {0, 1}},
            // Bottom face
            {{0, -thickness, -width/2}, {0, -1, 0}, {0, 0}},
            {{length, -thickness/2, -width/4}, {0, -1, 0}, {1, 0}},
            {{length, -thickness/2, width/4}, {0, -1, 0}, {1, 1}},
            {{0, -thickness, width/2}, {0, -1, 0}, {0, 1}},
        };
        
        bladeInds = {
            0, 1, 2, 2, 3, 0,
            4, 7, 6, 6, 5, 4,
            8, 9, 10, 10, 11, 8,
            12, 15, 14, 14, 13, 12,
        };
        
        s_windmillBlade = new Mesh(bladeVerts, bladeInds);
    }
    
    base = s_windmillBase;
    tower = s_windmillTower;
    hub = s_windmillHub;
    blade = s_windmillBlade;
}

void Windmill::Update(float deltaTime) {
    bladeRotation += bladeSpeed * deltaTime;
    if (bladeRotation >= 360.0f) {
        bladeRotation -= 360.0f;
    }
}

void Windmill::Draw(Shader& shader) {
    Material woodMat = Material::Wood();
    shader.setVec3("material.ambient", woodMat.ambient);
    shader.setVec3("material.diffuse", woodMat.diffuse);
    shader.setVec3("material.specular", woodMat.specular);
    shader.setFloat("material.shininess", woodMat.shininess);

    // Base transformation (parent of all)
    glm::mat4 baseTransform = glm::mat4(1.0f);
    baseTransform = glm::translate(baseTransform, position);
    baseTransform = glm::rotate(baseTransform, glm::radians(baseRotation), glm::vec3(0, 1, 0));

    // Draw base (CONE - textured curvy surface)
    glm::mat4 baseModel = baseTransform;
    baseModel = glm::translate(baseModel, glm::vec3(0, 0.25f, 0));
    shader.setMat4("model", baseModel);
    // Apply cone texture (gated by enableCurvyTexture + enableTextures)
    if (coneTexture != 0 && enableTextures && enableCurvyTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, coneTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", textureBlendMode == 1);
    }
    base->Draw();
    shader.setBool("useTexture", false);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Tower transformation (child of base)
    glm::mat4 towerTransform = baseTransform;
    towerTransform = glm::translate(towerTransform, glm::vec3(0, 3.25f, 0));
    shader.setMat4("model", towerTransform);
    tower->Draw();

    // Hub transformation (child of tower, rotates with blades)
    glm::mat4 hubTransform = towerTransform;
    hubTransform = glm::translate(hubTransform, glm::vec3(0, 2.5f, 0.9f));
    
    // Apply blade rotation around Z-axis (facing forward)
    glm::mat4 hubRotation = glm::rotate(glm::mat4(1.0f), glm::radians(bladeRotation), glm::vec3(0, 0, 1));
    hubTransform = hubTransform * hubRotation;
    
    shader.setMat4("model", hubTransform);
    hub->Draw();

    // Draw 4 blades (children of hub)
    Material metalMat = Material::Metal();
    shader.setVec3("material.ambient", metalMat.ambient);
    shader.setVec3("material.diffuse", metalMat.diffuse);
    shader.setVec3("material.specular", metalMat.specular);
    shader.setFloat("material.shininess", metalMat.shininess);

    for (int i = 0; i < 4; ++i) {
        glm::mat4 bladeTransform = hubTransform;
        // Rotate each blade 90 degrees apart
        bladeTransform = glm::rotate(bladeTransform, glm::radians(90.0f * i), glm::vec3(0, 0, 1));
        // Offset blade from hub center
        bladeTransform = glm::translate(bladeTransform, glm::vec3(0.3f, 0, 0));
        
        shader.setMat4("model", bladeTransform);
        blade->Draw();
    }
}

// ============================================================================
// VILLAGE HOUSE IMPLEMENTATION (Gable Roof, Double Door, Windows, Solar Panel)
// ============================================================================

static Mesh* s_houseCube = nullptr;

bool MudHouse::doorOpen = false;
bool MudHouse::windowOpen = false;
unsigned int MudHouse::bricksTexture = 0;
unsigned int MudHouse::roofTexture = 0;
unsigned int MudHouse::solarTexture = 0;
unsigned int MudHouse::floorTexture = 0;
bool MudHouse::texturesLoaded = false;

void MudHouse::loadTextures() {
    if (texturesLoaded) return;
    stbi_set_flip_vertically_on_load(true);
    bricksTexture = loadSingleTexture("src/bricks_texture2.png");
    roofTexture = loadSingleTexture("src/tin_shade.png");
    solarTexture = loadSingleTexture("src/solar_panel.png");
    floorTexture = loadSingleTexture("src/mosque_floor1.png");
    texturesLoaded = true;
}

MudHouse::MudHouse(glm::vec3 pos, float rot, float s)
    : position(pos), rotation(rot), scale(s), doorAngle(0), windowAngle(0) {
    initMeshes();
}

void MudHouse::initMeshes() {
    if (!s_houseCube) {
        s_houseCube = new Mesh(Mesh::CreateCube(1.0f));
    }
    cubeMesh = s_houseCube;
    loadTextures();
}

void MudHouse::Update(float dt) {
    float dT = doorOpen ? 90.0f : 0.0f;
    float wT = windowOpen ? 90.0f : 0.0f;
    float sp = 120.0f;
    if (doorAngle < dT) doorAngle = std::min(doorAngle + sp * dt, dT);
    else if (doorAngle > dT) doorAngle = std::max(doorAngle - sp * dt, dT);
    if (windowAngle < wT) windowAngle = std::min(windowAngle + sp * dt, wT);
    else if (windowAngle > wT) windowAngle = std::max(windowAngle - sp * dt, wT);
}

// -- drawBox: sets material + model matrix + optional texture, draws cube --
void MudHouse::drawBox(Shader& shader, const glm::mat4& base, glm::vec3 color,
                       glm::vec3 pos, glm::vec3 rot, glm::vec3 scl,
                       unsigned int texId, bool texOnly) {
    shader.setVec3("material.ambient", color * 0.5f);
    shader.setVec3("material.diffuse", color);
    shader.setVec3("material.specular", glm::vec3(0.5f));
    shader.setFloat("material.shininess", 32.0f);
    shader.setVec3("material.emissive", glm::vec3(0.0f));

    // Pass material to vertex shader for Gouraud mode (Key 4)
    if (textureBlendMode == 2) {
        shader.setVec3("vertexMatAmbient", color * 0.5f);
        shader.setVec3("vertexMatDiffuse", color);
    }

    // Texture binding (respects global toggle)
    if (texId != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        // Mode: 0=fragment-blend, 1=simple(textureOnly), 2=vertex-blend(useVertexColor)
        shader.setBool("textureOnly", textureBlendMode == 1 ? true : (textureBlendMode == 2 ? false : texOnly));
    } else {
        shader.setBool("useTexture", false);
        shader.setBool("textureOnly", false);
    }

    glm::mat4 m = base;
    m = glm::translate(m, pos);
    m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, scl);
    shader.setMat4("model", m);
    cubeMesh->Draw();

    // Reset texture state
    if (texId != 0 && enableTextures) {
        shader.setBool("useTexture", false);
        shader.setBool("textureOnly", false);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void MudHouse::Draw(Shader& shader) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, position);
    base = glm::rotate(base, glm::radians(rotation), glm::vec3(0, 1, 0));
    base = glm::scale(base, glm::vec3(scale));

    // Disable culling for the house (thin walls need both sides)
    GLboolean cullingOn;
    glGetBooleanv(GL_CULL_FACE, &cullingOn);
    glDisable(GL_CULL_FACE);

    drawGround(shader, base);
    drawFloor(shader, base);
    drawWalls(shader, base);
    drawDoor(shader, base);
    drawWindows(shader, base);
    drawRoof(shader, base);
    drawSolarPanel(shader, base);

    if (cullingOn) glEnable(GL_CULL_FACE);
}

// ==================== HOUSE CONSTANTS ====================
static const float H_WALL_H = 3.5f;
static const float H_WALL_W = 10.0f;
static const float H_WALL_D = 6.0f;
static const float H_WALL_T = 0.15f;
static const float H_FRONT_Z = H_WALL_D / 2.0f;
static const float H_BACK_Z = -H_WALL_D / 2.0f;
static const float H_LEFT_X = -H_WALL_W / 2.0f;
static const float H_RIGHT_X = H_WALL_W / 2.0f;

// ==================== COLORS ====================
static const glm::vec3 cWall(0.85f, 0.82f, 0.78f);  // Light cream (neutral for better texture visibility)
static const glm::vec3 cRoof(0.55f, 0.55f, 0.55f);
static const glm::vec3 cDoor(0.55f, 0.38f, 0.18f);
static const glm::vec3 cWindowFrame(0.50f, 0.35f, 0.15f);
static const glm::vec3 cWindowGlass(0.55f, 0.75f, 0.85f);
static const glm::vec3 cBaseboard(0.60f, 0.15f, 0.10f);
static const glm::vec3 cGrass(0.25f, 0.50f, 0.15f);
static const glm::vec3 cPath(0.55f, 0.50f, 0.45f);
static const glm::vec3 cFloor(0.80f, 0.72f, 0.60f);
static const glm::vec3 cSolar(0.40f, 0.45f, 0.65f);  // Lighter blue for solar panel texture
static const glm::vec3 cGround(0.45f, 0.42f, 0.38f);

// ==================== GROUND ====================
void MudHouse::drawGround(Shader& shader, const glm::mat4& base) {
    drawBox(shader, base, cPath, glm::vec3(0, 0.01f, 6.5f), glm::vec3(0), glm::vec3(14, 0.08f, 2.5f));
    drawBox(shader, base, cPath, glm::vec3(0, 0.01f, 5.0f), glm::vec3(0), glm::vec3(2.5f, 0.06f, 2.0f));
    drawBox(shader, base, cGround, glm::vec3(0, 0.005f, 8.5f), glm::vec3(0), glm::vec3(14, 0.15f, 3));
}

// ==================== FLOOR ====================
void MudHouse::drawFloor(Shader& shader, const glm::mat4& base) {
    drawBox(shader, base, cFloor,
        glm::vec3(0, 0.05f, 0), glm::vec3(0),
        glm::vec3(H_WALL_W - H_WALL_T * 2, 0.1f, H_WALL_D - H_WALL_T * 2),
        floorTexture, false);  // Blended texture with surface color
}

// ==================== WALLS ====================
void MudHouse::drawWalls(Shader& shader, const glm::mat4& base) {
    unsigned int wTex = bricksTexture;  // Blended bricks texture for all walls
    // Back wall (solid)
    drawBox(shader, base, cWall,
        glm::vec3(0, H_WALL_H / 2, H_BACK_Z), glm::vec3(0), glm::vec3(H_WALL_W, H_WALL_H, H_WALL_T), wTex, false);
    // Left wall
    drawBox(shader, base, cWall,
        glm::vec3(H_LEFT_X, H_WALL_H / 2, 0), glm::vec3(0), glm::vec3(H_WALL_T, H_WALL_H, H_WALL_D), wTex, false);
    // Right wall
    drawBox(shader, base, cWall,
        glm::vec3(H_RIGHT_X, H_WALL_H / 2, 0), glm::vec3(0), glm::vec3(H_WALL_T, H_WALL_H, H_WALL_D), wTex, false);

    // Front wall - panels with gaps for door & windows
    float doorW = 1.8f, doorH = 2.5f;
    float winW = 1.4f, winH = 1.4f, winBot = 1.3f;

    float flx = H_LEFT_X + H_WALL_T / 2;
    float lwl = -3.2f - winW / 2;
    float panelW = lwl - flx;
    drawBox(shader, base, cWall,
        glm::vec3((flx + lwl) / 2, H_WALL_H / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(panelW, H_WALL_H, H_WALL_T), wTex, false);

    float lwr = -3.2f + winW / 2;
    float dl = -doorW / 2;
    panelW = dl - lwr;
    drawBox(shader, base, cWall,
        glm::vec3((lwr + dl) / 2, H_WALL_H / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(panelW, H_WALL_H, H_WALL_T), wTex, false);

    float dr = doorW / 2;
    float rwl = 3.2f - winW / 2;
    panelW = rwl - dr;
    drawBox(shader, base, cWall,
        glm::vec3((dr + rwl) / 2, H_WALL_H / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(panelW, H_WALL_H, H_WALL_T), wTex, false);

    float rwr = 3.2f + winW / 2;
    float frx = H_RIGHT_X - H_WALL_T / 2;
    panelW = frx - rwr;
    drawBox(shader, base, cWall,
        glm::vec3((rwr + frx) / 2, H_WALL_H / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(panelW, H_WALL_H, H_WALL_T), wTex, false);

    // Above door
    drawBox(shader, base, cWall,
        glm::vec3(0, (doorH + H_WALL_H) / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(doorW, H_WALL_H - doorH, H_WALL_T), wTex, false);

    // Below/Above left window
    drawBox(shader, base, cWall,
        glm::vec3(-3.2f, winBot / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(winW, winBot, H_WALL_T), wTex, false);
    float winTop = winBot + winH;
    drawBox(shader, base, cWall,
        glm::vec3(-3.2f, (winTop + H_WALL_H) / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(winW, H_WALL_H - winTop, H_WALL_T), wTex, false);

    // Below/Above right window
    drawBox(shader, base, cWall,
        glm::vec3(3.2f, winBot / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(winW, winBot, H_WALL_T), wTex, false);
    drawBox(shader, base, cWall,
        glm::vec3(3.2f, (winTop + H_WALL_H) / 2, H_FRONT_Z), glm::vec3(0), glm::vec3(winW, H_WALL_H - winTop, H_WALL_T), wTex, false);

    // Red baseboard (all 4 sides)
    drawBox(shader, base, cBaseboard,
        glm::vec3(0, 0.15f, H_FRONT_Z + H_WALL_T / 2 + 0.01f), glm::vec3(0), glm::vec3(H_WALL_W + 0.1f, 0.3f, 0.04f));
    drawBox(shader, base, cBaseboard,
        glm::vec3(0, 0.15f, H_BACK_Z - H_WALL_T / 2 - 0.01f), glm::vec3(0), glm::vec3(H_WALL_W + 0.1f, 0.3f, 0.04f));
    drawBox(shader, base, cBaseboard,
        glm::vec3(H_LEFT_X - H_WALL_T / 2 - 0.01f, 0.15f, 0), glm::vec3(0), glm::vec3(0.04f, 0.3f, H_WALL_D + 0.1f));
    drawBox(shader, base, cBaseboard,
        glm::vec3(H_RIGHT_X + H_WALL_T / 2 + 0.01f, 0.15f, 0), glm::vec3(0), glm::vec3(0.04f, 0.3f, H_WALL_D + 0.1f));
}

// ==================== DOOR ====================
void MudHouse::drawDoor(Shader& shader, const glm::mat4& base) {
    float doorH = 2.5f, doorW = 0.85f;
    float fz = H_FRONT_Z + H_WALL_T / 2.0f;
    glm::vec3 frameColor = cDoor * 0.55f;

    // Door frame
    drawBox(shader, base, frameColor,
        glm::vec3(0, doorH + 0.06f, fz + 0.01f), glm::vec3(0), glm::vec3(doorW * 2 + 0.3f, 0.12f, H_WALL_T + 0.04f));
    drawBox(shader, base, frameColor,
        glm::vec3(-doorW - 0.08f, doorH / 2, fz + 0.01f), glm::vec3(0), glm::vec3(0.12f, doorH + 0.12f, H_WALL_T + 0.04f));
    drawBox(shader, base, frameColor,
        glm::vec3(doorW + 0.08f, doorH / 2, fz + 0.01f), glm::vec3(0), glm::vec3(0.12f, doorH + 0.12f, H_WALL_T + 0.04f));

    if (doorAngle < 1.0f) {
        // CLOSED door panels
        drawBox(shader, base, cDoor,
            glm::vec3(-doorW / 2, doorH / 2, fz + 0.02f), glm::vec3(0), glm::vec3(doorW - 0.02f, doorH, 0.08f));
        drawBox(shader, base, cDoor,
            glm::vec3(doorW / 2, doorH / 2, fz + 0.02f), glm::vec3(0), glm::vec3(doorW - 0.02f, doorH, 0.08f));
        // Center divider
        drawBox(shader, base, cDoor * 0.4f,
            glm::vec3(0, doorH / 2, fz + 0.06f), glm::vec3(0), glm::vec3(0.06f, doorH, 0.02f));
        // Panel insets
        for (float panel : {-doorW / 2, doorW / 2}) {
            drawBox(shader, base, cDoor * 0.75f,
                glm::vec3(panel, doorH * 0.7f, fz + 0.07f), glm::vec3(0), glm::vec3(doorW * 0.6f, doorH * 0.35f, 0.02f));
            drawBox(shader, base, cDoor * 0.75f,
                glm::vec3(panel, doorH * 0.25f, fz + 0.07f), glm::vec3(0), glm::vec3(doorW * 0.6f, doorH * 0.3f, 0.02f));
        }
        // Knobs
        drawBox(shader, base, glm::vec3(0.8f, 0.7f, 0.2f),
            glm::vec3(-0.1f, doorH * 0.45f, fz + 0.08f), glm::vec3(0), glm::vec3(0.08f, 0.08f, 0.06f));
        drawBox(shader, base, glm::vec3(0.8f, 0.7f, 0.2f),
            glm::vec3(0.1f, doorH * 0.45f, fz + 0.08f), glm::vec3(0), glm::vec3(0.08f, 0.08f, 0.06f));
    } else {
        // OPEN: panels swing inward at 90 degrees
        drawBox(shader, base, cDoor,
            glm::vec3(-doorW + 0.02f, doorH / 2, fz - doorW / 2), glm::vec3(0, 90, 0),
            glm::vec3(doorW - 0.02f, doorH, 0.08f));
        drawBox(shader, base, cDoor,
            glm::vec3(doorW - 0.02f, doorH / 2, fz - doorW / 2), glm::vec3(0, -90, 0),
            glm::vec3(doorW - 0.02f, doorH, 0.08f));
    }

    // Doorstep
    drawBox(shader, base, cPath,
        glm::vec3(0, 0.08f, H_FRONT_Z + H_WALL_T / 2 + 0.35f), glm::vec3(0), glm::vec3(2.4f, 0.16f, 0.6f));
}

// ==================== WINDOWS ====================
void MudHouse::drawWindows(Shader& shader, const glm::mat4& base) {
    float winH = 1.4f, winW = 1.4f, winY = 1.3f + winH / 2;
    float fz = H_FRONT_Z + H_WALL_T / 2.0f;

    auto drawOneWindow = [&](float x, float z, float outDir) {
        // Frame
        drawBox(shader, base, cWindowFrame,
            glm::vec3(x, winY + winH / 2 + 0.04f, z), glm::vec3(0), glm::vec3(winW + 0.16f, 0.08f, H_WALL_T + 0.04f));
        drawBox(shader, base, cWindowFrame,
            glm::vec3(x, winY - winH / 2 - 0.04f, z), glm::vec3(0), glm::vec3(winW + 0.16f, 0.08f, H_WALL_T + 0.04f));
        drawBox(shader, base, cWindowFrame,
            glm::vec3(x - winW / 2 - 0.04f, winY, z), glm::vec3(0), glm::vec3(0.08f, winH + 0.24f, H_WALL_T + 0.04f));
        drawBox(shader, base, cWindowFrame,
            glm::vec3(x + winW / 2 + 0.04f, winY, z), glm::vec3(0), glm::vec3(0.08f, winH + 0.24f, H_WALL_T + 0.04f));

        // Sill
        drawBox(shader, base, cWindowFrame * 0.85f,
            glm::vec3(x, winY - winH / 2 - 0.1f, z + outDir * 0.08f), glm::vec3(0),
            glm::vec3(winW + 0.24f, 0.06f, 0.16f));

        if (windowAngle < 1.0f) {
            // CLOSED: glass + cross bars
            drawBox(shader, base, cWindowGlass,
                glm::vec3(x, winY, z + outDir * 0.01f), glm::vec3(0), glm::vec3(winW, winH, 0.03f));
            drawBox(shader, base, cWindowFrame * 0.8f,
                glm::vec3(x, winY, z + outDir * 0.02f), glm::vec3(0), glm::vec3(0.04f, winH, 0.02f));
            drawBox(shader, base, cWindowFrame * 0.8f,
                glm::vec3(x, winY, z + outDir * 0.02f), glm::vec3(0), glm::vec3(winW, 0.04f, 0.02f));
        } else {
            // OPEN: panes swung outward at 90 degrees
            float halfW = winW / 2 - 0.02f;
            drawBox(shader, base, cWindowGlass,
                glm::vec3(x - winW / 2 + 0.01f, winY, z + outDir * halfW / 2), glm::vec3(0, outDir * 90, 0),
                glm::vec3(halfW, winH, 0.03f));
            drawBox(shader, base, cWindowGlass,
                glm::vec3(x + winW / 2 - 0.01f, winY, z + outDir * halfW / 2), glm::vec3(0, -outDir * 90, 0),
                glm::vec3(halfW, winH, 0.03f));
        }
    };

    // Front windows
    drawOneWindow(-3.2f, fz, 1.0f);
    drawOneWindow(3.2f, fz, 1.0f);

    // Back windows
    float bz = H_BACK_Z - H_WALL_T / 2.0f;
    drawOneWindow(-2.5f, bz, -1.0f);
    drawOneWindow(2.5f, bz, -1.0f);
}

// ==================== ROOF (V-shape gable, ridge along X) ====================
void MudHouse::drawRoof(Shader& shader, const glm::mat4& base) {
    float xOverhang = 0.6f;
    float roofRise = 2.0f;
    float halfDepth = H_WALL_D / 2.0f;
    float roofW = H_WALL_W + xOverhang * 2;
    float roofT = 0.12f;
    float peakY = H_WALL_H + roofRise;
    float slopeLen = sqrt(roofRise * roofRise + halfDepth * halfDepth);
    float angle = glm::degrees(atan2(roofRise, halfDepth));

    // Front slope - simple texture (tin shade, no surface color)
    float centerZ = H_FRONT_Z / 2.0f;
    float centerY = (H_WALL_H + peakY) / 2.0f;
    drawBox(shader, base, cRoof,
        glm::vec3(0, centerY, centerZ), glm::vec3(angle, 0, 0), glm::vec3(roofW, roofT, slopeLen),
        roofTexture, true);  // Simple texture: no surface color

    // Back slope
    drawBox(shader, base, cRoof,
        glm::vec3(0, centerY, -centerZ), glm::vec3(-angle, 0, 0), glm::vec3(roofW, roofT, slopeLen),
        roofTexture, true);  // Simple texture: no surface color

    // Ridge beam
    drawBox(shader, base, cRoof * 0.7f,
        glm::vec3(0, peakY + 0.05f, 0), glm::vec3(0), glm::vec3(roofW + 0.1f, 0.12f, 0.2f));

    // Gable walls (triangular fill on left/right sides)
    int steps = 50;
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)steps;
        float y = H_WALL_H + t * roofRise;
        float gD = H_WALL_D * (1.0f - t);
        float slabH = roofRise / steps + 0.02f;
        if (gD > 0.1f) {
            drawBox(shader, base, cWall,
                glm::vec3(H_LEFT_X, y + slabH / 2, 0), glm::vec3(0), glm::vec3(H_WALL_T + 0.05f, slabH, gD), bricksTexture, false);
            drawBox(shader, base, cWall,
                glm::vec3(H_RIGHT_X, y + slabH / 2, 0), glm::vec3(0), glm::vec3(H_WALL_T + 0.05f, slabH, gD), bricksTexture, false);
        }
    }
}

// ==================== SOLAR PANEL ====================
void MudHouse::drawSolarPanel(Shader& shader, const glm::mat4& base) {
    float panelW = 2.5f, panelD = 1.8f, panelT = 0.08f;
    float roofRise = 2.0f;
    float halfDepth = H_WALL_D / 2.0f;
    float roofAngle = glm::degrees(atan2(roofRise, halfDepth));
    float roofT = 0.12f;

    float t = 0.4f;
    float panelZ = H_FRONT_Z * (1.0f - t);
    float panelY = H_WALL_H + roofRise * t + roofT / 2.0f + panelT / 2.0f + 0.05f;

    drawBox(shader, base, cSolar,
        glm::vec3(1.5f, panelY, panelZ), glm::vec3(roofAngle, 0, 0),
        glm::vec3(panelW, panelT, panelD),
        solarTexture, false);  // Blended texture with surface color
}


// ============================================================================
// BOAT IMPLEMENTATION
// Professional Bangladesh River Nouka (High-Fidelity Low-Poly)
// ============================================================================

// Custom Shaders for the Boat
const char* boatVShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vFragPos = vec3(model * vec4(aPos, 1.0));
    vNormal = mat3(transpose(inverse(model))) * aNormal; 
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* boatFShaderSrc = R"(
#version 330 core
out vec4 FragColor;

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 viewPos;

// Toggle uniforms (matches main shader - Assignment Requirement)
uniform bool enableDirLight;
uniform bool enableAmbient;
uniform bool enableDiffuse;
uniform bool enableSpecular;

void main() {
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - vFragPos);

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // 1. Key Light (The Sun - Warm) - Only if directional light enabled
    if (enableDirLight) {
        vec3 sunDir = normalize(vec3(0.5, 1.0, 0.3));
        vec3 sunColor = vec3(1.0, 0.95, 0.8);
        
        // Diffuse (Half-Lambert / Wrapped Diffuse)
        if (enableDiffuse) {
            float diff = dot(norm, sunDir) * 0.5 + 0.5;
            diffuse = diff * sunColor;
        }
        
        // Specular
        if (enableSpecular) {
            vec3 reflectDir = reflect(-sunDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            specular = vec3(0.3) * spec;
        }
    }

    // 2. Fill Light (The Sky - Ambient)
    if (enableAmbient) {
        vec3 skyColor = vec3(0.5, 0.6, 0.8);
        ambient = skyColor * 0.5;
    }

    // Combine
    vec3 lighting = ambient + diffuse + specular;
    FragColor = vec4(lighting * vColor, 1.0);
}
)";

Boat::Boat(glm::vec3 start, glm::vec3 end, float s)
    : startPos(start), endPos(end), speed(s), progress(0.0f), movingForward(true), oarAngle(0.0f),
      boatYaw(90.0f), boatPitch(0.0f), boatRoll(0.0f), wavePhase(0.0f),
      VAO(0), VBO(0), shaderProgram(0), vertexCount(0) {
    position = start;
    initMeshes();
}

Boat::Boat(std::vector<glm::vec3> path, float s)
    : splinePath(path), speed(s), progress(0.0f), movingForward(true), oarAngle(0.0f),
      boatYaw(0.0f), boatPitch(0.0f), boatRoll(0.0f), wavePhase(0.0f),
      VAO(0), VBO(0), shaderProgram(0), vertexCount(0) {
    if(splinePath.size() >= 4) {
        startPos = splinePath[1];
        endPos = splinePath[splinePath.size()-2];
    }
    position = startPos;
    initMeshes();
}

Boat::~Boat() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

// --- Geometry Helpers ---
glm::vec3 calcNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    return glm::normalize(glm::cross(p2 - p1, p3 - p1));
}

void Boat::pushVert(std::vector<float>& buffer, glm::vec3 p, glm::vec3 n, glm::vec3 c) {
    buffer.insert(buffer.end(), { p.x, p.y, p.z, n.x, n.y, n.z, c.x, c.y, c.z });
}

void Boat::addTri(std::vector<float>& buffer, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 color) {
    glm::vec3 n = calcNormal(p1, p2, p3);
    pushVert(buffer, p1, n, color);
    pushVert(buffer, p2, n, color);
    pushVert(buffer, p3, n, color);
}

void Boat::addQuad(std::vector<float>& buffer, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 color) {
    addTri(buffer, p1, p2, p3, color);
    addTri(buffer, p1, p3, p4, color);
}

void Boat::addBox(std::vector<float>& buffer, glm::vec3 c, glm::vec3 s, glm::vec3 col) {
    glm::vec3 h = s * 0.5f;
    glm::vec3 p1(c.x - h.x, c.y - h.y, c.z + h.z), p2(c.x + h.x, c.y - h.y, c.z + h.z);
    glm::vec3 p3(c.x + h.x, c.y + h.y, c.z + h.z), p4(c.x - h.x, c.y + h.y, c.z + h.z);
    glm::vec3 p5(c.x - h.x, c.y - h.y, c.z - h.z), p6(c.x + h.x, c.y - h.y, c.z - h.z);
    glm::vec3 p7(c.x + h.x, c.y + h.y, c.z - h.z), p8(c.x - h.x, c.y + h.y, c.z - h.z);
    addQuad(buffer, p1, p2, p3, p4, col); addQuad(buffer, p6, p5, p8, p7, col);
    addQuad(buffer, p4, p3, p7, p8, col); addQuad(buffer, p5, p6, p2, p1, col);
    addQuad(buffer, p5, p1, p4, p8, col); addQuad(buffer, p2, p6, p7, p3, col);
}

void Boat::addMesh(std::vector<float>& buffer, const Mesh& mesh, const glm::mat4& transform, glm::vec3 col) {
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        Vertex v1 = mesh.vertices[mesh.indices[i]];
        Vertex v2 = mesh.vertices[mesh.indices[i+1]];
        Vertex v3 = mesh.vertices[mesh.indices[i+2]];

        glm::vec3 p1 = glm::vec3(transform * glm::vec4(v1.Position, 1.0f));
        glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(transform)));
        glm::vec3 n1 = glm::normalize(normalMat * v1.Normal);
        
        glm::vec3 p2 = glm::vec3(transform * glm::vec4(v2.Position, 1.0f));
        glm::vec3 n2 = glm::normalize(normalMat * v2.Normal);
        
        glm::vec3 p3 = glm::vec3(transform * glm::vec4(v3.Position, 1.0f));
        glm::vec3 n3 = glm::normalize(normalMat * v3.Normal);
        
        pushVert(buffer, p1, n1, col);
        pushVert(buffer, p2, n2, col);
        pushVert(buffer, p3, n3, col);
    }
}

void Boat::generateBoat() {
    std::vector<float> vertexBuffer;

    // Palette
    glm::vec3 cHullDark(0.35f, 0.20f, 0.10f);
    glm::vec3 cHullLight(0.50f, 0.30f, 0.15f);
    glm::vec3 cFloor(0.55f, 0.40f, 0.25f);
    glm::vec3 cRim(0.25f, 0.12f, 0.05f);
    glm::vec3 cBamboo(0.75f, 0.65f, 0.40f);
    glm::vec3 cMat(0.65f, 0.55f, 0.35f);
    glm::vec3 cRed(0.60f, 0.15f, 0.10f);
    glm::vec3 cMetal(0.3f, 0.3f, 0.35f);

    // Hull Parameters
    int segments = 40; 
    float length = 4.0f; 
    float width = 1.1f; 
    float depth = 0.70f;
    
    struct Slice { glm::vec3 keel, floor, rim; };
    std::vector<Slice> slices;

    // 1. GENERATE HULL PROFILE
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments; float mt = (t - 0.5f) * 2.0f;
        float wFactor = 1.0f - pow(abs(mt), 2.5f);
        float hFactor = pow(abs(mt), 2.2f) * 0.7f;
        float x = mt * (length * 0.5f);
        float yKeel = hFactor; float yRim = hFactor + depth;
        slices.push_back({ glm::vec3(x, yKeel, 0.0f), glm::vec3(x, yKeel + 0.45f, width * 0.4f * wFactor), glm::vec3(x, yRim, width * 0.5f * wFactor) });
    }

    // 2. BUILD HULL MESH
    for (int i = 0; i < segments; i++) {
        Slice s1 = slices[i]; Slice s2 = slices[i + 1];
        // Solid color for hull (no stripes)
        glm::vec3 col = cHullDark; 
        
        // Outer Bottom
        addQuad(vertexBuffer, s1.keel, s2.keel, glm::vec3(s2.floor.x, s2.floor.y, s2.floor.z), glm::vec3(s1.floor.x, s1.floor.y, s1.floor.z), col);
        // Negative side: reversed vertex order for correct -Z facing normal
        addQuad(vertexBuffer, 
            glm::vec3(s1.keel.x, s1.keel.y, -s1.keel.z), 
            glm::vec3(s1.floor.x, s1.floor.y, -s1.floor.z), 
            glm::vec3(s2.floor.x, s2.floor.y, -s2.floor.z), 
            glm::vec3(s2.keel.x, s2.keel.y, -s2.keel.z), 
            col);
        // Outer Sides
        addQuad(vertexBuffer, s1.floor, s2.floor, s2.rim, s1.rim, col);
        // Negative side: Use addQuad with REVERSED vertex order for proper mirroring
        // Positive side was: s1.floor, s2.floor, s2.rim, s1.rim (diagonal s1.floor->s2.rim)
        // Negative side uses: s1.floor_neg, s1.rim_neg, s2.rim_neg, s2.floor_neg (same diagonal, correct normal)
        addQuad(vertexBuffer, 
            glm::vec3(s1.floor.x, s1.floor.y, -s1.floor.z),  // Back-Bottom
            glm::vec3(s1.rim.x, s1.rim.y, -s1.rim.z),        // Back-Top
            glm::vec3(s2.rim.x, s2.rim.y, -s2.rim.z),        // Front-Top
            glm::vec3(s2.floor.x, s2.floor.y, -s2.floor.z),  // Front-Bottom
            col);

        // Inner Hull
        if (s1.rim.z > 0.05f) {
            float th = 0.06f;
            glm::vec3 iRim1 = s1.rim - glm::vec3(0, 0, th); glm::vec3 iRim2 = s2.rim - glm::vec3(0, 0, th);
            glm::vec3 iFl1 = s1.floor - glm::vec3(0, -0.02f, 0.02f); glm::vec3 iFl2 = s2.floor - glm::vec3(0, -0.02f, 0.02f);
            addQuad(vertexBuffer, s1.rim, s2.rim, iRim2, iRim1, cRim); // Gunwale
            addQuad(vertexBuffer, iRim1, iRim2, iFl2, iFl1, cHullLight); // Wall
            addQuad(vertexBuffer, iFl1, iFl2, glm::vec3(iFl2.x, iFl2.y, -iFl2.z), glm::vec3(iFl1.x, iFl1.y, -iFl1.z), cFloor); // Floor
            // Mirror
            addQuad(vertexBuffer, glm::vec3(s2.rim.x, s2.rim.y, -s2.rim.z), glm::vec3(s1.rim.x, s1.rim.y, -s1.rim.z), glm::vec3(iRim1.x, iRim1.y, -iRim1.z), glm::vec3(iRim2.x, iRim2.y, -iRim2.z), cRim);
            addQuad(vertexBuffer, glm::vec3(iRim1.x, iRim1.y, -iRim1.z), glm::vec3(iRim2.x, iRim2.y, -iRim2.z), glm::vec3(iFl2.x, iFl2.y, -iFl2.z), glm::vec3(iFl1.x, iFl1.y, -iFl1.z), cHullLight);
        }
        // Ribs
        if (i % 4 == 0 && s1.rim.z > 0.2f) addBox(vertexBuffer, glm::vec3(s1.floor.x, s1.floor.y + 0.03f, 0), glm::vec3(0.04f, 0.04f, s1.floor.z * 1.8f), cRim);
    }

    // 3. THE CHHAUNI (Bamboo Roof)
    int roofStart = segments / 2 - 6; int roofEnd = segments / 2 + 4;
    for (int i = roofStart; i < roofEnd; i++) {
        Slice s1 = slices[i]; Slice s2 = slices[i + 1];
        float r1 = s1.rim.z; float r2 = s2.rim.z;
        int arcRes = 6;
        for (int j = 0; j < arcRes; j++) {
            float ang1 = (float)j / arcRes * 3.14159f; float ang2 = (float)(j + 1) / arcRes * 3.14159f;
            glm::vec3 p1(s1.rim.x, s1.rim.y + sin(ang1) * r1 * 0.8f, cos(ang1) * r1);
            glm::vec3 p2(s2.rim.x, s2.rim.y + sin(ang1) * r2 * 0.8f, cos(ang1) * r2);
            glm::vec3 p3(s2.rim.x, s2.rim.y + sin(ang2) * r2 * 0.8f, cos(ang2) * r2);
            glm::vec3 p4(s1.rim.x, s1.rim.y + sin(ang2) * r1 * 0.8f, cos(ang2) * r1);
            // Solid color for roof
            glm::vec3 matCol = cBamboo; 
            addQuad(vertexBuffer, p1, p2, p3, p4, matCol);
        }
    }

    // 4. THE HARIKEN (Lantern)
    glm::vec3 lampPos = slices[roofEnd - 1].rim; lampPos.y += 0.0f; lampPos.z = 0.0f;
    addBox(vertexBuffer, glm::vec3(lampPos.x, lampPos.y - 0.1f, 0), glm::vec3(0.01f, 0.2f, 0.01f), glm::vec3(0.8f, 0.7f, 0.5f));
    addBox(vertexBuffer, glm::vec3(lampPos.x, lampPos.y - 0.25f, 0), glm::vec3(0.12f, 0.1f, 0.12f), cMetal);
    addBox(vertexBuffer, glm::vec3(lampPos.x, lampPos.y - 0.15f, 0), glm::vec3(0.08f, 0.15f, 0.08f), glm::vec3(0.9f, 0.9f, 0.6f));
    addBox(vertexBuffer, glm::vec3(lampPos.x, lampPos.y - 0.08f, 0), glm::vec3(0.06f, 0.02f, 0.06f), cMetal);

    // 5. GOLUI (Tips)
    Slice front = slices[segments]; glm::vec3 tip = front.rim + glm::vec3(0.3f, 0.2f, 0.0f);
    addTri(vertexBuffer, front.rim, front.keel, tip, cRim);
    addTri(vertexBuffer, front.keel, glm::vec3(front.rim.x, front.rim.y, -front.rim.z), tip, cRim);
    addBox(vertexBuffer, tip, glm::vec3(0.05f, 0.05f, 0.05f), cRed);
    Slice back = slices[0]; glm::vec3 bTip = back.rim + glm::vec3(-0.35f, 0.25f, 0.0f);
    addTri(vertexBuffer, back.rim, back.keel, bTip, cRim);
    addTri(vertexBuffer, back.keel, glm::vec3(back.rim.x, back.rim.y, -back.rim.z), bTip, cRim);

    // 6. BOATMAN (Human figure sitting at the stern, driving the boat)
    // Boat is 4.0 long (X: -2 to +2), 1.1 wide (Z: -0.55 to +0.55)
    // Roof covers roughly X: -0.6 to +0.8. Boatman sits at stern behind roof.
    {
        // Colors
        glm::vec3 cSkin(0.72f, 0.55f, 0.40f);
        glm::vec3 cSkinDark(0.60f, 0.45f, 0.32f);
        glm::vec3 cShirt(0.85f, 0.85f, 0.80f);
        glm::vec3 cLungi(0.15f, 0.30f, 0.55f);
        glm::vec3 cLungiStripe(0.10f, 0.22f, 0.45f);
        glm::vec3 cHair(0.08f, 0.06f, 0.05f);
        glm::vec3 cPaddle(0.50f, 0.35f, 0.18f);
        glm::vec3 cPaddleBlade(0.40f, 0.28f, 0.14f);

        // Boatman base position - stern area behind the roof
        float bx = -1.15f;  // X position (well into the stern)
        float by = 0.42f;   // Y base (sitting on the boat floor)
        float bz = 0.0f;    // Centered on Z

        // Boatman (Real human-like using spheres and cylinders)
        Mesh cylinderMesh = Mesh::CreateCylinder(0.5f, 1.0f, 12);
        Mesh sphereMesh = Mesh::CreateSphere(0.5f, 12, 12);
        
        auto addCapsule = [&](glm::vec3 c, glm::vec3 dir, float length, float radius, glm::vec3 color) {
            glm::mat4 t = glm::mat4(1.0f);
            t = glm::translate(t, c);
            glm::vec3 udir = glm::normalize(dir);
            glm::vec3 yAxis = glm::vec3(0, 1, 0);
            if (glm::abs(glm::dot(udir, yAxis)) < 0.999f) {
                glm::vec3 axis = glm::cross(yAxis, udir);
                float angle = acos(glm::dot(yAxis, udir));
                t = glm::rotate(t, angle, glm::normalize(axis));
            } else if (udir.y < -0.99f) {
                t = glm::rotate(t, glm::radians(180.0f), glm::vec3(1, 0, 0));
            }
            t = glm::scale(t, glm::vec3(radius * 2.0f, length, radius * 2.0f));
            addMesh(vertexBuffer, cylinderMesh, t, color);
        };
        
        auto addSphereScaled = [&](glm::vec3 c, glm::vec3 s, glm::vec3 color) {
            glm::mat4 t = glm::mat4(1.0f);
            t = glm::translate(t, c);
            t = glm::scale(t, s * 2.0f);
            addMesh(vertexBuffer, sphereMesh, t, color);
        };

        // --- ARMS (calculated first to align hands with paddle) ---
        glm::vec3 rShoulder(bx, by + 0.66f, -0.22f);
        glm::vec3 lShoulder(bx, by + 0.66f, 0.22f);
        glm::vec3 rHand(bx + 0.44f, by + 0.42f, -0.22f);
        glm::vec3 lHand(bx + 0.34f, by + 0.41f, -0.20f);
        
        glm::vec3 rElbow(bx + 0.20f, by + 0.52f, -0.12f);
        glm::vec3 lElbow(bx + 0.15f, by + 0.52f, -0.05f);

        // Right arm
        addCapsule((rShoulder + rElbow) * 0.5f, rElbow - rShoulder, glm::length(rElbow - rShoulder), 0.04f, cSkin);
        addCapsule((rElbow + rHand) * 0.5f, rHand - rElbow, glm::length(rHand - rElbow), 0.035f, cSkin);
        addSphereScaled(rHand, glm::vec3(0.04f), cSkin);

        // Left arm
        addCapsule((lShoulder + lElbow) * 0.5f, lElbow - lShoulder, glm::length(lElbow - lShoulder), 0.04f, cSkin);
        addCapsule((lElbow + lHand) * 0.5f, lHand - lElbow, glm::length(lHand - lElbow), 0.035f, cSkin);
        addSphereScaled(lHand, glm::vec3(0.04f), cSkin);

        // --- LEGS ---
        glm::vec3 rHip(bx, by + 0.30f, -0.12f);
        glm::vec3 lHip(bx, by + 0.30f, 0.12f);
        glm::vec3 rKnee(bx + 0.35f, by + 0.20f, -0.12f);
        glm::vec3 lKnee(bx + 0.35f, by + 0.20f, 0.12f);
        glm::vec3 rAnkle(bx + 0.60f, by + 0.08f, -0.12f);
        glm::vec3 lAnkle(bx + 0.60f, by + 0.08f, 0.12f);
        glm::vec3 rToe(bx + 0.75f, by + 0.06f, -0.12f);
        glm::vec3 lToe(bx + 0.75f, by + 0.06f, 0.12f);

        addCapsule((rHip + rKnee) * 0.5f, rKnee - rHip, glm::length(rKnee - rHip), 0.07f, cLungi);
        addCapsule((lHip + lKnee) * 0.5f, lKnee - lHip, glm::length(lKnee - lHip), 0.07f, cLungi);
        addCapsule((rKnee + rAnkle) * 0.5f, rAnkle - rKnee, glm::length(rAnkle - rKnee), 0.05f, cSkin);
        addCapsule((lKnee + lAnkle) * 0.5f, lAnkle - lKnee, glm::length(lAnkle - lKnee), 0.05f, cSkin);
        addSphereScaled((rAnkle + rToe) * 0.5f, glm::vec3(0.08f, 0.03f, 0.04f), cSkinDark);
        addSphereScaled((lAnkle + lToe) * 0.5f, glm::vec3(0.08f, 0.03f, 0.04f), cSkinDark);

        // --- TORSO ---
        glm::vec3 pelvis(bx, by + 0.35f, bz);
        glm::vec3 spineBase(bx, by + 0.45f, bz);
        glm::vec3 spineTop(bx, by + 0.65f, bz);
        
        addCapsule((pelvis + spineBase) * 0.5f, spineBase - pelvis, 0.25f, 0.15f, cShirt);
        addCapsule((spineBase + spineTop) * 0.5f, spineTop - spineBase, 0.30f, 0.16f, cShirt);

        // SHOULDERS
        addSphereScaled(rShoulder, glm::vec3(0.07f), cShirt);
        addSphereScaled(lShoulder, glm::vec3(0.07f), cShirt);

        // --- NECK & HEAD ---
        glm::vec3 neck(bx, by + 0.75f, bz);
        glm::vec3 head(bx, by + 0.90f, bz);
        addCapsule((spineTop + neck) * 0.5f, neck - spineTop, 0.15f, 0.04f, cSkin);
        
        addSphereScaled(head, glm::vec3(0.12f, 0.14f, 0.11f), cSkin);
        addSphereScaled(head + glm::vec3(0.0f, 0.12f, 0.0f), glm::vec3(0.13f, 0.05f, 0.12f), cHair);
        addSphereScaled(head + glm::vec3(-0.10f, 0.02f, 0.0f), glm::vec3(0.05f, 0.11f, 0.10f), cHair);
        addSphereScaled(head + glm::vec3(0.12f, -0.02f, 0.0f), glm::vec3(0.03f, 0.04f, 0.03f), cSkinDark);

        // --- PADDLE / STEERING OAR ---
        glm::vec3 pStart(bx + 0.48f, by + 0.45f, -0.18f); // Top of shaft
        glm::vec3 pEnd(bx - 1.25f, by - 0.05f, -0.50f);   // Bottom of shaft at blade
        glm::vec3 pCenter = (pStart + pEnd) * 0.5f;
        glm::vec3 pDir = pEnd - pStart;
        float pLen = glm::length(pDir);
        
        // Main continuous shaft using a single cylinder
        addCapsule(pCenter, pDir, pLen, 0.03f, cPaddle);
        // Grip
        addCapsule(glm::vec3(bx + 0.44f, by + 0.42f, -0.22f), pDir, 0.16f, 0.035f, cPaddle);
        // Paddle blade
        addSphereScaled(glm::vec3(bx - 1.30f, by - 0.06f, -0.52f), glm::vec3(0.06f, 0.20f, 0.12f), cPaddleBlade);
    }

    // Upload to GPU
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(float), vertexBuffer.data(), GL_STATIC_DRAW);

    // Stride is 9 floats (3 pos, 3 norm, 3 color)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    vertexCount = static_cast<int>(vertexBuffer.size() / 9);
}

void Boat::initMeshes() {
    // Compile Custom Shader
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &boatVShaderSrc, NULL);
    glCompileShader(v);
    
    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &boatFShaderSrc, NULL);
    glCompileShader(f);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, v);
    glAttachShader(shaderProgram, f);
    glLinkProgram(shaderProgram);

    glDeleteShader(v);
    glDeleteShader(f);

    generateBoat();
}

void Boat::Update(float deltaTime) {
    wavePhase += deltaTime;
    
    if (splinePath.size() >= 4) {
        int segments = splinePath.size() - 3;
        float totalProgress = progress * segments; // e.g. 0 to 4
        int currentSegment = static_cast<int>(totalProgress);
        if (currentSegment >= segments) currentSegment = segments - 1;
        float segmentT = totalProgress - currentSegment;

        glm::vec3 p0 = splinePath[currentSegment];
        glm::vec3 p1 = splinePath[currentSegment + 1];
        glm::vec3 p2 = splinePath[currentSegment + 2];
        glm::vec3 p3 = splinePath[currentSegment + 3];

        glm::vec3 newPos = BezierSurface::EvaluateCatmullRomSpline(p0, p1, p2, p3, segmentT);
        
        // Calculate forward direction for yaw
        glm::vec3 forward = glm::normalize(newPos - position);
        if (glm::length(forward) > 0.001f) {
            boatYaw = glm::degrees(atan2(forward.x, forward.z)) + 90.0f; // Add 90 if boat faces right naturally
        }
        
        position = newPos;

        float distance = glm::length(p2 - p1);
        float step = (speed * deltaTime) / (distance * segments);

        if (movingForward) {
            progress += step;
            if (progress >= 1.0f) { progress = 1.0f; movingForward = false; }
        } else {
            progress -= step;
            if (progress <= 0.0f) { progress = 0.0f; movingForward = true; }
        }
    } else {
        // Fallback linear
        float distance = glm::length(endPos - startPos);
        float step = (speed * deltaTime) / distance;
        
        if (movingForward) {
            progress += step;
            if (progress >= 1.0f) {
                progress = 1.0f;
                movingForward = false;
            }
        } else {
            progress -= step;
            if (progress <= 0.0f) {
                progress = 0.0f;
                movingForward = true;
            }
        }
        
        position = glm::mix(startPos, endPos, progress);
    }
    
    // Animate oars (rowing motion)
    oarAngle += deltaTime * 120.0f;
    if (oarAngle >= 360.0f) oarAngle -= 360.0f;
    
    // Slight rocking from waves
    boatPitch = sin(wavePhase) * 2.0f;
    boatRoll = cos(wavePhase * 1.5f) * 2.0f;
}

void Boat::Draw(Shader& ignoredShader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos) {
    // Use our custom shader
    glUseProgram(shaderProgram);
    
    // Disable backface culling for the boat (it has thin walls/roof)
    GLboolean cullingOn;
    glGetBooleanv(GL_CULL_FACE, &cullingOn);
    glDisable(GL_CULL_FACE);
    
    // Setup uniforms
    glm::mat4 model = glm::mat4(1.0f);
    // Apply position + wave bobbing
    model = glm::translate(model, position + glm::vec3(0, sin(wavePhase * 2.0f) * 0.05f - 0.25f, 0)); 
    
    // Boat orientation
    model = glm::rotate(model, glm::radians(boatYaw), glm::vec3(0, 1, 0)); 
    
    // Physics rocking
    model = glm::rotate(model, glm::radians(boatPitch), glm::vec3(1, 0, 0)); 
    model = glm::rotate(model, glm::radians(boatRoll), glm::vec3(0, 0, 1));

    // Send uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
    
    // Send toggle uniforms (these are defined in main.cpp as global variables)
    // We use extern to access them
    extern bool enableDirLight, enableAmbient, enableDiffuse, enableSpecular;
    glUniform1i(glGetUniformLocation(shaderProgram, "enableDirLight"), enableDirLight);
    glUniform1i(glGetUniformLocation(shaderProgram, "enableAmbient"), enableAmbient);
    glUniform1i(glGetUniformLocation(shaderProgram, "enableDiffuse"), enableDiffuse);
    glUniform1i(glGetUniformLocation(shaderProgram, "enableSpecular"), enableSpecular);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    
    // Restore culling state
    if (cullingOn) glEnable(GL_CULL_FACE);
}

// ============================================================================
// STREETLIGHT IMPLEMENTATION
// ============================================================================

static Mesh* s_lightPole = nullptr;
static Mesh* s_lightLamp = nullptr;

Streetlight::Streetlight(glm::vec3 pos)
    : position(pos),
      lightColor(1.0f, 0.9f, 0.7f),
      lightIntensity(0.0f),
      isOn(false),
      direction(0, -1, 0),
      cutOff(glm::cos(glm::radians(25.0f))),
      outerCutOff(glm::cos(glm::radians(35.0f))) {
    initMeshes();
}

void Streetlight::initMeshes() {
    if (!s_lightPole) {
        s_lightPole = new Mesh(Mesh::CreateCylinder(0.1f, 4.0f, 24));
    }
    if (!s_lightLamp) {
        s_lightLamp = new Mesh(Mesh::CreateSphere(0.2f, 16, 24));
    }
    
    pole = s_lightPole;
    lamp = s_lightLamp;
}

void Streetlight::SetNightMode(bool night) {
    isOn = night;
    lightIntensity = night ? 1.0f : 0.0f;
}

void Streetlight::Draw(Shader& shader) {
    // Pole material - dark metal, NO glow
    Material metalMat = Material::Metal();
    shader.setVec3("material.ambient", metalMat.ambient);
    shader.setVec3("material.diffuse", metalMat.diffuse);
    shader.setVec3("material.specular", metalMat.specular);
    shader.setFloat("material.shininess", metalMat.shininess);
    shader.setVec3("material.emissive", glm::vec3(0.0f));  // Pole does NOT glow

    // Draw pole
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::translate(model, glm::vec3(0, 2.0f, 0));
    shader.setMat4("model", model);
    pole->Draw();

    // Draw lamp (emissive when on - creates glowing effect)
    glm::mat4 lampModel = glm::mat4(1.0f);
    lampModel = glm::translate(lampModel, position);
    lampModel = glm::translate(lampModel, glm::vec3(0, 4.2f, 0));
    
    if (isOn) {
        // Lamp glows bright when light is on
        shader.setVec3("material.ambient", lightColor * 0.8f);
        shader.setVec3("material.diffuse", lightColor);
        shader.setVec3("material.specular", glm::vec3(1.0f));
        shader.setVec3("material.emissive", lightColor * 1.5f);  // BRIGHT GLOW
    } else {
        // Lamp is dim when off
        shader.setVec3("material.ambient", glm::vec3(0.2f, 0.2f, 0.25f));
        shader.setVec3("material.diffuse", glm::vec3(0.3f, 0.3f, 0.35f));
        shader.setVec3("material.specular", glm::vec3(0.1f));
        shader.setVec3("material.emissive", glm::vec3(0.0f));  // No glow
    }
    
    shader.setMat4("model", lampModel);
    lamp->Draw();
}

void Streetlight::ApplyLighting(Shader& shader, int lightIndex) {
    std::string prefix = "spotLights[" + std::to_string(lightIndex) + "].";
    
    shader.setVec3(prefix + "position", position + glm::vec3(0, 4.2f, 0));
    shader.setVec3(prefix + "direction", direction);
    shader.setVec3(prefix + "color", lightColor * lightIntensity);
    shader.setFloat(prefix + "cutOff", cutOff);
    shader.setFloat(prefix + "outerCutOff", outerCutOff);
    shader.setFloat(prefix + "constant", 1.0f);
    shader.setFloat(prefix + "linear", 0.09f);
    shader.setFloat(prefix + "quadratic", 0.032f);
}

// ============================================================================
// BRIDGE IMPLEMENTATION
// ============================================================================

static Mesh* s_bridgeDeck = nullptr;
static Mesh* s_bridgeRailing = nullptr;
static Mesh* s_bridgeSupport = nullptr;

Bridge::Bridge(glm::vec3 pos, float len, float wid, float rot)
    : position(pos), length(len), width(wid), rotation(rot) {
    initMeshes();
}

void Bridge::initMeshes() {
    if (!s_bridgeSupport) {
        s_bridgeSupport = new Mesh(Mesh::CreateCylinder(0.2f, 2.0f, 24));
    }
    support = s_bridgeSupport;
    
    // Create an arched bridge deck using Ruled Surfaces for 3D thickness
    std::vector<glm::vec3> cBotL, cBotR, cTopL, cTopR;
    int segs = 30;
    float halfWidth = 2.0f;
    float halfLen = 6.0f;
    float deckThick = 0.2f;
    
    for(int i = 0; i <= segs; i++) {
        float t = (float)i / segs;
        float x = (t - 0.5f) * halfLen * 2.0f;
        // Arch profile (parabola)
        float normalizedX = (x / halfLen);
        float y = (1.0f - normalizedX * normalizedX) * 1.5f; 
        
        cTopL.push_back(glm::vec3(x, y, -halfWidth));
        cTopR.push_back(glm::vec3(x, y, halfWidth));
        cBotL.push_back(glm::vec3(x, y - deckThick, -halfWidth));
        cBotR.push_back(glm::vec3(x, y - deckThick, halfWidth));
    }
    
    deckTop = new Mesh(BezierSurface::CreateRuledSurface(cTopL, cTopR, 10));
    deckBot = new Mesh(BezierSurface::CreateRuledSurface(cBotR, cBotL, 10)); // Reversed for normal direction
    deckSideL = new Mesh(BezierSurface::CreateRuledSurface(cBotL, cTopL, 2));
    deckSideR = new Mesh(BezierSurface::CreateRuledSurface(cTopR, cBotR, 2));

    // Create 3D Ruled Surface for Railings (A solid curved wall)
    std::vector<glm::vec3> rBotL, rBotR, rTopL, rTopR;
    float rWidth = 0.08f;
    float rHeight = 0.6f;
    
    for(int i = 0; i <= segs; i++) {
        float t = (float)i / segs;
        float x = (t - 0.5f) * halfLen * 2.0f;
        float normalizedX = (x / halfLen);
        float deckY = (1.0f - normalizedX * normalizedX) * 1.5f; 
        
        rTopL.push_back(glm::vec3(x, deckY + rHeight, -rWidth/2.0f));
        rTopR.push_back(glm::vec3(x, deckY + rHeight, rWidth/2.0f));
        rBotL.push_back(glm::vec3(x, deckY, -rWidth/2.0f));
        rBotR.push_back(glm::vec3(x, deckY, rWidth/2.0f));
    }
    railTop = new Mesh(BezierSurface::CreateRuledSurface(rTopL, rTopR, 2));
    railBot = new Mesh(BezierSurface::CreateRuledSurface(rBotR, rBotL, 2));
    railSideL = new Mesh(BezierSurface::CreateRuledSurface(rBotL, rTopL, 2));
    railSideR = new Mesh(BezierSurface::CreateRuledSurface(rTopR, rBotR, 2));
}

void Bridge::Draw(Shader& shader) {
    Material woodMat = Material::Wood();
    shader.setVec3("material.ambient", woodMat.ambient);
    shader.setVec3("material.diffuse", woodMat.diffuse);
    shader.setVec3("material.specular", woodMat.specular);
    shader.setFloat("material.shininess", woodMat.shininess);

    glm::mat4 baseTransform = glm::mat4(1.0f);
    baseTransform = glm::translate(baseTransform, position);
    baseTransform = glm::rotate(baseTransform, glm::radians(rotation), glm::vec3(0, 1, 0));

    // Draw deck using Solid Ruled Surfaces
    glm::mat4 deckModel = baseTransform;
    deckModel = glm::scale(deckModel, glm::vec3(length / 12.0f, 1.0f, width / 4.0f));
    shader.setMat4("model", deckModel);
    
    // Disable culling for ruled surfaces to ensure all sides render robustly
    GLboolean cullingOn;
    glGetBooleanv(GL_CULL_FACE, &cullingOn);
    glDisable(GL_CULL_FACE);
    
    deckTop->Draw();
    deckBot->Draw();
    deckSideL->Draw();
    deckSideR->Draw();
    
    // Draw solid ruled surface railings on both sides
    for (int side = -1; side <= 1; side += 2) {
        glm::mat4 railModel = baseTransform;
        railModel = glm::translate(railModel, glm::vec3(0, 0, side * (width / 2.0f - 0.1f)));
        railModel = glm::scale(railModel, glm::vec3(length / 12.0f, 1.0f, 1.0f));
        shader.setMat4("model", railModel);
        
        railTop->Draw();
        railBot->Draw();
        railSideL->Draw();
        railSideR->Draw();
    }
    
    if (cullingOn) glEnable(GL_CULL_FACE);

    // Draw support pillars
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glm::mat4 supportModel = baseTransform;
            supportModel = glm::translate(supportModel, glm::vec3(i * (length / 2 - 1.0f), -1.0f, j * (width / 2 - 0.3f)));
            shader.setMat4("model", supportModel);
            support->Draw();
        }
    }
}


// ============================================================================
// WATER TANK IMPLEMENTATION
// ============================================================================

static Mesh* s_waterTank = nullptr;
static Mesh* s_tankBase = nullptr;

WaterTank::WaterTank(glm::vec3 pos, float s)
    : position(pos), scale(s) {
    initMeshes();
}

void WaterTank::initMeshes() {
    if (!s_waterTank) {
        s_waterTank = new Mesh(BezierSurface::CreateWaterTank(1.5f, 3.0f, 1.0f, 32));
    }
    if (!s_tankBase) {
        s_tankBase = new Mesh(Mesh::CreateCylinder(0.3f, 4.0f, 24));
    }
    
    tank = s_waterTank;
    base = s_tankBase;
}

void WaterTank::Draw(Shader& shader) {
    Material metalMat = Material::Metal();
    shader.setVec3("material.ambient", metalMat.ambient);
    shader.setVec3("material.diffuse", metalMat.diffuse);
    shader.setVec3("material.specular", metalMat.specular);
    shader.setFloat("material.shininess", metalMat.shininess);

    // Draw tank
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::translate(model, glm::vec3(0, 4.0f, 0));
    model = glm::scale(model, glm::vec3(scale));
    shader.setMat4("model", model);
    tank->Draw();

    // Draw support legs
    for (int i = 0; i < 4; ++i) {
        float angle = static_cast<float>(M_PI) / 2.0f * i + static_cast<float>(M_PI) / 4.0f;
        glm::mat4 legModel = glm::mat4(1.0f);
        legModel = glm::translate(legModel, position);
        legModel = glm::translate(legModel, glm::vec3(cos(angle) * 1.0f * scale, 2.0f, sin(angle) * 1.0f * scale));
        shader.setMat4("model", legModel);
        base->Draw();
    }
}

// ============================================================================
// MOSQUE DOME IMPLEMENTATION
// Walkable 3D mosque with open interior, main gate, arched windows,
// twin minarets, Bezier dome, and crescent finials
// ============================================================================

static Mesh* s_mosqueDome = nullptr;
static Mesh* s_mosqueSmallDome = nullptr;
static Mesh* s_mosqueCube = nullptr;
static Mesh* s_mosqueMinaret = nullptr;
static Mesh* s_mosqueMinaretTop = nullptr;
static Mesh* s_mosqueCrescent = nullptr;
static Mesh* s_mosqueDomeBase = nullptr;
static Mesh* s_mosqueFloor = nullptr;
static Mesh* s_mosquePillar = nullptr;
static Mesh* s_mosqueArchWindow = nullptr;
static Mesh* s_mosqueMainArch = nullptr;
static Mesh* s_mosqueIslamicGlass = nullptr;
static Mesh* s_mosqueIslamicWallFill = nullptr;
static Mesh* s_frontGateArchFrame[4] = {nullptr, nullptr, nullptr, nullptr};
static Mesh* s_courtyardGateArchFrame[4] = {nullptr, nullptr, nullptr, nullptr};

unsigned int MosqueDome::mosqueWallTexture = 0;
unsigned int MosqueDome::mosqueFloorTexture = 0;
unsigned int MosqueDome::mosqueDomeTexture = 0;
unsigned int MosqueDome::mosqueWindowTexture = 0;
bool MosqueDome::texturesLoaded = false;

void MosqueDome::loadTextures() {
    if (texturesLoaded) return;
    stbi_set_flip_vertically_on_load(true);
    mosqueWallTexture = loadSingleTexture("src/white_bricks.png");
    mosqueFloorTexture = loadSingleTexture("src/mosque_floor1.png");
    mosqueDomeTexture = loadSingleTexture("src/mosque_side.png");
    mosqueWindowTexture = loadSingleTexture("src/mosque_window_texture.png");
    texturesLoaded = true;
}

// Helper: create a wall with a rectangular hole (for doors/windows)
static Mesh* CreateWallWithHole(float w, float h, float thick,
                                 float holeX, float holeY, float holeW, float holeH,
                                 glm::vec3 normal) {
    std::vector<Vertex> v;
    std::vector<unsigned int> idx;
    float z = thick / 2.0f;
    float hw = w / 2.0f;

    // The wall face is in the XY plane at z = +z
    // Hole corners
    float hL = holeX - holeW / 2.0f;
    float hR = holeX + holeW / 2.0f;
    float hB = holeY;
    float hT = holeY + holeH;

    auto addQ = [&](float x0, float y0, float x1, float y1) {
        int b = static_cast<int>(v.size());
        v.push_back({{x0, y0, z}, normal, {0, 0}});
        v.push_back({{x1, y0, z}, normal, {1, 0}});
        v.push_back({{x1, y1, z}, normal, {1, 1}});
        v.push_back({{x0, y1, z}, normal, {0, 1}});
        idx.push_back(b); idx.push_back(b+1); idx.push_back(b+2);
        idx.push_back(b); idx.push_back(b+2); idx.push_back(b+3);
    };

    // Left strip
    addQ(-hw, 0, hL, h);
    // Right strip
    addQ(hR, 0, hw, h);
    // Bottom strip (under hole)
    addQ(hL, 0, hR, hB);
    // Top strip (above hole)
    addQ(hL, hT, hR, h);

    return new Mesh(v, idx);
}

// Creates a solid Islamic pointed-arch window mesh using Bezier curves.
// The mesh fills the entire rectangular bay with wall color around the arch
// and window (glass) color inside the arch. Two separate meshes are returned
// via output parameters: one for the wall surround, one for the glass fill.
static void CreateIslamicArchWindowMeshes(
    float bayW, float bayH, float depth,
    Mesh** outWallFill, Mesh** outGlassFill, int segments = 24) 
{
    float halfW = bayW / 2.0f;
    float halfD = depth / 2.0f;

    // Get the arch outline points
    // The outline goes: BL, BR, right-spring, ..right bezier.., apex, ..left bezier.., left-spring
    float rectH = bayH * 0.45f;
    float archH = bayH - rectH;

    // Bezier control points for the arch
    glm::vec2 L0(-halfW, rectH), L1(-halfW, rectH + archH * 0.85f);
    glm::vec2 L2(-halfW * 0.15f, rectH + archH), L3(0.0f, rectH + archH);
    glm::vec2 R0(0.0f, rectH + archH), R1(halfW * 0.15f, rectH + archH);
    glm::vec2 R2(halfW, rectH + archH * 0.85f), R3(halfW, rectH);

    // =============================================
    // GLASS FILL: the pointed arch interior
    // =============================================
    {
        std::vector<Vertex> verts;
        std::vector<unsigned int> inds;
        glm::vec3 n(0, 0, 1);

        // Center point for fan triangulation
        glm::vec2 center(0.0f, rectH + archH * 0.4f);
        verts.push_back({{center.x, center.y, halfD}, n, {0.5f, 0.5f}});

        // Build outline: bottom-left of arch rect, bottom-right, then right Bezier up,
        // then left Bezier down
        std::vector<glm::vec2> outline;
        outline.push_back(glm::vec2(-halfW, rectH)); // left spring
        outline.push_back(glm::vec2(-halfW, 0.0f));  // bottom-left
        outline.push_back(glm::vec2(halfW, 0.0f));   // bottom-right
        outline.push_back(glm::vec2(halfW, rectH));   // right spring

        // Right Bezier: R3 (right spring) -> R0 (apex)
        for (int i = 1; i <= segments; ++i) {
            float t = (float)i / segments;
            float u = 1.0f - t;
            glm::vec2 p = u*u*u*R3 + 3.0f*u*u*t*R2 + 3.0f*u*t*t*R1 + t*t*t*R0;
            outline.push_back(p);
        }
        // Left Bezier: L3 (apex) -> L0 (left spring)
        for (int i = 1; i < segments; ++i) {
            float t = (float)i / segments;
            float u = 1.0f - t;
            glm::vec2 p = u*u*u*L3 + 3.0f*u*u*t*L2 + 3.0f*u*t*t*L1 + t*t*t*L0;
            outline.push_back(p);
        }

        // Add outline vertices
        for (auto& pt : outline) {
            float uu = (pt.x + halfW) / bayW;
            float vv = pt.y / bayH;
            verts.push_back({{pt.x, pt.y, halfD}, n, {uu, vv}});
        }

        // Fan triangles from center
        int count = (int)outline.size();
        for (int i = 0; i < count; ++i) {
            inds.push_back(0);
            inds.push_back(1 + i);
            inds.push_back(1 + (i + 1) % count);
        }

        // Back face
        int backStart = (int)verts.size();
        for (int i = 0; i < (int)verts.size(); ++i) {
            // Only copy front face verts
            if (i >= backStart) break;
            Vertex vv = verts[i];
            vv.Position.z = -halfD;
            vv.Normal = glm::vec3(0, 0, -1);
            verts.push_back(vv);
        }
        size_t frontIndCount = inds.size();
        for (size_t i = 0; i < frontIndCount; i += 3) {
            inds.push_back(backStart + inds[i]);
            inds.push_back(backStart + inds[i + 2]);
            inds.push_back(backStart + inds[i + 1]);
        }

        *outGlassFill = new Mesh(verts, inds);
    }

    // =============================================
    // WALL FILL: the area between the rectangular bay and the arch outline
    // (the "spandrels" - the corner regions)
    // =============================================
    {
        std::vector<Vertex> verts;
        std::vector<unsigned int> inds;
        glm::vec3 n(0, 0, 1);

        // Bay rectangle corners
        glm::vec2 TL(-halfW, bayH), TR(halfW, bayH);

        // LEFT SPANDREL: triangle-ish region from TL down left edge to left-spring,
        // then along arch to apex, then across top to TL
        // We'll do it as a filled polygon using fan from TL
        int base = 0;
        verts.push_back({{TL.x, TL.y, halfD}, n, {0, 1}});
        // Add left wall edge down to left spring
        verts.push_back({{-halfW, rectH, halfD}, n, {0, rectH/bayH}});
        // Add left Bezier curve points up to apex
        for (int i = 1; i <= segments; ++i) {
            float t = (float)i / segments;
            float u = 1.0f - t;
            glm::vec2 p = u*u*u*L0 + 3.0f*u*u*t*L1 + 3.0f*u*t*t*L2 + t*t*t*L3;
            float uu = (p.x + halfW) / bayW;
            float vv = p.y / bayH;
            verts.push_back({{p.x, p.y, halfD}, n, {uu, vv}});
        }
        // Add top-center
        verts.push_back({{0.0f, bayH, halfD}, n, {0.5f, 1.0f}});

        int leftCount = (int)verts.size() - 1; // number of outline verts
        for (int i = 1; i < leftCount; ++i) {
            inds.push_back(base);
            inds.push_back(base + i);
            inds.push_back(base + i + 1);
        }

        // RIGHT SPANDREL
        int rbase = (int)verts.size();
        verts.push_back({{TR.x, TR.y, halfD}, n, {1, 1}});
        // Top-center
        verts.push_back({{0.0f, bayH, halfD}, n, {0.5f, 1.0f}});
        // Apex
        verts.push_back({{0.0f, rectH + archH, halfD}, n, {0.5f, (rectH+archH)/bayH}});
        // Right Bezier from apex to right spring
        for (int i = 1; i <= segments; ++i) {
            float t = (float)i / segments;
            float u = 1.0f - t;
            glm::vec2 p = u*u*u*R0 + 3.0f*u*u*t*R1 + 3.0f*u*t*t*R2 + t*t*t*R3;
            float uu = (p.x + halfW) / bayW;
            float vv = p.y / bayH;
            verts.push_back({{p.x, p.y, halfD}, n, {uu, vv}});
        }
        // Right wall edge up to TR
        verts.push_back({{halfW, rectH, halfD}, n, {1, rectH/bayH}});

        // Actually we need to fan from TR
        int rightCount = (int)verts.size() - rbase - 1;
        for (int i = 1; i < rightCount; ++i) {
            inds.push_back(rbase);
            inds.push_back(rbase + i + 1);
            inds.push_back(rbase + i);
        }

        // Back face: duplicate all verts with flipped normal and flipped z
        int backStart = (int)verts.size();
        int frontVertCount = backStart;
        for (int i = 0; i < frontVertCount; ++i) {
            Vertex vv = verts[i];
            vv.Position.z = -halfD;
            vv.Normal = glm::vec3(0, 0, -1);
            verts.push_back(vv);
        }
        size_t frontIndCount = inds.size();
        for (size_t i = 0; i < frontIndCount; i += 3) {
            inds.push_back(backStart + inds[i]);
            inds.push_back(backStart + inds[i + 2]);
            inds.push_back(backStart + inds[i + 1]);
        }

        *outWallFill = new Mesh(verts, inds);
    }
}

// Helper function to create an arch shape (legacy, kept for gate arch)
static Mesh CreateArchMesh(float width, float height, float depth, int segments = 16) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    float halfW = width / 2.0f;
    float archHeight = height * 0.4f;
    float rectHeight = height * 0.6f;
    float radius = halfW;

    verts.push_back({{-halfW, 0, depth/2}, {0, 0, 1}, {0, 0}});
    verts.push_back({{halfW, 0, depth/2}, {0, 0, 1}, {1, 0}});
    verts.push_back({{halfW, rectHeight, depth/2}, {0, 0, 1}, {1, 0.6f}});
    verts.push_back({{-halfW, rectHeight, depth/2}, {0, 0, 1}, {0, 0.6f}});

    int archStart = static_cast<int>(verts.size());
    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(M_PI) * i / segments;
        float x = -cos(angle) * radius;
        float y = rectHeight + sin(angle) * archHeight;
        float u = static_cast<float>(i) / segments;
        verts.push_back({{x, y, depth/2}, {0, 0, 1}, {u, 0.6f + 0.4f * sin(angle)}});
    }

    inds.push_back(0); inds.push_back(1); inds.push_back(2);
    inds.push_back(0); inds.push_back(2); inds.push_back(3);
    for (int i = 0; i < segments; ++i) {
        inds.push_back(3);
        inds.push_back(archStart + i);
        inds.push_back(archStart + i + 1);
    }
    inds.push_back(3); inds.push_back(archStart + segments); inds.push_back(2);

    int backStart = static_cast<int>(verts.size());
    int frontVertCount = backStart;
    for (int i = 0; i < frontVertCount; ++i) {
        Vertex vv = verts[i];
        vv.Position.z = -depth/2;
        vv.Normal = glm::vec3(0, 0, -1);
        verts.push_back(vv);
    }
    size_t frontIndCount = inds.size();
    for (size_t i = 0; i < frontIndCount; i += 3) {
        inds.push_back(backStart + inds[i]);
        inds.push_back(backStart + inds[i + 2]);
        inds.push_back(backStart + inds[i + 1]);
    }

    return Mesh(verts, inds);
}

// Helper function to create crescent moon shape
static Mesh CreateCrescentMesh(float outerRadius, float innerRadius, float thickness) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    int segments = 24;
    float offset = outerRadius * 0.3f;

    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(M_PI) * 0.2f + static_cast<float>(M_PI) * 1.6f * i / segments;
        float ox = cos(angle) * outerRadius;
        float oy = sin(angle) * outerRadius;
        float ix = cos(angle) * innerRadius + offset;
        float iy = sin(angle) * innerRadius;
        verts.push_back({{ox, oy, thickness/2}, {0, 0, 1}, {0, static_cast<float>(i)/segments}});
        verts.push_back({{ix, iy, thickness/2}, {0, 0, 1}, {1, static_cast<float>(i)/segments}});
        verts.push_back({{ox, oy, -thickness/2}, {0, 0, -1}, {0, static_cast<float>(i)/segments}});
        verts.push_back({{ix, iy, -thickness/2}, {0, 0, -1}, {1, static_cast<float>(i)/segments}});
    }
    for (int i = 0; i < segments; ++i) {
        int base = i * 4;
        inds.push_back(base); inds.push_back(base + 4); inds.push_back(base + 1);
        inds.push_back(base + 1); inds.push_back(base + 4); inds.push_back(base + 5);
        inds.push_back(base + 2); inds.push_back(base + 3); inds.push_back(base + 6);
        inds.push_back(base + 3); inds.push_back(base + 7); inds.push_back(base + 6);
    }
    return Mesh(verts, inds);
}

// Helper function to create a 3D curved frame using Ruled Surfaces
static void CreateCurvyArchFrameMeshes(Mesh* outMeshes[4], float innerR, float outerR, float depth, float squashY = 1.0f) {
    std::vector<glm::vec3> cInnerFront, cInnerBack, cOuterFront, cOuterBack;
    int segments = 24;
    for (int i = 0; i <= segments; ++i) {
        float ang = i * static_cast<float>(M_PI) / segments;
        float xInner = -cos(ang) * innerR;
        float yInner = sin(ang) * innerR * squashY;
        float xOuter = -cos(ang) * outerR;
        float yOuter = sin(ang) * outerR * squashY;
        
        cInnerFront.push_back(glm::vec3(xInner, yInner, depth/2));
        cInnerBack.push_back(glm::vec3(xInner, yInner, -depth/2));
        cOuterFront.push_back(glm::vec3(xOuter, yOuter, depth/2));
        cOuterBack.push_back(glm::vec3(xOuter, yOuter, -depth/2));
    }
    
    outMeshes[0] = new Mesh(BezierSurface::CreateRuledSurface(cInnerFront, cOuterFront, 2)); // Front Face
    outMeshes[1] = new Mesh(BezierSurface::CreateRuledSurface(cOuterBack, cInnerBack, 2));   // Back Face
    outMeshes[2] = new Mesh(BezierSurface::CreateRuledSurface(cInnerBack, cInnerFront, 2));  // Inner Face
    outMeshes[3] = new Mesh(BezierSurface::CreateRuledSurface(cOuterFront, cOuterBack, 2));  // Outer Face
}

// Helper function to create a dome using simple stacked circles logic
static Mesh CreateStackedDomeMesh(float maxRadius, float domeHeight, int stacks, int slices) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265358979323846f;

    for (int i = 0; i <= stacks; ++i) {
        // 0 to 90 degrees (PI / 2)
        float stackAngle = (PI / 2.0f) * ((float)i / stacks); 
        
        float height = domeHeight * std::sin(stackAngle);
        float currentRadius = maxRadius * std::cos(stackAngle);

        for (int j = 0; j <= slices; ++j) {
            float sliceAngle = (2.0f * PI) * ((float)j / slices);

            float x = currentRadius * std::cos(sliceAngle);
            float y = height;
            float z = currentRadius * std::sin(sliceAngle);

            glm::vec3 position(x, y, z);
            
            glm::vec3 normal(
                std::cos(stackAngle) * std::cos(sliceAngle), 
                std::sin(stackAngle), 
                std::cos(stackAngle) * std::sin(sliceAngle)
            );
            
            float len = glm::length(normal);
            if (len > 0.0001f) normal = normal / len;
            else normal = glm::vec3(0, 1, 0);

            glm::vec2 texCoords((float)j / slices, (float)i / stacks);

            vertices.push_back({position, normal, texCoords});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    return Mesh(vertices, indices);
}

MosqueDome::MosqueDome(glm::vec3 pos, float s)
    : position(pos), scale(s) {
    initMeshes();
}

void MosqueDome::initMeshes() {
    if (!s_mosqueDome) {
        // Using Stacked Circles logic for the main dome
        s_mosqueDome = new Mesh(CreateStackedDomeMesh(2.5f, 3.5f, 64, 64));
    }
    if (!s_mosqueSmallDome) {
        // Using Stacked Circles logic for the small dome
        s_mosqueSmallDome = new Mesh(CreateStackedDomeMesh(0.5f, 0.8f, 16, 24));
    }
    if (!s_mosqueDomeBase) {
        s_mosqueDomeBase = new Mesh(Mesh::CreateCylinder(2.6f, 1.0f, 32));
    }
    if (!s_mosqueCube) {
        s_mosqueCube = new Mesh(Mesh::CreateCube(1.0f));
    }
    if (!s_mosqueFloor) {
        s_mosqueFloor = new Mesh(Mesh::CreatePlane(1.0f, 1.0f, 1));
    }
    if (!s_mosquePillar) {
        s_mosquePillar = new Mesh(Mesh::CreateCylinder(0.25f, 1.0f, 16));
    }
    if (!s_mosqueMinaret) {
        s_mosqueMinaret = new Mesh(Mesh::CreateCylinder(0.6f, 12.0f, 32));
    }
    if (!s_mosqueMinaretTop) {
        s_mosqueMinaretTop = new Mesh(Mesh::CreateCylinder(0.8f, 0.5f, 32));
    }
    if (!s_mosqueArchWindow) {
        s_mosqueArchWindow = new Mesh(CreateArchMesh(1.2f, 2.5f, 0.3f, 12));
    }
    if (!s_mosqueIslamicGlass) {
        // Islamic pointed arch window: bay size 1.8W x 2.3H, wall thickness as depth
        CreateIslamicArchWindowMeshes(1.8f, 2.3f, 0.5f,
            &s_mosqueIslamicWallFill, &s_mosqueIslamicGlass, 20);
    }
    if (!s_mosqueMainArch) {
        s_mosqueMainArch = new Mesh(CreateArchMesh(3.2f, 4.5f, 0.4f, 16));
    }
    if (!s_mosqueCrescent) {
        s_mosqueCrescent = new Mesh(CreateCrescentMesh(0.4f, 0.3f, 0.05f));
    }
    
    // Initialize Ruled Surface Arches
    if (!s_frontGateArchFrame[0]) {
        // gateW = 3.2f, innerR = 1.6f. outerR = 1.9f. depth = 0.35f.
        // door arch height = 1.8f. squashY = 1.8f / 1.6f = 1.125f.
        CreateCurvyArchFrameMeshes(s_frontGateArchFrame, 1.6f, 1.9f, 0.35f, 1.125f);
    }
    if (!s_courtyardGateArchFrame[0]) {
        // cgW = 4.0f -> innerR=2.0f, outerR=2.3f. depth = 0.4f.
        // Squash is slightly less (e.g., 0.6f of radius)
        CreateCurvyArchFrameMeshes(s_courtyardGateArchFrame, 2.0f, 2.3f, 0.4f, 0.6f);
    }

    mainDome = s_mosqueDome;
    smallDome = s_mosqueSmallDome;
    mainBuilding = s_mosqueCube;
    frontSection = s_mosqueCube;
    minaret = s_mosqueMinaret;
    minaretTop = s_mosqueMinaretTop;
    archWindow = s_mosqueArchWindow;
    mainArch = s_mosqueMainArch;
    crescent = s_mosqueCrescent;
    domeBase = s_mosqueDomeBase;
    loadTextures();
}

void MosqueDome::Draw(Shader& shader) {
    glm::mat4 bt = glm::mat4(1.0f);
    bt = glm::translate(bt, position);
    bt = glm::scale(bt, glm::vec3(scale));

    // Cube mesh reference (all walls built from this single cube)
    Mesh* cube = s_mosqueCube;

    // Building dimensions
    float bW = 12.0f;   // width  (X)
    float bH = 6.0f;    // wall height
    float bD = 14.0f;   // depth  (Z)
    float wt = 0.4f;    // wall thickness
    float gateW = 3.2f;
    float gateH = 4.5f;
    float baseY = 0.3f; // top of platform
    float frontZ = bD / 2.0f;

    // Helper: draw a cube at (cx, cy, cz) with size (sx, sy, sz)
    auto drawCube = [&](float cx, float cy, float cz, float sx, float sy, float sz) {
        glm::mat4 m = bt;
        m = glm::translate(m, glm::vec3(cx, cy, cz));
        m = glm::scale(m, glm::vec3(sx, sy, sz));
        shader.setMat4("model", m);
        cube->Draw();
    };

    auto setMat = [&](glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shin) {
        shader.setVec3("material.ambient", amb);
        shader.setVec3("material.diffuse", diff);
        shader.setVec3("material.specular", spec);
        shader.setFloat("material.shininess", shin);
    };

    // ========================================================================
    // MATERIALS
    // ========================================================================
    auto matWall   = [&]() { setMat({0.90f,0.88f,0.85f},{0.98f,0.96f,0.92f},{0.4f,0.4f,0.4f}, 24.f); };
    auto matStone  = [&]() { setMat({0.55f,0.55f,0.52f},{0.70f,0.70f,0.65f},{0.2f,0.2f,0.2f},  8.f); };
    auto matDome   = [&]() { setMat({0.70f,0.60f,0.20f},{0.85f,0.75f,0.25f},{0.9f,0.8f,0.4f}, 64.f); };
    auto matGold   = [&]() { setMat({0.60f,0.50f,0.15f},{0.85f,0.75f,0.20f},{1.0f,0.95f,0.6f},96.f); };
    auto matWindow = [&]() { setMat({0.05f,0.12f,0.22f},{0.08f,0.18f,0.30f},{0.5f,0.5f,0.6f}, 64.f); };
    auto matFloor  = [&]() { setMat({0.85f,0.82f,0.78f},{0.90f,0.87f,0.82f},{0.3f,0.3f,0.3f}, 32.f); };
    auto matDark   = [&]() { setMat({0.25f,0.15f,0.08f},{0.40f,0.25f,0.12f},{0.15f,0.1f,0.08f},16.f); };
    auto matBorder = [&]() { setMat({0.75f,0.72f,0.68f},{0.82f,0.78f,0.72f},{0.25f,0.25f,0.25f},16.f); };

    // ========================================================================
    // 1. RAISED PLATFORM (cube) + STEPS (cubes)
    // ========================================================================
    matStone();
    drawCube(0, 0.15f, 0, bW + 4.f, 0.3f, bD + 4.f);

    for (int i = 0; i < 3; ++i) {
        drawCube(0, 0.05f + i * 0.1f, frontZ + 2.5f - i * 0.5f, 4.f, 0.1f, 1.f);
    }

    // ========================================================================
    // 2. WALLS built from solid structural slabs
    // ========================================================================
    matWall();

    // Apply white bricks texture to outer walls
    if (mosqueWallTexture != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mosqueWallTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", textureBlendMode == 1);
    }

    // The walls will be constructed with a solid base, solid roofline, 
    // and correctly proportioned pillars between to form realistic window bays.
    float wallBaseH = 2.2f;
    float wallTopH = 1.5f;
    float colW = 1.2f; // Much thicker columns for realism

    // --- LEFT WALL (X = -bW/2) ---
    drawCube(-bW/2, baseY + wallBaseH/2, 0, wt, wallBaseH, bD); // base
    drawCube(-bW/2, baseY + bH - wallTopH/2, 0, wt, wallTopH, bD); // top
    for (int i = -2; i <= 2; ++i) {
        drawCube(-bW/2, baseY + bH/2, i * 3.0f, wt, bH, colW); // solid vertical spans
    }

    // --- RIGHT WALL (X = bW/2) ---
    drawCube(bW/2, baseY + wallBaseH/2, 0, wt, wallBaseH, bD);
    drawCube(bW/2, baseY + bH - wallTopH/2, 0, wt, wallTopH, bD);
    for (int i = -2; i <= 2; ++i) {
        drawCube(bW/2, baseY + bH/2, i * 3.0f, wt, bH, colW);
    }

    // --- BACK WALL (Z = -bD/2) ---
    drawCube(0, baseY + wallBaseH/2, -bD/2, bW, wallBaseH, wt); // base
    drawCube(0, baseY + bH - wallTopH/2, -bD/2, bW, wallTopH, wt); // top
    // Back wall columns between window bays (widened to close gaps)
    drawCube(0, baseY + bH/2, -bD/2, 1.5f, bH, wt);       // center column
    drawCube(-3.5f, baseY + bH/2, -bD/2, 1.5f, bH, wt);   // left column
    drawCube(3.5f, baseY + bH/2, -bD/2, 1.5f, bH, wt);    // right column
    // Solid fill for outer bays (between outer columns and wall edges - no windows here)
    float outerBayCenter = (3.5f + 0.75f + bW/2.0f) / 2.0f; // center of outer bay
    float outerBayW = bW/2.0f - 3.5f - 0.75f;               // width of outer bay
    // Fill the full height of the window area in the outer bays with solid wall
    drawCube(-outerBayCenter, baseY + bH/2, -bD/2, outerBayW + 0.5f, bH, wt);
    drawCube(outerBayCenter, baseY + bH/2, -bD/2, outerBayW + 0.5f, bH, wt);

    // --- FRONT WALL with GATE OPENING ---
    float sideW = (bW - gateW) / 2.0f;
    drawCube(-(gateW/2 + sideW/2), baseY + bH/2, frontZ, sideW, bH, wt); // left wing
    drawCube(gateW/2 + sideW/2, baseY + bH/2, frontZ, sideW, bH, wt);    // right wing
    drawCube(0, baseY + gateH + (bH - gateH)/2, frontZ, gateW, bH - gateH, wt); // top arch lintel
    
    // Fill the empty square corners of the white wall opening behind the curved door
    drawCube(-gateW/2 + 0.4f, baseY + 3.8f, frontZ, 0.8f, 1.4f, wt); // left corner fill
    drawCube(gateW/2 - 0.4f, baseY + 3.8f, frontZ, 0.8f, 1.4f, wt); // right corner fill

    // Unbind wall texture
    shader.setBool("useTexture", false);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Beautiful Curvy Gate Frame around the main door
    matDark();
    float doorRectHeight = 4.5f * 0.6f; // 2.7f
    float doorArchHeight = 4.5f * 0.4f; // 1.8f
    // Vertical side posts
    drawCube(-gateW/2 - 0.15f, baseY + doorRectHeight/2, frontZ + 0.25f, 0.3f, doorRectHeight, 0.3f);
    drawCube(gateW/2 + 0.15f, baseY + doorRectHeight/2, frontZ + 0.25f, 0.3f, doorRectHeight, 0.3f);
    // Curvy solid ruled-surface wooden arch top
    for (int k = 0; k < 4; ++k) {
        glm::mat4 bm = bt;
        // Translate to the hinge line
        bm = glm::translate(bm, glm::vec3(0, baseY + doorRectHeight, frontZ + 0.25f));
        shader.setMat4("model", bm);
        s_frontGateArchFrame[k]->Draw();
    }

    // Decorative exterior trim bands (Split up so they don't block the doorway!)
    matBorder();
    // Trim along left side
    drawCube(-bW/2, baseY + wallBaseH, 0, wt + 0.15f, 0.15f, bD + 0.15f);
    drawCube(-bW/2, baseY + bH - wallTopH, 0, wt + 0.15f, 0.15f, bD + 0.15f);
    // Trim along right side
    drawCube(bW/2, baseY + wallBaseH, 0, wt + 0.15f, 0.15f, bD + 0.15f);
    drawCube(bW/2, baseY + bH - wallTopH, 0, wt + 0.15f, 0.15f, bD + 0.15f);
    // Trim along back
    drawCube(0, baseY + wallBaseH, -bD/2, bW + 0.15f, 0.15f, wt + 0.15f);
    drawCube(0, baseY + bH - wallTopH, -bD/2, bW + 0.15f, 0.15f, wt + 0.15f);
    // Trim along front (split left and right wings to avoid door)
    drawCube(-(gateW/2 + sideW/2), baseY + wallBaseH, frontZ, sideW, 0.15f, wt + 0.15f);
    drawCube(gateW/2 + sideW/2, baseY + wallBaseH, frontZ, sideW, 0.15f, wt + 0.15f);
    drawCube(-(gateW/2 + sideW/2), baseY + bH - wallTopH, frontZ, sideW, 0.15f, wt + 0.15f);
    drawCube(gateW/2 + sideW/2, baseY + bH - wallTopH, frontZ, sideW, 0.15f, wt + 0.15f);


    // ========================================================================
    // 3. ISLAMIC POINTED ARCH WINDOWS (Bezier curve) inside the bays
    //    Each window is composed of:
    //    - Wall fill (spandrels): same material as outer wall, fills corners
    //    - Glass fill: dark window material, fills the pointed arch shape
    //    - Decorative frame cubes around the arch outline
    // ========================================================================
    glDisable(GL_CULL_FACE); // Disable culling for flat window meshes
    
    // Helper lambda to draw one Islamic window at a given position/rotation
    auto drawIslamicWindow = [&](glm::vec3 pos, float rotY) {
        glm::mat4 m = bt;
        m = glm::translate(m, pos);
        m = glm::rotate(m, glm::radians(rotY), glm::vec3(0, 1, 0));
        
        // Draw wall fill (spandrels) with wall material/texture
        if (mosqueWallTexture != 0 && enableTextures) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mosqueWallTexture);
            shader.setInt("texture_diffuse", 0);
            shader.setBool("useTexture", true);
            shader.setBool("textureOnly", textureBlendMode == 1);
        }
        matWall();
        shader.setMat4("model", m);
        s_mosqueIslamicWallFill->Draw();
        shader.setBool("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // Draw glass fill (the pointed arch interior) with mosque window texture
        if (mosqueWindowTexture != 0 && enableTextures) {
            // Use a bright neutral material so the texture is clearly visible
            setMat({0.8f,0.8f,0.8f},{0.9f,0.9f,0.9f},{0.3f,0.3f,0.3f}, 16.f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mosqueWindowTexture);
            shader.setInt("texture_diffuse", 0);
            shader.setBool("useTexture", true);
            shader.setBool("textureOnly", true);  // Force texture-only so it's not darkened by material
        } else {
            matWindow();
        }
        shader.setMat4("model", m);
        s_mosqueIslamicGlass->Draw();
        shader.setBool("useTexture", false);
        shader.setBool("textureOnly", false);
        glBindTexture(GL_TEXTURE_2D, 0);
    };
    
    // --- LEFT WALL windows (X = -bW/2, rotate 90 degrees so Z-face points outward) ---
    for (int i = 0; i < 4; ++i) {
        float wz = -4.5f + i * 3.0f;
        drawIslamicWindow(glm::vec3(-bW/2, baseY + wallBaseH, wz), 90.0f);
    }
    
    // --- RIGHT WALL windows (X = bW/2, rotate -90 degrees) ---
    for (int i = 0; i < 4; ++i) {
        float wz = -4.5f + i * 3.0f;
        drawIslamicWindow(glm::vec3(bW/2, baseY + wallBaseH, wz), -90.0f);
    }
    
    // --- BACK WALL windows (Z = -bD/2, facing -Z so rotate 180) ---
    for (int i = -1; i <= 1; i+=2) {
        float wx = i * 1.75f;
        drawIslamicWindow(glm::vec3(wx, baseY + wallBaseH, -bD/2), 0.0f);
    }
    
    glEnable(GL_CULL_FACE); // Re-enable culling

    // ========================================================================
    // 4. INTERIOR FLOOR (cube slab)
    // ========================================================================
    matFloor();
    // Apply mosque floor texture (blended)
    if (mosqueFloorTexture != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mosqueFloorTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", textureBlendMode == 1);
    }
    drawCube(0, baseY + 0.02f, 0, bW - wt, 0.04f, bD - wt);
    shader.setBool("useTexture", false);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Floor tile pattern (alternating color cubes for carpet look)
    matDark();
    for (int ix = -2; ix <= 2; ++ix) {
        for (int iz = -3; iz <= 3; ++iz) {
            if ((ix + iz) % 2 == 0) {
                drawCube(ix * 1.5f, baseY + 0.05f, iz * 1.5f, 1.4f, 0.02f, 1.4f);
            }
        }
    }



    // ========================================================================
    // 6. MIHRAB (prayer niche on back wall - cube blocks)
    // ========================================================================
    // matGold();
    // Niche frame cubes
    // drawCube(0, baseY + 2.5f, -bD/2 + 0.15f, 2.0f, 4.5f, 0.3f);  // inset back
    // matDark();
    // drawCube(-1.05f, baseY + 2.5f, -bD/2 + 0.2f, 0.15f, 4.5f, 0.2f);  // left
    // drawCube(1.05f, baseY + 2.5f, -bD/2 + 0.2f, 0.15f, 4.5f, 0.2f);  // right
    // drawCube(0, baseY + 4.8f, -bD/2 + 0.2f, 2.2f, 0.15f, 0.2f);  // top

    // ========================================================================
    // 7. ROOF / CEILING (cube slab)
    // ========================================================================
    matWall();
    drawCube(0, baseY + bH, 0, bW + 0.5f, 0.3f, bD + 0.5f);

    // Parapet cubes along roof edge (decorative crenellation)
    matBorder();
    for (int i = 0; i < 12; ++i) {
        float px = -bW/2 + 0.5f + i * (bW / 11.0f);
        drawCube(px, baseY + bH + 0.4f, frontZ + 0.1f, 0.5f, 0.5f, 0.3f);
        drawCube(px, baseY + bH + 0.4f, -bD/2 - 0.1f, 0.5f, 0.5f, 0.3f);
    }
    for (int i = 0; i < 14; ++i) {
        float pz = -bD/2 + 0.5f + i * (bD / 13.0f);
        drawCube(-bW/2 - 0.1f, baseY + bH + 0.4f, pz, 0.3f, 0.5f, 0.5f);
        drawCube(bW/2 + 0.1f, baseY + bH + 0.4f, pz, 0.3f, 0.5f, 0.5f);
    }

    // ========================================================================
    // 8. DOME BASE DRUM + MAIN DOME + CRESCENT
    // ========================================================================
    matWall();
    {
        glm::mat4 m = bt;
        m = glm::translate(m, glm::vec3(0, baseY + bH + 0.5f, 0));
        shader.setMat4("model", m);
        domeBase->Draw();
    }

    matDome();
    {
        // Textured curvy surface: Bezier dome with mosque_side texture (blended)
        if (mosqueDomeTexture != 0 && enableTextures && enableCurvyTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mosqueDomeTexture);
            shader.setInt("texture_diffuse", 0);
            shader.setBool("useTexture", true);
            shader.setBool("textureOnly", textureBlendMode == 1);
        }
        glm::mat4 m = bt;
        m = glm::translate(m, glm::vec3(0, baseY + bH + 1.0f, 0));
        shader.setMat4("model", m);
        mainDome->Draw();
        shader.setBool("useTexture", false);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    matGold();
    {
        glm::mat4 m = bt;
        m = glm::translate(m, glm::vec3(0, baseY + bH + 4.7f, 0));
        m = glm::scale(m, glm::vec3(1.5f));
        shader.setMat4("model", m);
        crescent->Draw();
    }

    // ========================================================================
    // 9. GATE ARCH (golden decorative arch)
    // ========================================================================
    matGold();
    {
        glm::mat4 m = bt;
        m = glm::translate(m, glm::vec3(0, baseY, frontZ + 0.25f));
        shader.setMat4("model", m);
        mainArch->Draw();
    }

    // ========================================================================
    // 10. TWIN MINARETS (flanking the entrance)
    // ========================================================================
    for (int side = -1; side <= 1; side += 2) {
        float mx = side * (bW / 2.0f + 1.5f);
        float mz = frontZ;

        // Minaret base (cube block)
        matStone();
        drawCube(mx, baseY + 0.5f, mz, 1.6f, 1.0f, 1.6f);

        // Tower Smooth Cylinder
        matWall();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 6.0f, mz));
            shader.setMat4("model", m);
            minaret->Draw(); // this is Mesh::CreateCylinder
        }

        // Curved Balcony Ring Underneath
        matBorder();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 9.9f, mz));
            m = glm::scale(m, glm::vec3(2.5f, 0.4f, 2.5f));
            shader.setMat4("model", m);
            minaretTop->Draw(); // cylinder used as a smooth ring
        }

        // Balcony Solid Floor
        matStone();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 10.0f, mz));
            m = glm::scale(m, glm::vec3(2.0f, 0.2f, 2.0f));
            shader.setMat4("model", m);
            minaretTop->Draw();
        }

        // Solid Curved Balcony Railing (Cylinder)
        matWall();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 10.4f, mz));
            // A slightly larger cylinder representing continuous railing
            m = glm::scale(m, glm::vec3(2.0f, 1.2f, 2.0f));
            shader.setMat4("model", m);
            minaretTop->Draw();
        }

        // Upper section
        matWall();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 12.5f, mz));
            m = glm::scale(m, glm::vec3(0.7f, 0.5f, 0.7f));
            shader.setMat4("model", m);
            minaret->Draw();
        }

        // Small dome
        matDome();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 15.5f, mz));
            shader.setMat4("model", m);
            smallDome->Draw();
        }

        // Crescent
        matGold();
        {
            glm::mat4 m = bt;
            m = glm::translate(m, glm::vec3(mx, baseY + 16.5f, mz));
            m = glm::scale(m, glm::vec3(0.6f));
            shader.setMat4("model", m);
            crescent->Draw();
        }
    }

    // ========================================================================
    // 11. COURTYARD WALL built from cube blocks
    // ========================================================================
    matStone();
    float cwW = bW + 10.0f;
    float cwD = bD + 10.0f;
    float cwH = 1.2f;
    float cwT = 0.3f;

    // Left courtyard wall
    drawCube(-cwW/2, cwH/2, 0, cwT, cwH, cwD);
    // Right courtyard wall
    drawCube(cwW/2, cwH/2, 0, cwT, cwH, cwD);
    // Back courtyard wall
    drawCube(0, cwH/2, -cwD/2, cwW, cwH, cwT);
    // Front courtyard wall (with gate gap)
    float cgW = 4.0f;
    float csW = (cwW - cgW) / 2.0f;
    drawCube(-(cgW/2 + csW/2), cwH/2, cwD/2, csW, cwH, cwT);
    drawCube(cgW/2 + csW/2, cwH/2, cwD/2, csW, cwH, cwT);

    // Courtyard wall pillaster cubes (decorative posts)
    matBorder();
    for (int i = 0; i < 6; ++i) {
        float pz = -cwD/2 + 2.0f + i * (cwD / 5.0f);
        drawCube(-cwW/2, cwH + 0.3f, pz, 0.5f, 0.6f, 0.5f);
        drawCube(cwW/2, cwH + 0.3f, pz, 0.5f, 0.6f, 0.5f);
    }
    for (int i = 0; i < 5; ++i) {
        float px = -cwW/2 + 2.0f + i * (cwW / 4.0f);
        drawCube(px, cwH + 0.3f, -cwD/2, 0.5f, 0.6f, 0.5f);
    }

    // Courtyard gate posts and Curvy Gate archway
    matDark();
    drawCube(-cgW/2 - 0.2f, cwH/2 + 0.4f, cwD/2, 0.4f, cwH + 0.8f, 0.4f); // Left post
    drawCube(cgW/2 + 0.2f, cwH/2 + 0.4f, cwD/2, 0.4f, cwH + 0.8f, 0.4f);  // Right post
    
    // Curvy solid ruled-surface string course connecting the posts
    for (int k = 0; k < 4; ++k) {
        glm::mat4 bm = bt;
        bm = glm::translate(bm, glm::vec3(0, cwH + 0.8f, cwD/2));
        shader.setMat4("model", bm);
        s_courtyardGateArchFrame[k]->Draw();
    }
}

// ============================================================================
// TERRAIN IMPLEMENTATION
// ============================================================================

static Mesh* s_terrain = nullptr;

// Grass texture static members
unsigned int Terrain::grassTexture = 0;
bool Terrain::grassTextureLoaded = false;

void Terrain::loadGrassTexture() {
    if (grassTextureLoaded) return;
    stbi_set_flip_vertically_on_load(true);
    grassTexture = loadSingleTexture("src/grass.png");
    grassTextureLoaded = true;
}

Terrain::Terrain(float w, float d, glm::vec3 pos)
    : width(w), depth(d), position(pos) {
    initMesh();
}

void Terrain::initMesh() {
    if (!s_terrain) {
        // Build terrain plane manually with tiled UVs for grass texture
        std::vector<Vertex> verts;
        std::vector<unsigned int> inds;
        int subdivs = 20;
        float halfW = 50.0f, halfD = 50.0f;
        float stepX = 100.0f / subdivs, stepZ = 100.0f / subdivs;
        float uvScale = 15.0f; // Tile the grass texture 15x
        
        for (int z = 0; z <= subdivs; ++z) {
            for (int x = 0; x <= subdivs; ++x) {
                float px = -halfW + x * stepX;
                float pz = -halfD + z * stepZ;
                float u = (float)x / subdivs * uvScale;
                float v = (float)z / subdivs * uvScale;
                verts.push_back({{px, 0, pz}, {0, 1, 0}, {u, v}});
            }
        }
        for (int z = 0; z < subdivs; ++z) {
            for (int x = 0; x < subdivs; ++x) {
                int tl = z * (subdivs+1) + x;
                int tr = tl + 1;
                int bl = (z+1) * (subdivs+1) + x;
                int br = bl + 1;
                inds.push_back(tl); inds.push_back(bl); inds.push_back(tr);
                inds.push_back(tr); inds.push_back(bl); inds.push_back(br);
            }
        }
        s_terrain = new Mesh(verts, inds);
        loadGrassTexture();
    }
    ground = s_terrain;
}

void Terrain::Draw(Shader& shader) {
    Material grassMat = Material::Grass();
    shader.setVec3("material.ambient", grassMat.ambient);
    shader.setVec3("material.diffuse", grassMat.diffuse);
    shader.setVec3("material.specular", grassMat.specular);
    shader.setFloat("material.shininess", grassMat.shininess);

    // Bind grass texture
    if (grassTextureLoaded && grassTexture != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", false);
        shader.setBool("useMultiplyBlend", true);  // Natural grass blend
    } else {
        shader.setBool("useTexture", false);
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    shader.setMat4("model", model);
    ground->Draw();

    // Reset texture
    shader.setBool("useTexture", false);
    shader.setBool("useMultiplyBlend", false);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// RIVER IMPLEMENTATION
// Realistic 3D River with sloped earth banks, water surface, and riverbed
// ============================================================================

static Mesh* s_riverWater = nullptr;
static Mesh* s_riverBanks = nullptr;
static Mesh* s_riverBed = nullptr;

unsigned int River::waterTexture = 0;
bool River::waterTextureLoaded = false;

void River::loadWaterTexture() {
    if (waterTextureLoaded) return;
    stbi_set_flip_vertically_on_load(true);
    waterTexture = loadSingleTexture("src/water.png");
    waterTextureLoaded = true;
}

River::River(std::vector<glm::vec3> path, float w)
    : splinePath(path), width(w), flowOffset(0.0f), waveTime(0.0f) {
    initMesh();
    loadWaterTexture();
}

void River::initMesh() {
    auto getSplineState = [&](float t, glm::vec3& outPos, glm::vec3& outDir, glm::vec3& outRight) {
        if (splinePath.size() < 4) {
            outPos = glm::mix(splinePath.front(), splinePath.back(), t);
            outDir = glm::normalize(splinePath.back() - splinePath.front());
            outRight = glm::normalize(glm::cross(outDir, glm::vec3(0, 1, 0)));
            return;
        }
        
        int segments = splinePath.size() - 3;
        float totalProgress = t * segments; 
        int currentSegment = static_cast<int>(totalProgress);
        if (currentSegment >= segments) currentSegment = segments - 1;
        float segmentT = totalProgress - currentSegment;

        glm::vec3 p0 = splinePath[currentSegment];
        glm::vec3 p1 = splinePath[currentSegment + 1];
        glm::vec3 p2 = splinePath[currentSegment + 2];
        glm::vec3 p3 = splinePath[currentSegment + 3];

        outPos = BezierSurface::EvaluateCatmullRomSpline(p0, p1, p2, p3, segmentT);
        
        // Approximate derivative for tangent
        float dt = 0.01f;
        glm::vec3 nextPos;
        if (segmentT + dt > 1.0f && currentSegment < segments - 1) {
            nextPos = BezierSurface::EvaluateCatmullRomSpline(p1, p2, p3, splinePath[currentSegment + 4], segmentT + dt - 1.0f);
        } else {
            nextPos = BezierSurface::EvaluateCatmullRomSpline(p0, p1, p2, p3, std::min(1.0f, segmentT + dt));
        }
        
        outDir = glm::normalize(nextPos - outPos);
        if (glm::length(outDir) < 0.001f) outDir = glm::vec3(1, 0, 0); // fallback
        outRight = glm::normalize(glm::cross(outDir, glm::vec3(0, 1, 0)));
    };

    // ========================================================================
    // RIVER PARAMETERS - High detail for realism
    // ========================================================================
    int lengthSegments = 200;     // Much higher resolution for smooth curves
    float waterSurfaceY = 0.2f;   // RAISED: Water surface level (above ground to avoid clipping)
    float riverDepth = 0.5f;      // Visual depth (how deep the bottom appears relative to water)
    float bankWidth = 3.0f;       // Width of sloped earth banks on each side
    float bankHeight = 0.5f;      // How high banks rise above ground
    float waterWidth = width;     // Width of water surface
    
    // ========================================================================
    // CREATE WATER SURFACE - The actual visible water
    // ========================================================================
    std::vector<Vertex> waterVerts;
    std::vector<unsigned int> waterInds;
    
    int waterWidthSegs = 60;  // High detail for lateral waves
    
    for (int i = 0; i <= lengthSegments; ++i) {
        float t = static_cast<float>(i) / lengthSegments;
        glm::vec3 center, direction, right;
        getSplineState(t, center, direction, right);
        
        for (int j = 0; j <= waterWidthSegs; ++j) {
            float s = static_cast<float>(j) / waterWidthSegs;
            
            // Position across water width
            float xOffset = (s - 0.5f) * waterWidth;
            glm::vec3 pos = center + right * xOffset;
            
            // COMPLEX REALISTIC WAVE FUNCTION
            // Mix of low frequency (swells) and high frequency (ripples)
            
            // Main swells
            float wave1 = std::sin(t * 25.0f + s * 5.0f) * 0.02f;
            float wave2 = std::cos(t * 40.0f - s * 10.0f) * 0.015f;
            
            // Detail ripples
            float ripple1 = std::sin(t * 100.0f + s * 30.0f) * 0.005f;
            float ripple2 = std::cos(t * 80.0f - s * 40.0f) * 0.005f;
            
            // Interference
            float interference = std::sin((t - s) * 60.0f) * std::cos((t + s) * 40.0f) * 0.008f;
            
            float totalWave = wave1 + wave2 + ripple1 + ripple2 + interference;
            
            // Edge dampening
            float edgeFactor = 1.0f - std::pow(std::abs(s - 0.5f) * 2.0f, 4.0f);
            totalWave *= edgeFactor;
            
            pos.y = waterSurfaceY + totalWave;
            
            // Analytical normals for lighting (approximate derivatives)
            float dWaveX = std::cos(t * 25.0f + s * 5.0f) * 5.0f * 0.02f - 
                          std::sin(t * 40.0f - s * 10.0f) * 10.0f * 0.015f;
            float dWaveZ = std::cos(t * 25.0f + s * 5.0f) * 25.0f * 0.02f - 
                          std::sin(t * 40.0f - s * 10.0f) * 40.0f * 0.015f;
                          
            // Add ripple derivatives
            dWaveX += std::cos(t * 100.0f + s * 30.0f) * 30.0f * 0.005f;
            dWaveZ += std::cos(t * 100.0f + s * 30.0f) * 100.0f * 0.005f;
            
            glm::vec3 normal = glm::normalize(glm::vec3(-dWaveX * 0.3f, 1.0f, -dWaveZ * 0.05f));
            
            waterVerts.push_back({pos, normal, {s, t * 12.0f}});
        }
    }
    
    // Create water surface triangles
    int waterVertsPerRow = waterWidthSegs + 1;
    for (int i = 0; i < lengthSegments; ++i) {
        for (int j = 0; j < waterWidthSegs; ++j) {
            int curr = i * waterVertsPerRow + j;
            int next = (i + 1) * waterVertsPerRow + j;
            
            waterInds.push_back(curr);
            waterInds.push_back(curr + 1);
            waterInds.push_back(next + 1);
            
            waterInds.push_back(curr);
            waterInds.push_back(next + 1);
            waterInds.push_back(next);
        }
    }
    
    s_riverWater = new Mesh(waterVerts, waterInds);
    
    // ========================================================================
    // CREATE RIVER BANKS - Sloped earth sides going down to water
    // ========================================================================
    std::vector<Vertex> bankVerts;
    std::vector<unsigned int> bankInds;
    
    for (int i = 0; i <= lengthSegments; ++i) {
        float t = static_cast<float>(i) / lengthSegments;
        glm::vec3 center, direction, right;
        getSplineState(t, center, direction, right);
        
        // LEFT BANK (5 vertices per section for smooth slope)
        // Outer edge (at ground level, away from river)
        glm::vec3 leftOuter = center - right * (waterWidth / 2.0f + bankWidth);
        leftOuter.y = 0.05f; // Slightly above 0 to avoid z-fighting with ground
        bankVerts.push_back({leftOuter, glm::normalize(glm::vec3(-0.3f, 1.0f, 0)), {0, t * 4.0f}});
        
        // Upper bank edge
        glm::vec3 leftUpper = center - right * (waterWidth / 2.0f + bankWidth * 0.3f);
        leftUpper.y = bankHeight;
        bankVerts.push_back({leftUpper, glm::normalize(glm::vec3(-0.5f, 0.8f, 0)), {0.2f, t * 4.0f}});
        
        // Bank crest (highest point)
        glm::vec3 leftCrest = center - right * (waterWidth / 2.0f + 0.3f);
        leftCrest.y = bankHeight + 0.1f;
        bankVerts.push_back({leftCrest, glm::normalize(glm::vec3(-0.7f, 0.5f, 0)), {0.4f, t * 4.0f}});
        
        // Slope down to water
        glm::vec3 leftSlope = center - right * (waterWidth / 2.0f);
        leftSlope.y = waterSurfaceY + 0.05f;
        bankVerts.push_back({leftSlope, glm::normalize(glm::vec3(-0.9f, 0.2f, 0)), {0.5f, t * 4.0f}});
        
        // At water edge
        glm::vec3 leftWater = center - right * (waterWidth / 2.0f - 0.2f);
        leftWater.y = waterSurfaceY - 0.1f; // Go slightly under water
        bankVerts.push_back({leftWater, glm::normalize(glm::vec3(-0.3f, 0.95f, 0)), {0.6f, t * 4.0f}});
        
        // RIGHT BANK (mirror of left)
        glm::vec3 rightWater = center + right * (waterWidth / 2.0f - 0.2f);
        rightWater.y = waterSurfaceY - 0.1f;
        bankVerts.push_back({rightWater, glm::normalize(glm::vec3(0.3f, 0.95f, 0)), {0.6f, t * 4.0f}});
        
        glm::vec3 rightSlope = center + right * (waterWidth / 2.0f);
        rightSlope.y = waterSurfaceY + 0.05f;
        bankVerts.push_back({rightSlope, glm::normalize(glm::vec3(0.9f, 0.2f, 0)), {0.5f, t * 4.0f}});
        
        glm::vec3 rightCrest = center + right * (waterWidth / 2.0f + 0.3f);
        rightCrest.y = bankHeight + 0.1f;
        bankVerts.push_back({rightCrest, glm::normalize(glm::vec3(0.7f, 0.5f, 0)), {0.4f, t * 4.0f}});
        
        glm::vec3 rightUpper = center + right * (waterWidth / 2.0f + bankWidth * 0.3f);
        rightUpper.y = bankHeight;
        bankVerts.push_back({rightUpper, glm::normalize(glm::vec3(0.5f, 0.8f, 0)), {0.2f, t * 4.0f}});
        
        glm::vec3 rightOuter = center + right * (waterWidth / 2.0f + bankWidth);
        rightOuter.y = 0.05f;
        bankVerts.push_back({rightOuter, glm::normalize(glm::vec3(0.3f, 1.0f, 0)), {0, t * 4.0f}});
    }
    
    // Create bank triangles
    int bankVertsPerRow = 10;  // 5 left + 5 right
    for (int i = 0; i < lengthSegments; ++i) {
        for (int j = 0; j < bankVertsPerRow - 1; ++j) {
            int curr = i * bankVertsPerRow + j;
            int next = (i + 1) * bankVertsPerRow + j;
            
            bankInds.push_back(curr);
            bankInds.push_back(curr + 1);
            bankInds.push_back(next + 1);
            
            bankInds.push_back(curr);
            bankInds.push_back(next + 1);
            bankInds.push_back(next);
        }
    }
    
    s_riverBanks = new Mesh(bankVerts, bankInds);
    
    // ========================================================================
    // CREATE RIVER BED - Bottom of the river (visible through water)
    // ========================================================================
    std::vector<Vertex> bedVerts;
    std::vector<unsigned int> bedInds;
    
    int bedWidthSegs = 10;
    
    for (int i = 0; i <= lengthSegments; ++i) {
        float t = static_cast<float>(i) / lengthSegments;
        glm::vec3 center, direction, right;
        getSplineState(t, center, direction, right);
        
        for (int j = 0; j <= bedWidthSegs; ++j) {
            float s = static_cast<float>(j) / bedWidthSegs;
            
            float xOffset = (s - 0.5f) * (waterWidth - 0.4f);
            glm::vec3 pos = center + right * xOffset;
            
            // Curved riverbed (deeper in center)
            float depthCurve = std::pow(std::abs(s - 0.5f) * 2.0f, 2.0f);
            pos.y = waterSurfaceY - riverDepth * (1.0f - depthCurve * 0.6f);
            
            // Add some rocky texture variation
            float rockBump = std::sin(t * 50.0f) * std::cos(s * 30.0f) * 0.05f;
            pos.y += rockBump;
            
            bedVerts.push_back({pos, {0, 1, 0}, {s, t * 4.0f}});
        }
    }
    
    // Create riverbed triangles
    int bedVertsPerRow = bedWidthSegs + 1;
    for (int i = 0; i < lengthSegments; ++i) {
        for (int j = 0; j < bedWidthSegs; ++j) {
            int curr = i * bedVertsPerRow + j;
            int next = (i + 1) * bedVertsPerRow + j;
            
            bedInds.push_back(curr);
            bedInds.push_back(next + 1);
            bedInds.push_back(curr + 1);
            
            bedInds.push_back(curr);
            bedInds.push_back(next);
            bedInds.push_back(next + 1);
        }
    }
    
    s_riverBed = new Mesh(bedVerts, bedInds);
    
    // Store water mesh for main drawing
    water = s_riverWater;
}

void River::Update(float deltaTime) {
    flowOffset += deltaTime * 0.5f;
    if (flowOffset > 1.0f) flowOffset -= 1.0f;
    
    waveTime += deltaTime;
    if (waveTime > 100.0f) waveTime -= 100.0f;
}

void River::Draw(Shader& shader) {
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    
    // ========================================================================
    // DRAW RIVER BANKS - Brown earth/dirt material
    // ========================================================================
    Material bankMat;
    bankMat.ambient = glm::vec3(0.15f, 0.10f, 0.05f);
    bankMat.diffuse = glm::vec3(0.45f, 0.32f, 0.18f);  // Brown earth
    bankMat.specular = glm::vec3(0.08f, 0.06f, 0.04f);
    bankMat.shininess = 4.0f;
    
    shader.setVec3("material.ambient", bankMat.ambient);
    shader.setVec3("material.diffuse", bankMat.diffuse);
    shader.setVec3("material.specular", bankMat.specular);
    shader.setFloat("material.shininess", bankMat.shininess);
    shader.setFloat("textureOffset", 0.0f);
    
    s_riverBanks->Draw();
    
    // ========================================================================
    // DRAW RIVER BED - Darker brown/grey rocky bottom
    // ========================================================================
    Material bedMat;
    bedMat.ambient = glm::vec3(0.08f, 0.07f, 0.06f);
    bedMat.diffuse = glm::vec3(0.25f, 0.22f, 0.18f);  // Dark rocky
    bedMat.specular = glm::vec3(0.05f, 0.05f, 0.05f);
    bedMat.shininess = 2.0f;
    
    shader.setVec3("material.ambient", bedMat.ambient);
    shader.setVec3("material.diffuse", bedMat.diffuse);
    shader.setVec3("material.specular", bedMat.specular);
    shader.setFloat("material.shininess", bedMat.shininess);
    
    s_riverBed->Draw();
}

void River::DrawWaterSurface(Shader& shader) {
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    
    // ========================================================================
    // DRAW WATER SURFACE - Transparent water with texture and reflections
    // ========================================================================
    Material waterMat;
    waterMat.ambient = glm::vec3(0.1f, 0.2f, 0.3f);
    waterMat.diffuse = glm::vec3(0.2f, 0.5f, 0.7f);
    waterMat.specular = glm::vec3(0.8f, 0.9f, 1.0f);
    waterMat.shininess = 128.0f;
    
    shader.setVec3("material.ambient", waterMat.ambient);
    shader.setVec3("material.diffuse", waterMat.diffuse);
    shader.setVec3("material.specular", waterMat.specular);
    shader.setFloat("material.shininess", waterMat.shininess);
    shader.setFloat("textureOffset", flowOffset);
    
    // Bind water texture (blended with water color)
    if (waterTexture != 0 && enableTextures && enableWaterTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", textureBlendMode == 1);
    }
    
    // Enable transparency for water
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    shader.setFloat("alpha", 0.4f);
    
    s_riverWater->Draw();
    
    // Restore default state
    glDepthMask(GL_TRUE);
    shader.setFloat("alpha", 1.0f);
    glDisable(GL_BLEND);
    shader.setBool("useTexture", false);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// TREE IMPLEMENTATION
// Tree with trunk, branches, and spherical foliage
// ============================================================================

// ============================================================================
// TREE IMPLEMENTATION
// Recursive fractal tree generation
// ============================================================================

static Mesh* s_treeWood = nullptr;    // Holds trunk and branches
static Mesh* s_treeLeaves = nullptr;  // Holds all leaf clusters

// Static texture members
unsigned int Tree::treeTextures[3] = {0, 0, 0};
unsigned int Tree::leafTexture = 0;
bool Tree::texturesLoaded = false;

static unsigned int loadSingleTexture(const char* path) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    // Fallback: try with ../../ prefix (if running from x64/Debug/)
    std::string altPath;
    if (!data) {
        altPath = std::string("../../") + path;
        data = stbi_load(altPath.c_str(), &width, &height, &nrChannels, 0);
    }
    unsigned int texID = 0;
    if (data) {
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;
        else if (nrChannels == 1) format = GL_RED;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "SUCCESS: Loaded texture: " << path << " (" << width << "x" << height << ", " << nrChannels << " ch)" << std::endl;
    } else {
        std::cout << "ERROR: Failed to load texture from ALL paths: " << path << std::endl;
    }
    stbi_image_free(data);
    return texID;
}

void Tree::loadTextures() {
    if (texturesLoaded) return;
    
    stbi_set_flip_vertically_on_load(true);
    
    // Load bark/shade textures for trunk
    treeTextures[0] = loadSingleTexture("src/tree_shade.png");
    treeTextures[1] = loadSingleTexture("src/tree_shade2.png");
    treeTextures[2] = loadSingleTexture("src/tree_shade3.png");
    
    // Load leaf texture for foliage
    leafTexture = loadSingleTexture("src/tree_leaf1.png");
    
    texturesLoaded = true;
}

// Helper to add a branch segment (cylinder/cone) between two points
static void AddBranchSegment(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, 
                           glm::vec3 p1, glm::vec3 p2, float r1, float r2) {
    glm::vec3 axis = p2 - p1;
    float len = glm::length(axis);
    if (len < 0.001f) return;
    glm::vec3 dir = axis / len;
    
    // Arbitrary basis construction
    glm::vec3 tempUp = (glm::abs(dir.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, tempUp));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));
    
    int segments = 8;
    int baseIdx = static_cast<int>(verts.size());
    
    // Generate rings
    for (int i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float c = std::cos(angle);
        float s = std::sin(angle);
        glm::vec3 radial = right * c + up * s;
        
        // Vertices at start (p1) and end (p2)
        verts.push_back({p1 + radial * r1, radial, {static_cast<float>(i)/segments, 0}});
        verts.push_back({p2 + radial * r2, radial, {static_cast<float>(i)/segments, 1}});
    }
    
    // Generate indices
    for (int i = 0; i < segments; ++i) {
        int idx = baseIdx + i * 2;
        inds.push_back(idx);     inds.push_back(idx + 3); inds.push_back(idx + 1);
        inds.push_back(idx);     inds.push_back(idx + 2); inds.push_back(idx + 3);
    }
}

// Recursive function to generate tree structure
static void GenerateRecursively(
    std::vector<Vertex>& woodVerts, std::vector<unsigned int>& woodInds,
    std::vector<Vertex>& leafVerts, std::vector<unsigned int>& leafInds,
    glm::vec3 start, glm::vec3 dir, float length, float radius, int depth
) {
    glm::vec3 end = start + dir * length;
    
    // Add branch segment
    float radiusEnd = radius * 0.7f;
    AddBranchSegment(woodVerts, woodInds, start, end, radius, radiusEnd);
    
    if (depth <= 0) {
        // Add leaf cluster at tip - slightly squashed spheres for natural canopy
        float leafRad = 0.55f + (rand() % 100) / 100.0f * 0.4f;
        
        // Generate 5-7 tightly overlapping leaf clusters
        int numClusters = 5 + (rand() % 3);
        for (int i = 0; i < numClusters; i++) {
            glm::vec3 offset = glm::vec3(
                (rand()%100 - 50)/100.0f,
                (rand()%100 - 20)/100.0f * 0.4f,  // Bias upward
                (rand()%100 - 50)/100.0f
            ) * 0.55f;
            
            glm::vec3 center = end + offset;
            
            int stacks = 12;
            int slices = 16;
            
            for (int lat = 0; lat <= stacks; ++lat) {
                float theta = lat * static_cast<float>(M_PI) / stacks;
                float sinTheta = std::sin(theta);
                float cosTheta = std::cos(theta);
                
                for (int lon = 0; lon <= slices; ++lon) {
                    float phi = lon * 2.0f * static_cast<float>(M_PI) / slices;
                    float sinPhi = std::sin(phi);
                    float cosPhi = std::cos(phi);
                    
                    glm::vec3 normal(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
                    // Slightly squash vertically for natural canopy look
                    glm::vec3 pos = center + glm::vec3(normal.x * leafRad, normal.y * leafRad * 0.75f, normal.z * leafRad);
                    
                    leafVerts.push_back({pos, normal, {(float)lon/slices * 3.0f, (float)lat/stacks * 3.0f}});
                }
            }
            
            int baseIdx = static_cast<int>(leafVerts.size()) - (stacks + 1) * (slices + 1);
            for (int lat = 0; lat < stacks; ++lat) {
                for (int lon = 0; lon < slices; ++lon) {
                    int first = baseIdx + lat * (slices + 1) + lon;
                    int second = first + slices + 1;
                    
                    leafInds.push_back(first);
                    leafInds.push_back(second);
                    leafInds.push_back(first + 1);
                    
                    leafInds.push_back(second);
                    leafInds.push_back(second + 1);
                    leafInds.push_back(first + 1);
                }
            }
        }
        return;
    }
    
    // Add foliage at branch junctions to hide bare wood
    if (depth <= 2) {
        float junctionLeafRad = 0.5f + (rand() % 100) / 100.0f * 0.3f;
        glm::vec3 jOffset(0, 0.2f, 0);
        glm::vec3 jCenter = end + jOffset;
        int jStacks = 8, jSlices = 10;
        for (int lat = 0; lat <= jStacks; ++lat) {
            float theta = lat * static_cast<float>(M_PI) / jStacks;
            float sinT = std::sin(theta), cosT = std::cos(theta);
            for (int lon = 0; lon <= jSlices; ++lon) {
                float phi = lon * 2.0f * static_cast<float>(M_PI) / jSlices;
                glm::vec3 n(std::cos(phi)*sinT, cosT, std::sin(phi)*sinT);
                glm::vec3 p = jCenter + glm::vec3(n.x*junctionLeafRad, n.y*junctionLeafRad*0.7f, n.z*junctionLeafRad);
                leafVerts.push_back({p, n, {(float)lon/jSlices * 3.0f, (float)lat/jStacks * 3.0f}});
            }
        }
        int jBase = static_cast<int>(leafVerts.size()) - (jStacks+1)*(jSlices+1);
        for (int lat = 0; lat < jStacks; ++lat) {
            for (int lon = 0; lon < jSlices; ++lon) {
                int f = jBase + lat*(jSlices+1)+lon, s = f+jSlices+1;
                leafInds.push_back(f); leafInds.push_back(s); leafInds.push_back(f+1);
                leafInds.push_back(s); leafInds.push_back(s+1); leafInds.push_back(f+1);
            }
        }
    }
    
    // Recursive branches
    int numBranches = 2 + (rand() % 2);
    for (int i = 0; i < numBranches; ++i) {
        float angleX = (rand() % 100 - 50) / 50.0f * 0.8f;
        float angleZ = (rand() % 100 - 50) / 50.0f * 0.8f;
        
        glm::vec3 newDir = dir + glm::vec3(angleX, 0.5f, angleZ);
        newDir = glm::normalize(newDir);
        
        GenerateRecursively(woodVerts, woodInds, leafVerts, leafInds, 
                          end, newDir, length * 0.75f, radiusEnd, depth - 1);
    }
}

Tree::Tree(glm::vec3 pos, float s, float rot)
    : position(pos), scale(s), rotation(rot) {
    // Pick one of the 3 textures based on rotation (gives variety)
    textureIndex = static_cast<int>(std::abs(rot)) % 3;
    initMeshes();
}

void Tree::initMeshes() {
    if (!s_treeWood) {
        std::vector<Vertex> woodVerts;
        std::vector<unsigned int> woodInds;
        std::vector<Vertex> leafVerts;
        std::vector<unsigned int> leafInds;
        
        // Start recursion from base
        srand(12345);
        GenerateRecursively(woodVerts, woodInds, leafVerts, leafInds,
                          glm::vec3(0,0,0), glm::vec3(0,1,0), 3.0f, 0.4f, 4);
                          
        s_treeWood = new Mesh(woodVerts, woodInds);
        s_treeLeaves = new Mesh(leafVerts, leafInds);
        
        // Load tree shade textures
        loadTextures();
    }
    
    trunk = s_treeWood;
    foliage = s_treeLeaves;
    branches = s_treeWood;
}

void Tree::Draw(Shader& shader) {
    glm::mat4 baseTransform = glm::mat4(1.0f);
    baseTransform = glm::translate(baseTransform, position);
    baseTransform = glm::rotate(baseTransform, glm::radians(rotation), glm::vec3(0, 1, 0));
    baseTransform = glm::scale(baseTransform, glm::vec3(scale));
    
    // Draw Wood (Trunk + Branches) - WITH TEXTURE MAPPING
    Material barkMat;
    barkMat.ambient = glm::vec3(0.5f, 0.4f, 0.35f);
    barkMat.diffuse = glm::vec3(0.6f, 0.4f, 0.25f);
    barkMat.specular = glm::vec3(0.05f, 0.05f, 0.05f);
    barkMat.shininess = 8.0f;
    
    shader.setVec3("material.ambient", barkMat.ambient);
    shader.setVec3("material.diffuse", barkMat.diffuse);
    shader.setVec3("material.specular", barkMat.specular);
    shader.setFloat("material.shininess", barkMat.shininess);
    
    // Bind tree shade texture for trunk/body
    if (texturesLoaded && treeTextures[textureIndex] != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, treeTextures[textureIndex]);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", false);       // NOT simple mode
        shader.setBool("useMultiplyBlend", true);    // Multiply blend for natural look
    } else {
        shader.setBool("useTexture", false);
    }
    
    // Pass material to vertex shader for Gouraud mode
    if (textureBlendMode == 2) {
        shader.setVec3("vertexMatAmbient", barkMat.ambient);
        shader.setVec3("vertexMatDiffuse", barkMat.diffuse);
    }
    
    shader.setMat4("model", baseTransform);
    trunk->Draw();
    
    // Reset texture after trunk
    shader.setBool("useTexture", false);
    shader.setBool("useMultiplyBlend", false);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Draw Foliage - WITH LEAF TEXTURE
    Material leafMat;
    leafMat.ambient = glm::vec3(0.3f, 0.5f, 0.2f);     // Bright natural green
    leafMat.diffuse = glm::vec3(0.4f, 0.7f, 0.3f);     // Vibrant lush green
    leafMat.specular = glm::vec3(0.12f, 0.18f, 0.1f);  // Slight waxy sheen
    leafMat.shininess = 10.0f;
    
    shader.setVec3("material.ambient", leafMat.ambient);
    shader.setVec3("material.diffuse", leafMat.diffuse);
    shader.setVec3("material.specular", leafMat.specular);
    shader.setFloat("material.shininess", leafMat.shininess);
    
    // Bind leaf texture for foliage
    if (texturesLoaded && leafTexture != 0 && enableTextures) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leafTexture);
        shader.setInt("texture_diffuse", 0);
        shader.setBool("useTexture", true);
        shader.setBool("textureOnly", false);       // NOT simple mode
        shader.setBool("useMultiplyBlend", true);    // Multiply blend for natural green
    } else {
        shader.setBool("useTexture", false);
    }
    
    // Pass material to vertex shader for Gouraud mode
    if (textureBlendMode == 2) {
        shader.setVec3("vertexMatAmbient", leafMat.ambient);
        shader.setVec3("vertexMatDiffuse", leafMat.diffuse);
    }
    
    shader.setMat4("model", baseTransform);
    foliage->Draw();
    
    // Reset texture state
    shader.setBool("useTexture", false);
    shader.setBool("useMultiplyBlend", false);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// BIRD FLOCK IMPLEMENTATION
// Animated birds flying through the sky with flapping wings
// ============================================================================

static Mesh* s_birdBody = nullptr;
static Mesh* s_birdWing = nullptr;

// Create a small elongated body (like a torpedo shape using a scaled cube)
static Mesh CreateBirdBodyMesh() {
    // Simple elongated diamond/body shape
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    // Body is a flattened elongated shape along Z-axis
    float len = 0.4f;   // body length
    float wid = 0.08f;  // body width
    float hgt = 0.06f;  // body height

    // Nose (front point)
    glm::vec3 nose(0, 0, len / 2.0f);
    // Tail (back point)
    glm::vec3 tail(0, 0, -len / 2.0f);
    // Mid points (diamond cross-section)
    glm::vec3 top(0, hgt, 0);
    glm::vec3 bot(0, -hgt, 0);
    glm::vec3 left(-wid, 0, 0);
    glm::vec3 right(wid, 0, 0);

    // Top-front triangles
    verts.push_back({nose,  {0, 1, 1}, {0.5f, 1}});
    verts.push_back({top,   {0, 1, 0}, {0.5f, 0.5f}});
    verts.push_back({right, {1, 1, 0}, {1, 0.5f}});
    inds.push_back(0); inds.push_back(1); inds.push_back(2);

    verts.push_back({nose,  {0, 1, 1}, {0.5f, 1}});
    verts.push_back({left,  {-1, 1, 0}, {0, 0.5f}});
    verts.push_back({top,   {0, 1, 0}, {0.5f, 0.5f}});
    inds.push_back(3); inds.push_back(4); inds.push_back(5);

    // Bottom-front triangles
    verts.push_back({nose,  {0, -1, 1}, {0.5f, 1}});
    verts.push_back({right, {1, -1, 0}, {1, 0.5f}});
    verts.push_back({bot,   {0, -1, 0}, {0.5f, 0.5f}});
    inds.push_back(6); inds.push_back(7); inds.push_back(8);

    verts.push_back({nose,  {0, -1, 1}, {0.5f, 1}});
    verts.push_back({bot,   {0, -1, 0}, {0.5f, 0.5f}});
    verts.push_back({left,  {-1, -1, 0}, {0, 0.5f}});
    inds.push_back(9); inds.push_back(10); inds.push_back(11);

    // Top-back triangles
    verts.push_back({tail,  {0, 1, -1}, {0.5f, 0}});
    verts.push_back({right, {1, 1, 0}, {1, 0.5f}});
    verts.push_back({top,   {0, 1, 0}, {0.5f, 0.5f}});
    inds.push_back(12); inds.push_back(13); inds.push_back(14);

    verts.push_back({tail,  {0, 1, -1}, {0.5f, 0}});
    verts.push_back({top,   {0, 1, 0}, {0.5f, 0.5f}});
    verts.push_back({left,  {-1, 1, 0}, {0, 0.5f}});
    inds.push_back(15); inds.push_back(16); inds.push_back(17);

    // Bottom-back triangles
    verts.push_back({tail,  {0, -1, -1}, {0.5f, 0}});
    verts.push_back({bot,   {0, -1, 0}, {0.5f, 0.5f}});
    verts.push_back({right, {1, -1, 0}, {1, 0.5f}});
    inds.push_back(18); inds.push_back(19); inds.push_back(20);

    verts.push_back({tail,  {0, -1, -1}, {0.5f, 0}});
    verts.push_back({left,  {-1, -1, 0}, {0, 0.5f}});
    verts.push_back({bot,   {0, -1, 0}, {0.5f, 0.5f}});
    inds.push_back(21); inds.push_back(22); inds.push_back(23);

    // Tail fin (two flat triangles forming a V tail)
    glm::vec3 tailTipL(-0.12f, 0.08f, -len / 2.0f - 0.15f);
    glm::vec3 tailTipR(0.12f, 0.08f, -len / 2.0f - 0.15f);

    int b = (int)verts.size();
    verts.push_back({tail,    {0, 1, -1}, {0.5f, 0}});
    verts.push_back({tailTipL, {-1, 1, -1}, {0, 0}});
    verts.push_back({tailTipR, {1, 1, -1}, {1, 0}});
    inds.push_back(b); inds.push_back(b + 1); inds.push_back(b + 2);
    // Back face
    inds.push_back(b); inds.push_back(b + 2); inds.push_back(b + 1);

    return Mesh(verts, inds);
}

// Create a single wing (flat triangle that can be rotated for flapping)
static Mesh CreateBirdWingMesh() {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    // Wing is a flat triangle extending along X, attached at X=0
    // Root is at origin, tip extends outward
    float wingLen = 0.5f;
    float wingChord = 0.2f; // front-to-back

    glm::vec3 root(0, 0, wingChord * 0.3f);
    glm::vec3 mid(wingLen * 0.6f, 0, wingChord * 0.1f);
    glm::vec3 tip(wingLen, 0, 0);
    glm::vec3 back(0, 0, -wingChord * 0.7f);
    glm::vec3 midBack(wingLen * 0.4f, 0, -wingChord * 0.3f);

    glm::vec3 up(0, 1, 0);

    // Top face
    int b = (int)verts.size();
    verts.push_back({root,    up, {0, 0.5f}});
    verts.push_back({mid,     up, {0.6f, 0.5f}});
    verts.push_back({tip,     up, {1, 0.5f}});
    verts.push_back({midBack, up, {0.4f, 0}});
    verts.push_back({back,    up, {0, 0}});

    // Triangle fan from root
    inds.push_back(b); inds.push_back(b + 1); inds.push_back(b + 3);
    inds.push_back(b + 1); inds.push_back(b + 2); inds.push_back(b + 3);
    inds.push_back(b); inds.push_back(b + 3); inds.push_back(b + 4);

    // Bottom face (reversed normals)
    glm::vec3 dn(0, -1, 0);
    b = (int)verts.size();
    verts.push_back({root,    dn, {0, 0.5f}});
    verts.push_back({mid,     dn, {0.6f, 0.5f}});
    verts.push_back({tip,     dn, {1, 0.5f}});
    verts.push_back({midBack, dn, {0.4f, 0}});
    verts.push_back({back,    dn, {0, 0}});

    inds.push_back(b); inds.push_back(b + 3); inds.push_back(b + 1);
    inds.push_back(b + 1); inds.push_back(b + 3); inds.push_back(b + 2);
    inds.push_back(b); inds.push_back(b + 4); inds.push_back(b + 3);

    return Mesh(verts, inds);
}

BirdFlock::BirdFlock(glm::vec3 center, int numBirds)
    : flockCenter(center) {
    initMeshes();

    // Create birds with varied circular flight paths
    srand(42); // Deterministic seed for reproducibility
    for (int i = 0; i < numBirds; ++i) {
        Bird bird;
        bird.circleRadius = 15.0f + (rand() % 30);           // 15-45 radius
        bird.circleAngle = (float)(rand() % 360);             // Random start angle
        bird.circleHeight = 20.0f + (float)(rand() % 20);     // 20-40 height
        bird.circleSpeed = 0.3f + (float)(rand() % 100) / 200.0f; // 0.3-0.8 rad/s
        bird.flapPhase = (float)(rand() % 100) / 100.0f * 6.28f;  // Random phase
        bird.flapSpeed = 6.0f + (float)(rand() % 40) / 10.0f;     // 6-10 Hz
        bird.speed = 3.0f + (float)(rand() % 20) / 10.0f;
        bird.bankAngle = 0.0f;

        // Initial position on the circle
        float rad = glm::radians(bird.circleAngle);
        bird.position = center + glm::vec3(
            cos(rad) * bird.circleRadius,
            bird.circleHeight,
            sin(rad) * bird.circleRadius
        );

        birds.push_back(bird);
    }
}

void BirdFlock::initMeshes() {
    if (!s_birdBody) {
        s_birdBody = new Mesh(CreateBirdBodyMesh());
    }
    if (!s_birdWing) {
        s_birdWing = new Mesh(CreateBirdWingMesh());
    }
    bodyMesh = s_birdBody;
    wingMesh = s_birdWing;
}

void BirdFlock::Update(float deltaTime) {
    for (auto& bird : birds) {
        // Advance along circular path
        bird.circleAngle += bird.circleSpeed * deltaTime * 50.0f; // degrees per second
        if (bird.circleAngle >= 360.0f) bird.circleAngle -= 360.0f;

        float rad = glm::radians(bird.circleAngle);

        // Slight vertical oscillation for more natural flight
        float vertOsc = sin(rad * 2.0f) * 1.5f;

        bird.position = flockCenter + glm::vec3(
            cos(rad) * bird.circleRadius,
            bird.circleHeight + vertOsc,
            sin(rad) * bird.circleRadius
        );

        // Update wing flap
        bird.flapPhase += bird.flapSpeed * deltaTime;
        if (bird.flapPhase > 6.2831853f) bird.flapPhase -= 6.2831853f;

        // Banking angle follows the turn (birds bank inward when circling)
        bird.bankAngle = -15.0f; // Constant slight bank for circular flight
    }
}

void BirdFlock::Draw(Shader& shader) {
    // Disable backface culling for thin bird geometry
    GLboolean cullingOn;
    glGetBooleanv(GL_CULL_FACE, &cullingOn);
    glDisable(GL_CULL_FACE);

    for (auto& bird : birds) {
        // Calculate heading direction (tangent to circle)
        float rad = glm::radians(bird.circleAngle);
        float headingAngle = bird.circleAngle + 90.0f; // Tangent to circle

        // Wing flap angle (smooth sinusoidal)
        float flapAngle = sin(bird.flapPhase) * 35.0f; // ±35 degrees

        // -- BODY --
        // Dark bird color (like a crow/starling)
        shader.setVec3("material.ambient", glm::vec3(0.05f, 0.05f, 0.08f));
        shader.setVec3("material.diffuse", glm::vec3(0.12f, 0.12f, 0.15f));
        shader.setVec3("material.specular", glm::vec3(0.3f, 0.3f, 0.35f));
        shader.setFloat("material.shininess", 32.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird.position);
        model = glm::rotate(model, glm::radians(-headingAngle), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(bird.bankAngle), glm::vec3(0, 0, 1));

        shader.setMat4("model", model);
        bodyMesh->Draw();

        // -- RIGHT WING (positive X) --
        shader.setVec3("material.ambient", glm::vec3(0.04f, 0.04f, 0.06f));
        shader.setVec3("material.diffuse", glm::vec3(0.10f, 0.10f, 0.13f));

        glm::mat4 rightWing = model;
        rightWing = glm::translate(rightWing, glm::vec3(0.05f, 0.0f, 0.0f));
        rightWing = glm::rotate(rightWing, glm::radians(flapAngle), glm::vec3(0, 0, 1));
        shader.setMat4("model", rightWing);
        wingMesh->Draw();

        // -- LEFT WING (negative X, mirrored) --
        glm::mat4 leftWing = model;
        leftWing = glm::translate(leftWing, glm::vec3(-0.05f, 0.0f, 0.0f));
        leftWing = glm::scale(leftWing, glm::vec3(-1, 1, 1)); // Mirror
        leftWing = glm::rotate(leftWing, glm::radians(flapAngle), glm::vec3(0, 0, 1));
        shader.setMat4("model", leftWing);
        wingMesh->Draw();
    }

    // Restore culling state
    if (cullingOn) glEnable(GL_CULL_FACE);
}

