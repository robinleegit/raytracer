#include <iostream>
#include <vector>
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"

using namespace std;

namespace _462
{

BvhNode::BvhNode(const Mesh* _mesh, vector<int>& _indices)
{
    cout << "Entered bvh constructor(indices.size()=" << _indices.size() << ")" << endl;
    left = NULL;
    right = NULL;
    mesh = _mesh;

    bbox = Box(_mesh, _indices);
    cout << "min, max = " << bbox.min_corner << ", " << bbox.max_corner << endl;

    cout << "Leaf node" << endl;
    for (int i = 0; i < _indices.size(); i++)
    {
        indices.push_back(_indices[i]);
        cout << "adding index " << i << endl;
    }

    // Make a leaf if there are few enough triangles
    if (_indices.size() >= BvhNode::leaf_size)
    {
        cout << "Not a leaf node" << endl;
        // For now just arbitrarily split into two lists of the same size
        int k = _indices.size() / 2;
        vector<int> left_indices, right_indices;
        for (int i = 0; i < _indices.size(); i++)
        {
            if (i < k)
            {
                cout << "left: " << _indices[i] << endl;
                left_indices.push_back(_indices[i]);
            }
            else
            {
                cout << "right: " << _indices[i] << endl;
                right_indices.push_back(_indices[i]);
            }
        }

        if (left_indices.size() > 0)
        {
            left = new BvhNode(_mesh, left_indices);
        }

        if (right_indices.size() > 0)
        {
            right = new BvhNode(_mesh, right_indices);
        }
    }

    cout << "Constructor done" << endl;
}

BvhNode::~BvhNode()
{
    if (left)
    {
        delete left;
        left = NULL;
    }
    if (right)
    {
        delete right;
        right = NULL;
    }
}

bool BvhNode::intersect(Vector3 e, Vector3 ray, vector<MeshTriangle>& winners)
{
    if (!bbox.intersect(e, ray))
    {
        return false;
    }

    // go deeper if we still have children
    if (left != NULL || right != NULL)
    {
        bool left_isect = false;
        if (left != NULL)
        {
            left_isect = left->intersect(e, ray, winners);
        }

        bool right_isect = false;
        if (right != NULL)
        {
            right_isect = right->intersect(e, ray, winners);
        }

        return left_isect || right_isect;
    }

    // if both are null, we are a leaf, so just return our triangles
    for (int i = 0; i < indices.size(); i++)
    {
        int idx = indices[i];
        winners.push_back(mesh->get_triangles()[idx]);
    }

    return true;
}

}
