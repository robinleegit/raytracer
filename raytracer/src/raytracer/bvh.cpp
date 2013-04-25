#include "raytracer/bvh.hpp"
#include "scene/model.hpp"

namespace _462
{

BvhNode::~BvhNode() 
{
    if (left)
        delete left;
    if (right)
        delete right;
}

BvhNode BvhNode::create(Model m) 
{
    return BvhNode();
}

bool BvhNode::intersect(Vector3 e, Vector3 ray)
{
    return false;
}

}
