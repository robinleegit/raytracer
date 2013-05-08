#include <iostream>
#include <vector>
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"

using namespace std;

namespace _462
{

BvhNode::BvhNode(const Mesh *mesh, int *indices, int start, int end)
{
    mid_idx = (start + end) / 2;

    left = NULL;
    right = NULL;

    if (end - start == 1)
    {
        // We are a leaf node, so don't need to do anything else
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

bool BvhNode::intersect(Vector3 e, Vector3 ray, std::vector<int>& winners)
{
    bool ret = false;

    if (!left && !right)
    {
        winners.push_back(mid_idx);
        return true;
    }

    if (left_bbox.intersect(e, ray))
    {
        ret = ret || left->intersect(e, ray, winners);
    }

    if (right_bbox.intersect(e, ray))
    {
        ret = ret || right->intersect(e, ray, winners);
    }

    return ret;
}

}
