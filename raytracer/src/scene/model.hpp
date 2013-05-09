#ifndef _462_SCENE_MODEL_HPP_
#define _462_SCENE_MODEL_HPP_

#include "scene/scene.hpp"
#include "scene/mesh.hpp"
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

    virtual void render() const;
    virtual bool intersect(Vector3 e, Vector3 ray, struct SceneInfo *info) const;
    virtual bool shadow_test(Vector3 e, Vector3 ray) const;
    virtual void make_bounding_volume();
};


} /* _462 */

#endif /* _462_SCENE_MODEL_HPP_ */

