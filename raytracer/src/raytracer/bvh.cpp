#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include "raytracer/CycleTimer.hpp"
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"
#include "raytracer/geom_utils.hpp"

#define STEP_SIZE 10
#define LEAF_SIZE 4

using namespace std;

namespace _462
{

Box Box::operator+(const Box& rhs)
{
    Box ret;

    for (int axis = 0; axis < 3; axis++)
    {
        ret.min_corner[axis] = min(this->min_corner[axis], rhs.min_corner[axis]);
        ret.max_corner[axis] = max(this->max_corner[axis], rhs.max_corner[axis]);
    }

    return ret;
}

float Box::get_surface_area()
{
    float ret = 0.0;
    Vector3 diag = max_corner - min_corner;

    ret += diag.x * diag.y;
    ret += diag.y * diag.z;
    ret += diag.z * diag.x;

    return ret * 2;
}

void intersect_packet(const Packet& ray, BvhNode::IsectInfo *info, bool *intersected)
{
}

bool Box::intersect_ray(Vector3 eye, Vector3 ray) const
{
    double tmin = -INFINITY, tmax = INFINITY;


    if (ray.x != 0.0)
    {
        double tx1 = (min_corner.x - eye.x) / ray.x;
        double tx2 = (max_corner.x - eye.x) / ray.x;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

    if (ray.y != 0.0)
    {
        double ty1 = (min_corner.y - eye.y)/ray.y;
        double ty2 = (max_corner.y - eye.y)/ray.y;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

    if (ray.z != 0.0)
    {
        double tz1 = (min_corner.z - eye.z)/ray.z;
        double tz2 = (max_corner.z - eye.z)/ray.z;

        tmin = max(tmin, min(tz1, tz2));
        tmax = min(tmax, max(tz1, tz2));
    }

    return tmax >= tmin;
}

Box::Box(const Mesh* mesh, vector<int>& indices, int n, int m)
{
    for (int axis = 0; axis < 3; axis++)
    {
        min_corner[axis] = INFINITY;
        max_corner[axis] = -INFINITY;
    }

    for (size_t i = n; i < m; i++)
    {
        MeshTriangle t = mesh->get_triangles()[indices[i]];

        // Iterate over the 3 vertices in the triangle
        for (size_t j = 0; j < 3; j++)
        {
            int vidx = t.vertices[j];
            MeshVertex v = mesh->get_vertices()[vidx];

            for (int axis = 0; axis < 3; axis++)
            {
                min_corner[axis] = min(min_corner[axis], v.position[axis]);
                max_corner[axis] = max(max_corner[axis], v.position[axis]);
            }
        }
    }
}

BvhNode::BvhNode(const Mesh *_mesh, vector<int> *_indices, int start, int end)
    : indices(_indices), mesh(_mesh), root(false)
{
    // do some setup for the root node
    if (!_indices)
    {
        root = true;
        double root_create_start = CycleTimer::currentSeconds();

        int num_triangles = mesh->num_triangles();
        start = 0;
        end = num_triangles;

        indices = new vector<int>[3];
        indices[0] = vector<int>(num_triangles);
        indices[1] = vector<int>(num_triangles);
        indices[2] = vector<int>(num_triangles);

        double index_assign_start = CycleTimer::currentSeconds();

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < num_triangles; j++)
            {
                indices[i][j] = j;
            }
        }

        double sort_start = CycleTimer::currentSeconds();

        for (int i = 0; i < 3; i++)
        {
            triangle_less tl(mesh, i);
            sort(indices[i].begin(), indices[i].end(), tl);
        }

        double done = CycleTimer::currentSeconds();
        cout << "Indices creation took   " << (done - index_assign_start) << "s" << endl
             << "Sorting of indices took " << (done - sort_start)         << "s" << endl
             << "Root bvh setup took     " << (done - root_create_start ) << "s" << endl;
    }

    left = NULL;
    right = NULL;

    if (end - start <= LEAF_SIZE)
    {
        // We are a leaf node
        start_triangle = start;
        end_triangle = end;

        return;
    }

    ///////////////////////////////////
    // Do SAH to choose partition
    int mid_idx, mid_tri_id, len = end - start, axis;
    float mid_val;
    float mincost = numeric_limits<float>::max();

    Box *left_boxes = new Box[len];
    Box *right_boxes = new Box[len];

    int step = (end - start) / STEP_SIZE;
    step = step > 0 ? step : 1;

    for (int i = 0; i < 3; i++)
    {
        // Create partial sums of bounding boxes
        left_boxes[0] = Box(mesh, indices[i], start, start + 1);
        right_boxes[len-1] = Box(mesh, indices[i], end - 1, end);
        for (int j = 1; j < len; j++)
        {
            left_boxes[j] = Box(mesh, indices[i], start + j, start + j+1) + left_boxes[j-1];
            right_boxes[len-j-1] = Box(mesh, indices[i], start + (len-j-1), start + (len-j)) + right_boxes[len-j];
        }

        // Actually find the minimum cost partition using the partial sums
        for (int j = 1; j < len; j += step)
        {
            float left_sa = left_boxes[j-1].get_surface_area();
            float right_sa = right_boxes[j].get_surface_area();
            float cost = left_sa * j + right_sa * (len - j);

            if (cost < mincost)
            {
                mincost = cost;
                float val1 = mesh->get_triangle_centroid(indices[i][start + j - 1])[i];
                float val2 = mesh->get_triangle_centroid(indices[i][start + j ])[i];
                mid_idx = start + j;
                mid_val = (val1 + val2) / 2;
                mid_tri_id = indices[i][start + j];
                left_bbox = left_boxes[j];
                right_bbox = right_boxes[j];
                axis = i;
            }
        }
    }

    delete [] right_boxes;
    delete [] left_boxes;
    ///////////////////////////////////

    // partition
    for (int i = 0; i < 3; i++)
    {
        if (i != axis)
        {
            vector<int> tmp(end - start);
            int p1 = 0, p2 = mid_idx - start;
            for (int j = start; j < end; j++)
            {
                float tri_val = mesh->get_triangle_centroid(indices[i][j])[axis];
                bool left_part;

                // Check which partition this triangle goes in. First check
                // based on the axis we are using to partition, and if some
                // triangles have the same value on that axis, use triangle
                // id to do the tire breaking
                if (tri_val != mid_val)
                {
                    left_part = tri_val < mid_val;
                }
                else
                {
                    left_part = indices[i][j] < mid_tri_id;
                }

                if (left_part)
                {
                    tmp[p1] = indices[i][j];
                    p1++;
                }
                else
                {
                    tmp[p2] = indices[i][j];
                    p2++;
                }
            }

            for (int j = start; j < end; j++)
            {
                indices[i][j] = tmp[j - start];
            }
        }
    }

    left = new BvhNode(mesh, indices, start, mid_idx);
    right = new BvhNode(mesh, indices, mid_idx, end);
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
    if (!left && !right)
    {
        for (int i = start_triangle; i < end_triangle; i++)
        {
            cout << indices[0][i];

            if (i + 1 != end_triangle)
                cout << " ";
        }
    }
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

bool BvhNode::intersect_ray(const Ray& ray, BvhNode::IsectInfo& info)
{
    bool ret = false;

    if (!left && !right)
    {
        //TODO SIMD
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

            if (triangle_ray_intersect(ray.eye, ray.dir, p0, p1, p2, info.time,
                                       info.gamma, info.beta))
            {
                info.index = indices[0][s];
                ret = true;
            }
        }

        return ret;
    }

    if (left_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool l_inter = left->intersect_ray(ray, info);
        ret = ret || l_inter;
    }

    if (right_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool r_inter = right->intersect_ray(ray, info);
        ret = ret || r_inter;
    }

    return ret;
}

// this test will exit early if any triangle is hit
bool BvhNode::shadow_test(const Ray& ray)
{
    bool ret = false;
    BvhNode::IsectInfo info;

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

            if (triangle_ray_intersect(ray.eye, ray.dir, p0, p1, p2, info.time,
                                       info.gamma, info.beta))
            {
                return true;
            }
        }

        return false;
    }

    if (left_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool l_inter = left->intersect_ray(ray, info);
        ret = ret || l_inter;

    }

    // no need to check the right side if left intersected
    if (ret)
    {
        return true;
    }

    if (right_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool r_inter = right->intersect_ray(ray, info);
        ret = ret || r_inter;
    }

    return ret;
}

}
