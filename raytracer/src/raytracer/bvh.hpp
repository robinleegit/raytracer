#ifndef __BVH_H__
#define __BVH_H__

#include <vector>
#include "scene/mesh.hpp"
#include "geom_utils.hpp"
#include "raytracer/ray.hpp"

namespace _462
{

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

        if (vi == vj)
            return i < j;

        return vi < vj;
    }
};

class Box
{
public:
    Box() { }
    Box(const Mesh* mesh, std::vector<int>& indices, int n, int m);
    Vector3 min_corner, max_corner;
    bool intersect_ray(Vector3 eye, Vector3 ray) const;
    Box operator+(const Box& rhs);
    float get_surface_area();
    Vector3 get_centroid();
};

class BvhNode
{
public:
    struct IsectInfo
    {
        float time;
        int index;
        float beta;
        float gamma;
    };
    std::vector<int> *indices;
    const Mesh *mesh;
    BvhNode *left, *right;
    Box left_bbox, right_bbox;
    int start_triangle;
    int end_triangle;
    bool root;

    BvhNode(const Mesh *_mesh, std::vector<int> *_indices, int start, int end);
    ~BvhNode();
    void intersect_packet(const Packet& ray, BvhNode::IsectInfo *info, bool *intersected);
    bool intersect_ray(const Ray& ray, BvhNode::IsectInfo& info);
    bool shadow_test(const Ray& ray);
    void print();
};

}

#endif
