#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    void Draw() const;

    // Static factory methods for primitive shapes
    static Mesh CreateCube(float size = 1.0f);
    static Mesh CreatePlane(float width, float depth, int subdivisions = 1);
    static Mesh CreateCylinder(float radius, float height, int segments = 32);
    static Mesh CreateCone(float radius, float height, int segments = 32);
    static Mesh CreateSphere(float radius, int stacks = 16, int slices = 32);

private:
    unsigned int VBO, EBO;
    void setupMesh();
};

#endif
