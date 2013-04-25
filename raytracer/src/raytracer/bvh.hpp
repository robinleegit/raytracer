#ifndef __BVH_H__
#define __BVH_H__

#include "scene/model.hpp"
#include "scene/box.hpp"

namespace _462 
{

class BvhNode 
{
public:
    static BvhNode create(Model m);
    ~BvhNode();
    BvhNode *left, *right;
    Box bbox;
    bool intersect(Vector3 e, Vector3 ray);
};

}

#endif
