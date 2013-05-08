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

    if (end - start == 1)
    {
        left = NULL;
        right = NULL;
        return;
    }

    if (start == mid_idx)
        left = NULL;
    else
        left = new BvhNode(mesh, indices, start, mid_idx);

    if (mid_idx == end)
        right = NULL;
    else
        right = new BvhNode(mesh, indices, mid_idx, end);

    //if (right != NULL || left != NULL)
    //{
        left_bbox = Box(mesh, indices, start, mid_idx);
        right_bbox = Box(mesh, indices, mid_idx, end);
    //}
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

    if (left && left_bbox.intersect(e, ray))
    {
        if (left->intersect(e, ray, winners))
        {
            ret = true;
        }
    }

    if (right && right_bbox.intersect(e, ray))
    {
        if (right->intersect(e, ray, winners))
        {
            ret = true;
        }
    }

    return ret;
}

}
