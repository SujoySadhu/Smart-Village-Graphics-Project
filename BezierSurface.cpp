#include "BezierSurface.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

glm::vec2 BezierSurface::EvaluateCubicBezier(
    const glm::vec2& p0, const glm::vec2& p1,
    const glm::vec2& p2, const glm::vec2& p3,
    float t) {
    
    // De Casteljau's algorithm implementation
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    // B(t) = (1-t)^3 * P0 + 3*(1-t)^2*t * P1 + 3*(1-t)*t^2 * P2 + t^3 * P3
    glm::vec2 point = uuu * p0;           // (1-t)^3 * P0
    point += 3.0f * uu * t * p1;          // 3*(1-t)^2*t * P1
    point += 3.0f * u * tt * p2;          // 3*(1-t)*t^2 * P2
    point += ttt * p3;                     // t^3 * P3

    return point;
}

glm::vec2 BezierSurface::EvaluateCubicBezierTangent(
    const glm::vec2& p0, const glm::vec2& p1,
    const glm::vec2& p2, const glm::vec2& p3,
    float t) {
    
    // Derivative of cubic Bezier: B'(t) = 3*(1-t)^2*(P1-P0) + 6*(1-t)*t*(P2-P1) + 3*t^2*(P3-P2)
    float u = 1.0f - t;
    
    glm::vec2 tangent = 3.0f * u * u * (p1 - p0);
    tangent += 6.0f * u * t * (p2 - p1);
    tangent += 3.0f * t * t * (p3 - p2);

    return tangent;
}

glm::vec2 BezierSurface::EvaluateBezier(const std::vector<glm::vec2>& controlPoints, float t) {
    // General Bezier curve using Bernstein polynomials
    int n = static_cast<int>(controlPoints.size()) - 1;
    glm::vec2 point(0.0f);

    for (int i = 0; i <= n; ++i) {
        // Calculate binomial coefficient C(n, i)
        float binomial = 1.0f;
        for (int j = 0; j < i; ++j) {
            binomial *= static_cast<float>(n - j) / static_cast<float>(j + 1);
        }

        // Bernstein basis polynomial: B_{i,n}(t) = C(n,i) * t^i * (1-t)^(n-i)
        float basis = binomial * std::pow(t, i) * std::pow(1.0f - t, n - i);
        point += basis * controlPoints[i];
    }

    return point;
}

Mesh BezierSurface::CreateSurfaceOfRevolution(
    const std::vector<glm::vec2>& controlPoints,
    int curveSegments,
    int rotationSegments) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Step 1: Evaluate points along the Bezier curve profile
    std::vector<glm::vec2> profilePoints;
    for (int i = 0; i <= curveSegments; ++i) {
        float t = static_cast<float>(i) / curveSegments;
        profilePoints.push_back(EvaluateBezier(controlPoints, t));
    }

    // Step 2: Create the surface of revolution by rotating the profile around Y-axis
    for (int i = 0; i <= curveSegments; ++i) {
        for (int j = 0; j <= rotationSegments; ++j) {
            float angle = 2.0f * static_cast<float>(M_PI) * j / rotationSegments;

            // Profile point: x = radius, y = height
            float radius = profilePoints[i].x;
            float height = profilePoints[i].y;

            // 3D position after rotation around Y-axis
            glm::vec3 position(
                radius * std::cos(angle),
                height,
                radius * std::sin(angle)
            );

            // Calculate tangent along the profile curve
            glm::vec2 tangent;
            if (i == 0) {
                tangent = profilePoints[1] - profilePoints[0];
            } else if (i == curveSegments) {
                tangent = profilePoints[curveSegments] - profilePoints[curveSegments - 1];
            } else {
                tangent = profilePoints[i + 1] - profilePoints[i - 1];
            }
            
            // Handle zero-length tangent
            float tangentLen = glm::length(tangent);
            if (tangentLen > 0.0001f) {
                tangent = tangent / tangentLen;
            } else {
                tangent = glm::vec2(0.0f, 1.0f);
            }

            // Normal in 2D profile: perpendicular to tangent, pointing OUTWARD
            // For a profile going from bottom to top (increasing Y), 
            // the outward normal should point in +X direction at the base
            // tangent = (dx, dy), normal should be (dy, -dx) for outward facing
            glm::vec2 normal2D(tangent.y, -tangent.x);

            // Rotate normal to 3D around Y-axis
            glm::vec3 normal(
                normal2D.x * std::cos(angle),
                normal2D.y,
                normal2D.x * std::sin(angle)
            );
            
            // Normalize, with fallback for degenerate cases
            float normalLen = glm::length(normal);
            if (normalLen > 0.0001f) {
                normal = normal / normalLen;
            } else {
                // At apex, normal points up
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Texture coordinates
            glm::vec2 texCoords(
                static_cast<float>(j) / rotationSegments,
                static_cast<float>(i) / curveSegments
            );

            vertices.push_back({position, normal, texCoords});
        }
    }

    // Step 3: Generate indices for triangle mesh
    for (int i = 0; i < curveSegments; ++i) {
        for (int j = 0; j < rotationSegments; ++j) {
            int row1 = i * (rotationSegments + 1);
            int row2 = (i + 1) * (rotationSegments + 1);

            // Two triangles per quad
            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row1 + j + 1);

            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh BezierSurface::CreateDome(float radius, float height, int segments) {
    // Define control points for a dome profile (in XY plane)
    // Starting from bottom edge, curving up to the apex
    std::vector<glm::vec2> controlPoints = {
        {radius, 0.0f},                           // Bottom edge
        {radius, height * 0.5f},                  // Control point for curve
        {radius * 0.5f, height * 0.9f},           // Upper curve control
        {0.0f, height}                            // Apex
    };

    return CreateSurfaceOfRevolution(controlPoints, segments, segments);
}

Mesh BezierSurface::CreateWaterTank(float radius, float bodyHeight, float domeHeight, int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Create cylindrical body
    Mesh body = Mesh::CreateCylinder(radius, bodyHeight, segments);
    
    // Create dome top using Bezier revolution
    std::vector<glm::vec2> domeControlPoints = {
        {radius, bodyHeight / 2.0f},                          // Base of dome (at cylinder top)
        {radius, bodyHeight / 2.0f + domeHeight * 0.4f},     // Control point
        {radius * 0.4f, bodyHeight / 2.0f + domeHeight * 0.8f}, // Upper control
        {0.0f, bodyHeight / 2.0f + domeHeight}               // Apex
    };

    Mesh dome = CreateSurfaceOfRevolution(domeControlPoints, segments / 2, segments);

    // Combine meshes (simplified - just return dome for now, full combination would need index offset)
    // In practice, you'd render both separately or properly combine vertex/index buffers
    
    // For this implementation, we return a complete water tank shape
    // by creating a combined profile
    std::vector<glm::vec2> tankProfile = {
        {0.0f, -bodyHeight / 2.0f},                           // Bottom center
        {radius * 0.9f, -bodyHeight / 2.0f},                  // Bottom edge
        {radius, -bodyHeight / 2.0f + 0.1f},                  // Bottom corner
        {radius, bodyHeight / 2.0f - 0.1f},                   // Top of cylinder
        {radius, bodyHeight / 2.0f},                          // Start of dome
        {radius, bodyHeight / 2.0f + domeHeight * 0.4f},     // Dome control
        {radius * 0.4f, bodyHeight / 2.0f + domeHeight * 0.8f},
        {0.0f, bodyHeight / 2.0f + domeHeight}               // Top
    };

    return CreateSurfaceOfRevolution(tankProfile, segments, segments);
}

glm::vec3 BezierSurface::EvaluateCatmullRomSpline(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3,
    float t) {
    // Catmull-Rom Spline Formula
    float t2 = t * t;
    float t3 = t2 * t;

    glm::vec3 result = 
        0.5f * ((2.0f * p1) +
                (-p0 + p2) * t +
                (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    return result;
}

glm::vec3 BezierSurface::EvaluateCubicBezier3D(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3,
    float t) {
    
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 point = uuu * p0;
    point += 3.0f * uu * t * p1;
    point += 3.0f * u * tt * p2;
    point += ttt * p3;

    return point;
}

Mesh BezierSurface::CreateRuledSurface(
    const std::vector<glm::vec3>& curve1,
    const std::vector<glm::vec3>& curve2,
    int vSegments) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if(curve1.empty() || curve2.empty() || curve1.size() != curve2.size()) {
        return Mesh(vertices, indices); // Invalid input
    }
    
    int uSegments = static_cast<int>(curve1.size()) - 1;

    // Generate grid of vertices
    for (int i = 0; i <= uSegments; ++i) {
        glm::vec3 p1 = curve1[i];
        glm::vec3 p2 = curve2[i];
        
        for (int j = 0; j <= vSegments; ++j) {
            float v = static_cast<float>(j) / vSegments;
            float u = static_cast<float>(i) / uSegments;
            
            // Linear interpolation between the two curves
            glm::vec3 position = p1 + v * (p2 - p1);
            
            // Approximate Normal Calculation (Cross product of partial derivatives)
            glm::vec3 du, dv;
            
            // dv is the vector from curve1 to curve2
            dv = p2 - p1;
            
            // du is the tangent along the curve at this location
            glm::vec3 du1, du2;
            if (i < uSegments) {
                du1 = curve1[i+1] - curve1[i];
                du2 = curve2[i+1] - curve2[i];
            } else {
                du1 = curve1[i] - curve1[i-1];
                du2 = curve2[i] - curve2[i-1];
            }
            du = du1 + v * (du2 - du1); // blend tangents
            
            glm::vec3 normal = glm::cross(dv, du);
            float len = glm::length(normal);
            if(len > 0.0001f) normal = normal / len;
            else normal = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec2 texCoords(u, v);
            vertices.push_back({position, normal, texCoords});
        }
    }

    // Generate indices
    for (int i = 0; i < uSegments; ++i) {
        for (int j = 0; j < vSegments; ++j) {
            int row1 = i * (vSegments + 1);
            int row2 = (i + 1) * (vSegments + 1);

            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row1 + j + 1);

            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh BezierSurface::CreateCurveTube(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3,
    float radius, int curveSegments, int radialSegments) {
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> tangents;

    // Evaluate Curve Points and Tangents
    for (int i = 0; i <= curveSegments; ++i) {
        float t = static_cast<float>(i) / curveSegments;
        points.push_back(EvaluateCubicBezier3D(p0, p1, p2, p3, t));
        
        // Approximate Tangent (Central difference)
        glm::vec3 tangent(0.0f);
        if (i == 0) tangent = EvaluateCubicBezier3D(p0,p1,p2,p3, 0.01f) - points[0];
        else if (i == curveSegments) tangent = points[i] - EvaluateCubicBezier3D(p0,p1,p2,p3, 0.99f);
        else tangent = EvaluateCubicBezier3D(p0,p1,p2,p3, t + 0.01f) - EvaluateCubicBezier3D(p0,p1,p2,p3, t - 0.01f);
        
        tangents.push_back(glm::normalize(tangent));
    }

    // Generate Tube Geometry using Frenet-Serret framing (or simplified Bishop frame)
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    for (int i = 0; i <= curveSegments; ++i) {
        glm::vec3 t_dir = tangents[i];
        
        // Handle collinearity
        if (abs(glm::dot(t_dir, up)) > 0.99f) up = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::vec3 right = glm::normalize(glm::cross(t_dir, up));
        glm::vec3 up_actual = glm::normalize(glm::cross(right, t_dir));
        
        // Propagate up vector to avoid twisting
        up = up_actual;

        for (int j = 0; j <= radialSegments; ++j) {
            float angle = 2.0f * static_cast<float>(M_PI) * j / radialSegments;
            float cosA = cos(angle);
            float sinA = sin(angle);
            
            // Circle formed on the normal plane
            glm::vec3 normal = right * cosA + up_actual * sinA;
            glm::vec3 position = points[i] + normal * radius;
            
            glm::vec2 texCoords(static_cast<float>(j) / radialSegments, static_cast<float>(i) / curveSegments);
            vertices.push_back({position, normal, texCoords});
        }
    }

    // Indices
    for (int i = 0; i < curveSegments; ++i) {
        for (int j = 0; j < radialSegments; ++j) {
            int row1 = i * (radialSegments + 1);
            int row2 = (i + 1) * (radialSegments + 1);

            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row1 + j + 1);

            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }

    return Mesh(vertices, indices);
}
