#include "Mesh.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : vertices(vertices), indices(indices) {
    setupMesh();
}

Mesh::~Mesh() {
    // Note: In a full implementation, you'd want proper resource management
    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);
    // glDeleteBuffers(1, &EBO);
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Vertex Texture Coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void Mesh::Draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Mesh Mesh::CreateCube(float size) {
    float s = size / 2.0f;
    std::vector<Vertex> vertices = {
        // Front face
        {{-s, -s,  s}, { 0,  0,  1}, {0, 0}},
        {{ s, -s,  s}, { 0,  0,  1}, {1, 0}},
        {{ s,  s,  s}, { 0,  0,  1}, {1, 1}},
        {{-s,  s,  s}, { 0,  0,  1}, {0, 1}},
        // Back face
        {{ s, -s, -s}, { 0,  0, -1}, {0, 0}},
        {{-s, -s, -s}, { 0,  0, -1}, {1, 0}},
        {{-s,  s, -s}, { 0,  0, -1}, {1, 1}},
        {{ s,  s, -s}, { 0,  0, -1}, {0, 1}},
        // Top face
        {{-s,  s,  s}, { 0,  1,  0}, {0, 0}},
        {{ s,  s,  s}, { 0,  1,  0}, {1, 0}},
        {{ s,  s, -s}, { 0,  1,  0}, {1, 1}},
        {{-s,  s, -s}, { 0,  1,  0}, {0, 1}},
        // Bottom face
        {{-s, -s, -s}, { 0, -1,  0}, {0, 0}},
        {{ s, -s, -s}, { 0, -1,  0}, {1, 0}},
        {{ s, -s,  s}, { 0, -1,  0}, {1, 1}},
        {{-s, -s,  s}, { 0, -1,  0}, {0, 1}},
        // Right face
        {{ s, -s,  s}, { 1,  0,  0}, {0, 0}},
        {{ s, -s, -s}, { 1,  0,  0}, {1, 0}},
        {{ s,  s, -s}, { 1,  0,  0}, {1, 1}},
        {{ s,  s,  s}, { 1,  0,  0}, {0, 1}},
        // Left face
        {{-s, -s, -s}, {-1,  0,  0}, {0, 0}},
        {{-s, -s,  s}, {-1,  0,  0}, {1, 0}},
        {{-s,  s,  s}, {-1,  0,  0}, {1, 1}},
        {{-s,  s, -s}, {-1,  0,  0}, {0, 1}},
    };

    std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    return Mesh(vertices, indices);
}

Mesh Mesh::CreatePlane(float width, float depth, int subdivisions) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;
    float stepX = width / subdivisions;
    float stepZ = depth / subdivisions;

    for (int z = 0; z <= subdivisions; ++z) {
        for (int x = 0; x <= subdivisions; ++x) {
            float px = -halfW + x * stepX;
            float pz = -halfD + z * stepZ;
            float u = static_cast<float>(x) / subdivisions;
            float v = static_cast<float>(z) / subdivisions;
            vertices.push_back({{px, 0, pz}, {0, 1, 0}, {u, v}});
        }
    }

    for (int z = 0; z < subdivisions; ++z) {
        for (int x = 0; x < subdivisions; ++x) {
            int topLeft = z * (subdivisions + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (subdivisions + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateCylinder(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfH = height / 2.0f;

    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        float u = static_cast<float>(i) / segments;

        // Bottom vertex
        vertices.push_back({{x, -halfH, z}, {cos(angle), 0, sin(angle)}, {u, 0}});
        // Top vertex
        vertices.push_back({{x, halfH, z}, {cos(angle), 0, sin(angle)}, {u, 1}});
    }

    // Side indices
    for (int i = 0; i < segments; ++i) {
        int base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
    }

    // Top cap
    int topCenter = static_cast<int>(vertices.size());
    vertices.push_back({{0, halfH, 0}, {0, 1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back({{x, halfH, z}, {0, 1, 0}, {cos(angle) * 0.5f + 0.5f, sin(angle) * 0.5f + 0.5f}});
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(topCenter);
        indices.push_back(topCenter + i + 1);
        indices.push_back(topCenter + i + 2);
    }

    // Bottom cap
    int bottomCenter = static_cast<int>(vertices.size());
    vertices.push_back({{0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back({{x, -halfH, z}, {0, -1, 0}, {cos(angle) * 0.5f + 0.5f, sin(angle) * 0.5f + 0.5f}});
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(bottomCenter);
        indices.push_back(bottomCenter + i + 2);
        indices.push_back(bottomCenter + i + 1);
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateCone(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfH = height / 2.0f;

    // Apex vertex
    vertices.push_back({{0, halfH, 0}, {0, 1, 0}, {0.5f, 1}});

    // Base circle vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        
        // Calculate normal for cone side
        glm::vec3 normal = glm::normalize(glm::vec3(cos(angle), radius / height, sin(angle)));
        float u = static_cast<float>(i) / segments;
        
        vertices.push_back({{x, -halfH, z}, normal, {u, 0}});
    }

    // Side triangles
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }

    // Base cap
    int baseCenter = static_cast<int>(vertices.size());
    vertices.push_back({{0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back({{x, -halfH, z}, {0, -1, 0}, {cos(angle) * 0.5f + 0.5f, sin(angle) * 0.5f + 0.5f}});
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(baseCenter);
        indices.push_back(baseCenter + i + 2);
        indices.push_back(baseCenter + i + 1);
    }

    return Mesh(vertices, indices);
}

Mesh Mesh::CreateSphere(float radius, int stacks, int slices) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = static_cast<float>(M_PI) * i / stacks;
        float y = cos(phi) * radius;
        float r = sin(phi) * radius;

        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * static_cast<float>(M_PI) * j / slices;
            float x = cos(theta) * r;
            float z = sin(theta) * r;

            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 uv(static_cast<float>(j) / slices, static_cast<float>(i) / stacks);

            vertices.push_back({pos, normal, uv});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
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
