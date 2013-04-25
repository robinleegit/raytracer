#ifndef _462_SCENE_SPHERE_HPP_
#define _462_SCENE_SPHERE_HPP_

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

    Sphere();
    virtual ~Sphere();
    virtual void render() const;
    virtual bool intersect(Vector3 e, Vector3 ray, struct SceneInfo *info) const;
    virtual bool shadow_test(Vector3 e, Vector3 ray) const;
    virtual void make_bounding_volume();
};

} /* _462 */

#endif /* _462_SCENE_SPHERE_HPP_ */

