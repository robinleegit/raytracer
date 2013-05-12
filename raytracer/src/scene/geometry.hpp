#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "math/quaternion.hpp"
#include "math/matrix.hpp"
#include "math/vector.hpp"
#include "math/color.hpp"
#include "raytracer/ray.hpp"

namespace _462
{

class Geometry
{
public:

    Geometry();
    virtual ~Geometry();

    /*
       World transformation are applied in the following order:
       1. Scale
       2. Orientation
       3. Position
    */

    // The world position of the object.
    Vector3 position;

    // The world orientation of the object.
    // Use Quaternion::to_matrix to get the rotation matrix.
    Quaternion orientation;

    // The world scale of the object.
    Vector3 scale;

    // transformation matrices to precalculate in intialization
    Matrix4 inverse_transform_matrix;
    Matrix4 transform_matrix;
    Matrix3 normal_matrix;

    /**
     * Renders this geometry using OpenGL in the local coordinate space.
     */
    virtual void render() const = 0;
    virtual void make_bounding_volume() = 0;
    virtual bool shadow_test(const Ray& ray) const = 0;
    virtual void intersect_packet(const Packet& packet, IsectInfo *infos, bool *intersected) const = 0;
    virtual bool intersect_ray(const Ray& ray, IsectInfo& info) const = 0;
};

}

#endif
