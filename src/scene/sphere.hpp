#pragma once

#include "scene/scene.hpp"

namespace _462
{

/**
 * A sphere, centered on its position with a certain radius.
 */
class Sphere : public Geometry
{
public:

    real_t radius;
    const Material* material;

    bool intersect_frustum(const Frustum& frustum) const;

    Sphere();
    virtual ~Sphere();
    virtual void render() const;
    virtual void intersect_packet(const Packet& packet, IsectInfo *infos, bool *intersected) const;
    virtual bool intersect_ray(const Ray& ray, IsectInfo& info) const;
    virtual bool shadow_test(const Ray& ray) const;
    virtual void make_bounding_volume();
};

} /* _462 */

