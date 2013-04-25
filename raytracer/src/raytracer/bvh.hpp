#ifndef __BVH_H__
#define __BVH_H__

#include <vector>
#include "scene/mesh.hpp"
#include "scene/box.hpp"

namespace _462 
{

class BvhNode 
{
private:
    Box bbox;
    const Mesh* mesh;
public:
    static const int leaf_size = 4;
    BvhNode(const Mesh* _mesh, std::vector<int>& _indices);
    ~BvhNode();
    BvhNode *left, *right;
    std::vector<int> indices;
    bool intersect(Vector3 e, Vector3 ray, std::vector<MeshTriangle>& winners);
};

}

#endif
