#include <iostream>
#include <vector>
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"
#include "geom_utils.hpp"

#define LEAF_SIZE 4

using namespace std;

namespace _462
{

BvhNode::BvhNode(const Mesh *_mesh, int *indices, int start, int end)
{
    mesh = _mesh;

    mid_idx = (start + end) / 2;
    if (mid_idx > 8) {cout << mid_idx << "  ";}

    left = NULL;
    right = NULL;

    if (end - start <= LEAF_SIZE)
    {
        // We are a leaf node
        start_triangle = start;
        end_triangle = end;
        return;
    }

    left = new BvhNode(mesh, indices, start, mid_idx);
    left_bbox = Box(mesh, indices, start, mid_idx);

    right = new BvhNode(mesh, indices, mid_idx, end);
    right_bbox = Box(mesh, indices, mid_idx, end);
}

BvhNode::~BvhNode()
{
    if (left)
        delete left;
    if (right)
        delete right;
}

void BvhNode::print()
{
    cout << "{";
    cout << mid_idx;
    if (!(left == NULL && right == NULL))
    {
        if (left != NULL)
        {
            left->print();
        }
        if (right != NULL)
        {
            right->print();
        }
    }
    cout << "}";
}

bool BvhNode::intersect(Vector3 e, Vector3 ray, float &min_time, size_t &min_index,
        float &min_beta, float &min_gamma)
{
    bool ret = false;

    if (!left && !right)
    {
        unsigned int v0, v1, v2;
        Vector3 p0, p1, p2;

        for (size_t s = start_triangle; s < end_triangle; s++)
        {
            MeshTriangle triangle = mesh->get_triangles()[s];
            v0 = triangle.vertices[0];
            v1 = triangle.vertices[1];
            v2 = triangle.vertices[2];
            p0 = mesh->get_vertices()[v0].position;
            p1 = mesh->get_vertices()[v1].position;
            p2 = mesh->get_vertices()[v2].position;

            if (triangle_intersect(e, ray, p0, p1, p2, min_time, min_gamma, min_beta))
            {
                min_index = s;
                ret = true;
            }
        }
        return ret;
    }

    if (left_bbox.intersect(e, ray))
    {
        bool l_inter = left->intersect(e, ray, min_time, min_index,
                min_beta, min_gamma);
        ret = ret || l_inter;
    }

    if (right_bbox.intersect(e, ray))
    {
        bool r_inter = right->intersect(e, ray, min_time, min_index,
                min_beta, min_gamma);
        ret = ret || r_inter;
    }

    return ret;
}


}
