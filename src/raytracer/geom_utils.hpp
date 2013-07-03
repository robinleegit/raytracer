#pragma once

#include <cstring>
#include "math/vector.hpp"

namespace _462
{

struct Int2
{
    int x, y;
    Int2() : x(0), y(0) { }
    Int2(int _x, int _y) : x(_x), y(_y) { }
};

enum frustum_sides {FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT};

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
};

bool triangle_ray_intersect(Vector3 eye, Vector3 ray, Vector3 p0, Vector3 p1,
                            Vector3 p2, float &min_time, float &min_gamma,
                            float &min_beta);

bool frustum_box_intersect(Frustum frustum, Vector3 box_min, Vector3 box_max);

}
