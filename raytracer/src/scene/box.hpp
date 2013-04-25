#ifndef __BOX_H__
#define __BOX_H__

#include "scene/mesh.hpp"

namespace _462
{

class Box
{
public:
    Box() { }
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
    Box(const Mesh* mesh, std::vector<int>& indices) {
        MeshVertex v;
        MeshTriangle t;

        bool binit = false;
        Vector3 bbox_min, bbox_max;
        for (size_t i = 0; i < indices.size(); i++)
        {
            int idx = indices[i];

            t = mesh->get_triangles()[idx];

            for (size_t j = 0; j < 3; j++) 
            {
                int vidx = t.vertices[j];
                v = mesh->get_vertices()[vidx];

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
        }

        min_corner = bbox_min;
        max_corner = bbox_max;
    }
};

}

#endif
