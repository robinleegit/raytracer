#ifndef _462_SCENE_TRIANGLE_HPP_
#define _462_SCENE_TRIANGLE_HPP_ 
#include "scene/scene.hpp"

namespace _462
{

/**
 * a triangle geometry.
 * Triangles consist of 3 vertices. Each vertex has its own position, normal,
 * texture coordinate, and material. These should all be interpolated to get
 * values in the middle of the triangle.
 * These values are all in local space, so it must still be transformed by
 * the Geometry's position, orientation, and scale.
 */
class Triangle : public Geometry
{
public:

    struct Vertex
    {
        // note that position and normal are in local space
        Vector3 position;
        Vector3 normal;
        Vector2 tex_coord;
        const Material* material;
    };

    // the triangle's vertices, in CCW order
    Vertex vertices[3];

    Triangle();
    virtual ~Triangle();
    virtual void render() const;
    virtual bool intersect_ray(Vector3& eye, Vector3& ray, intersect_info *info) const;
    virtual bool shadow_test(Vector3& eye, Vector3& ray) const;
    virtual void make_bounding_volume();
    bool intersect_frustum(Frustum& frustum) const;
    virtual void intersect_packet(Packet& packet) const;
};


} /* _462 */

#endif /* _462_SCENE_TRIANGLE_HPP_ */

