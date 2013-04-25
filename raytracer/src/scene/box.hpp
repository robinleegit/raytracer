#ifndef __BOX_H__
#define __BOX_H__

#include "scene/mesh.hpp"

namespace _462
{

class Box
{
public:
    Box() { }
    Box(Vector3 min, Vector3 max) : min_corner(min), max_corner(max) { }
    Vector3 min_corner, max_corner;
    bool intersect(Vector3 e, Vector3 d) const
    {
        Vector3 tmin, tmax;

        tmin.x = (min_corner.x - e.x) / d.x;
        tmax.x = (max_corner.x - e.x) / d.x;
        if (d.x < 0)
            std::swap(tmin.x, tmax.x);

        tmin.y = (min_corner.y - e.y) / d.y;
        tmax.y = (max_corner.y - e.y) / d.y;
        if (d.y < 0)
            std::swap(tmin.y, tmax.y);

        tmin.z = (min_corner.z - e.z) / d.z;
        tmax.z = (max_corner.z - e.z) / d.z;
        if (d.z < 0)
            std::swap(tmin.z, tmax.z);

        bool xzok, yzok, yxok;
        xzok = !(tmin.z > tmax.x || tmin.x > tmax.z);
        yzok = !(tmin.z > tmax.y || tmin.y > tmax.z);
        yxok = !(tmin.x > tmax.y || tmin.y > tmax.x);

        return xzok && yzok && yxok;
    }
    static Box create(const Mesh* mesh) {
        size_t num_vertices = mesh->num_vertices();
        MeshVertex v;

        bool binit = false;
        Vector3 bbox_min, bbox_max;
        for (size_t i = 0; i < num_vertices; i++)
        {
            v = mesh->get_vertices()[i];

            if (v.position.x > bbox_max.x || !binit)
                bbox_max.x = v.position.x;
            if (v.position.x < bbox_min.x || !binit)
                bbox_min.x = v.position.x;

            if (v.position.y > bbox_max.y || !binit)
                bbox_max.y = v.position.y;
            if (v.position.y < bbox_min.y || !binit)
                bbox_min.y = v.position.y;

            if (v.position.z > bbox_max.z || !binit)
                bbox_max.z = v.position.z;
            if (v.position.z < bbox_min.z || !binit)
                bbox_min.z = v.position.z;

            binit = true;
        }

        return Box(bbox_min, bbox_max);
    }
};

}

#endif
