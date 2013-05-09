#ifndef __GEOM_UTILS_H__
#define __GEOM_UTILS_H__

#include "math/vector.hpp"
#include <cstring>

namespace _462
{
bool triangle_ray_intersect(Vector3 e, Vector3 ray, Vector3 p0, Vector3 p1,
                        Vector3 p2, float &min_time, float &min_gamma,
                        float &min_beta);
}

#endif
