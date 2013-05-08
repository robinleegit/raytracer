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
    bool intersect(Vector3 e, Vector3 r) const;
    Box(const Mesh* mesh, int n, int m);
};

}

#endif
