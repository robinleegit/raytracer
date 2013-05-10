#ifndef __GEOM_UTILS_H__
#define __GEOM_UTILS_H__

#include "math/vector.hpp"
#include <cstring>

namespace _462
{

enum frustum_sides {FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT};
enum frustum_corners {NLL, NLR, NUL, NUR, FLL, FLR, FUL, FUR};

struct Plane
{
    // a plane can be defined with a point and a normal
    Vector3 point;
    Vector3 normal;
};

struct Frustum
{
    // top, bottom, left, right, front, back planes
    // note that the planes' normals point OUTWARD
    Plane planes[6];
    Vector3 corners[8];
};

bool triangle_ray_intersect(Vector3 eye, Vector3 ray, Vector3 p0, Vector3 p1,
                            Vector3 p2, float &min_time, float &min_gamma,
                            float &min_beta);

bool frustum_box_intersect(Frustum frustum, Vector3 box_min, Vector3 box_max);

}
#endif
