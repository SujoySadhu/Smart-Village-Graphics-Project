#ifndef BEZIER_SURFACE_H
#define BEZIER_SURFACE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"

/**
 * BezierSurface class for creating surfaces of revolution from Bezier curves.
 * 
 * ALGORITHM EXPLANATION - Rotational Sweeping of Bezier Curves:
 * 
 * 1. Define a 2D Bezier curve profile (typically in the XY plane)
 *    - Control points define the shape (e.g., dome profile, water tank shape)
 *    - The curve represents the silhouette of the surface
 * 
 * 2. Evaluate points along the Bezier curve
 *    - Use De Casteljau's algorithm or direct Bernstein polynomial evaluation
 *    - Sample at regular parameter intervals (t from 0 to 1)
 * 
 * 3. Revolve the curve around the Y-axis
 *    - For each point on the curve, create a circle of vertices
 *    - Each circle is at the original Y height with radius = original X value
 *    - Rotate around Y-axis at regular angle intervals
 * 
 * 4. Generate mesh
 *    - Connect adjacent circles with triangles
 *    - Calculate normals for proper lighting
 */
class BezierSurface {
public:
    /**
     * Creates a surface of revolution from Bezier curve control points.
     * 
     * @param controlPoints 2D control points (x=radius from axis, y=height)
     * @param curveSegments Number of segments along the Bezier curve
     * @param rotationSegments Number of segments around the rotation axis
     * @return Mesh object with the generated geometry
     */
    static Mesh CreateSurfaceOfRevolution(
        const std::vector<glm::vec2>& controlPoints,
        int curveSegments = 32,
        int rotationSegments = 32
    );

    /**
     * Creates a dome shape using Bezier curve revolution.
     * Ideal for mosque domes or water tank tops.
     */
    static Mesh CreateDome(float radius, float height, int segments = 32);

    /**
     * Creates a water tank shape using Bezier curve revolution.
     * Cylindrical body with curved dome top.
     */
    static Mesh CreateWaterTank(float radius, float bodyHeight, float domeHeight, int segments = 32);

public:
    /**
     * Evaluates a Catmull-Rom Spline point in 3D at parameter t (0.0 to 1.0).
     * p1 and p2 form the segment, p0 and p3 provide tangents.
     */
    static glm::vec3 EvaluateCatmullRomSpline(
        const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3,
        float t
    );

    /**
     * Evaluates a 3D Cubic Bezier curve at parameter t.
     */
    static glm::vec3 EvaluateCubicBezier3D(
        const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3,
        float t
    );

    /**
     * Creates a Ruled Surface by interpolating between two 3D curves.
     */
    static Mesh CreateRuledSurface(
        const std::vector<glm::vec3>& curve1,
        const std::vector<glm::vec3>& curve2,
        int vSegments
    );

    /**
     * Creates a 3D physical tube (extrusion) along a cubic Bezier curve.
     */
    static Mesh CreateCurveTube(
        const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3,
        float radius, int curveSegments, int radialSegments
    );

private:

    /**
     * Evaluates a cubic Bezier curve at parameter t using De Casteljau's algorithm.
     * 
     * The algorithm works by recursively interpolating between control points:
     * 1. Given 4 control points P0, P1, P2, P3
     * 2. Compute intermediate points at parameter t:
     *    Q0 = lerp(P0, P1, t)
     *    Q1 = lerp(P1, P2, t)
     *    Q2 = lerp(P2, P3, t)
     * 3. Then:
     *    R0 = lerp(Q0, Q1, t)
     *    R1 = lerp(Q1, Q2, t)
     * 4. Finally:
     *    Point = lerp(R0, R1, t)
     */
    static glm::vec2 EvaluateCubicBezier(
        const glm::vec2& p0, const glm::vec2& p1,
        const glm::vec2& p2, const glm::vec2& p3,
        float t
    );

    /**
     * Evaluates the tangent of a cubic Bezier curve at parameter t.
     * Used for normal calculation.
     */
    static glm::vec2 EvaluateCubicBezierTangent(
        const glm::vec2& p0, const glm::vec2& p1,
        const glm::vec2& p2, const glm::vec2& p3,
        float t
    );

    /**
     * Evaluates a general Bezier curve with any number of control points.
     * Uses the Bernstein polynomial form.
     */
    static glm::vec2 EvaluateBezier(const std::vector<glm::vec2>& controlPoints, float t);
};

#endif
