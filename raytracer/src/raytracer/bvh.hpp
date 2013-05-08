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
    const Mesh *mesh;
    BvhNode *left, *right;
    Box left_bbox, right_bbox;
    int mid_idx;
    int start_triangle;
    int end_triangle;

    BvhNode(const Mesh* _mesh, int *indices, int start, int end);
    ~BvhNode();
    bool intersect(Vector3 e, Vector3 ray, float &min_time, size_t &min_index,
            float &min_beta, float &min_gamma);
    void print();
};

}

#endif
