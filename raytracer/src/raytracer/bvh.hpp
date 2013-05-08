#ifndef __BVH_H__
#define __BVH_H__

#include <vector>
#include "scene/mesh.hpp"
#include "scene/box.hpp"

namespace _462
{

class BvhNode
{
public:
    BvhNode *left, *right;
    Box left_bbox, right_bbox;
    int mid_idx;
    BvhNode(const Mesh* mesh, int *indices, int start, int end);
    ~BvhNode();
    bool intersect(Vector3 e, Vector3 ray, std::vector<int>& winners);
    void print();
};

}

#endif
