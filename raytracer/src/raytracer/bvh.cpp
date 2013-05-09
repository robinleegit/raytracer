#include <iostream>
#include <vector>
#include "raytracer/CycleTimer.hpp"
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"
#include "geom_utils.hpp"

#define LEAF_SIZE 4

using namespace std;

namespace _462
{

bool Box::intersect(Vector3 e, Vector3 r) const
{
    double tmin = -INFINITY, tmax = INFINITY;


    if (r.x != 0.0)
    {
        double tx1 = (min_corner.x - e.x)/r.x;
        double tx2 = (max_corner.x - e.x)/r.x;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

    if (r.y != 0.0)
    {
        double ty1 = (min_corner.y - e.y)/r.y;
        double ty2 = (max_corner.y - e.y)/r.y;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

    if (r.z != 0.0)
    {
        double tz1 = (min_corner.z - e.z)/r.z;
        double tz2 = (max_corner.z - e.z)/r.z;

        tmin = max(tmin, min(tz1, tz2));
        tmax = min(tmax, max(tz1, tz2));
    }

    return tmax >= tmin;
}

Box::Box(const Mesh* mesh, vector<int>& indices, int n, int m)
{
    min_corner.x = INFINITY;
    min_corner.y = INFINITY;
    min_corner.z = INFINITY;

    max_corner.x = -INFINITY;
    max_corner.y = -INFINITY;
    max_corner.z = -INFINITY;

    for (size_t i = n; i < m; i++)
    {
        MeshTriangle t = mesh->get_triangles()[indices[i]];

        for (size_t j = 0; j < 3; j++)
        {
            int vidx = t.vertices[j];
            MeshVertex v = mesh->get_vertices()[vidx];

            min_corner.x = min(min_corner.x, v.position.x);
            max_corner.x = max(max_corner.x, v.position.x);

            min_corner.y = min(min_corner.y, v.position.y);
            max_corner.y = max(max_corner.y, v.position.y);

            min_corner.z = min(min_corner.z, v.position.z);
            max_corner.z = max(max_corner.z, v.position.z);
        }
    }
}

BvhNode::BvhNode(const Mesh *_mesh, vector<int> *_indices, int start, int end, int axis) 
    : indices(_indices), mesh(_mesh), root(false)
{
    // do some setup for the root node
    if (!_indices)
    {
        root = true;
        double bvh_create_start = CycleTimer::currentSeconds();

        int num_triangles = mesh->num_triangles();
        start = 0;
        end = num_triangles;
        axis = 0;

        indices = new vector<int>[3];
        indices[0] = vector<int>(num_triangles);
        indices[1] = vector<int>(num_triangles);
        indices[2] = vector<int>(num_triangles);

        double start = CycleTimer::currentSeconds();

        for (int j = 0; j < num_triangles; j++)
        {
            for (int i = 0; i < 3; i++)
            {
                indices[i][j] = j;
            }
        }

        /*
        for (int i = 0; i < 3; i++)
        {
            triangle_less tl(mesh, i);
            sort(indices[i].begin(), indices[i].end(), tl);
        }
        */

        cout << "Indices creation took " << (CycleTimer::currentSeconds() - start) << "s" << endl
             << "Bvh creation took     " << (CycleTimer::currentSeconds() - bvh_create_start) << "s" << endl;
    }

    mid_idx = (start + end) / 2;

    left = NULL;
    right = NULL;

    if (end - start <= LEAF_SIZE)
    {
        // We are a leaf node
        start_triangle = start;
        end_triangle = end;
        return;
    }

    /*
    // partition
    for (int i = 0; i < 3; i++)
    {
        if (i != axis)
        {
            float mid_val = mesh->get_triangle_centroid(mid_idx)[axis];
            partition_tester pt(mesh, axis, indices[axis][mid_idx], mid_val);

            int p1 = 0, p2 = mid_idx;
            for (int j = start; j < end; j++)
            {
                if (pt(j))
                {
                    swap(indices[i][j], indices[i][p1]);
                    p1++;
                }
                else
                {
                    swap(indices[i][j], indices[i][p2]);
                    p2++;
                }
            }
        }
    }*/

    int newaxis = 0;//(axis + 1) % 3;
    left = new BvhNode(mesh, indices, start, mid_idx, newaxis);
    left_bbox = Box(mesh, indices[newaxis], start, mid_idx);

    right = new BvhNode(mesh, indices, mid_idx, end, newaxis);
    right_bbox = Box(mesh, indices[newaxis], mid_idx, end);
}

BvhNode::~BvhNode()
{
    if (left)
        delete left;
    if (right)
        delete right;

    if (indices && root)
        delete [] indices;
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
        for (size_t s = start_triangle; s < end_triangle; s++)
        {
            unsigned int v0, v1, v2;
            Vector3 p0, p1, p2;

            MeshTriangle triangle = mesh->get_triangles()[indices[0][s]];
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
