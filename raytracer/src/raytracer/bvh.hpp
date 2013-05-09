#ifndef __BVH_H__
#define __BVH_H__

#include <vector>
#include "scene/mesh.hpp"
#include "geom_utils.hpp"

namespace _462
{

struct partition_tester
{
    const Mesh* mesh;
    size_t part_axis;
    float mid_val;

    partition_tester(const Mesh* _mesh, size_t _part_axis, float _mid_val) 
        : mesh(_mesh), part_axis(_part_axis), mid_val(_mid_val) { }

    bool operator()(int i)
    {
        return mesh->get_triangle_centroid(i)[part_axis] < mid_val;
    }
};

struct triangle_less
{
    const Mesh* mesh;
    size_t axis;
    triangle_less(const Mesh* _mesh, size_t _axis) : mesh(_mesh), axis(_axis) { }

    bool operator()(int i, int j)
    {
        float vi, vj;
        vi = mesh->get_triangle_centroid(i)[axis];
        vj = mesh->get_triangle_centroid(j)[axis];
        return vi < vj;
    }
};

class Box
{
public:
    Box() { }
    Box(const Mesh* mesh, std::vector<int>& indices, int n, int m);
    Vector3 min_corner, max_corner;
    bool intersect(Vector3 e, Vector3 r) const;
};

class BvhNode
{
public:
    std::vector<int> *indices;
    const Mesh *mesh;
    BvhNode *left, *right;
    Box left_bbox, right_bbox;
    int mid_idx;
    int start_triangle;
    int end_triangle;
    int axis;
    bool root;

    BvhNode(const Mesh *_mesh, std::vector<int> *_indices, int start, int end, int _axis);
    ~BvhNode();
    bool intersect(Vector3 e, Vector3 ray, float &min_time, size_t &min_index,
                   float &min_beta, float &min_gamma);
    void print();
};

}

#endif
