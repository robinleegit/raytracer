#ifndef _462_SCENE_MODEL_HPP_
#define _462_SCENE_MODEL_HPP_

#include "scene/geometry.hpp"
#include "scene/mesh.hpp"
#include "scene/material.hpp"
#include "raytracer/bvh.hpp"

namespace _462
{

/**
 * A mesh of triangles.
 */
class Model : public Geometry
{
public:

    const Mesh* mesh;
    const Material* material;

    BvhNode* bvh;

    Model();
    virtual ~Model();

    void compute_ray_info(const BvhNode::IsectInfo& bvh_info, IsectInfo& info) const;
    bool intersect_frustum(const Frustum& frustum) const;

    virtual void render() const;
    virtual void intersect_packet(const Packet& packet, IsectInfo *infos, bool *intersected) const;
    virtual bool intersect_ray(const Ray& ray, IsectInfo& info) const;
    virtual bool shadow_test(const Ray& ray) const;
    virtual void make_bounding_volume();
};


} /* _462 */

#endif /* _462_SCENE_MODEL_HPP_ */

