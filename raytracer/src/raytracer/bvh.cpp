#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <limits>
#include "raytracer/CycleTimer.hpp"
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"

#define CHECK_HEAP 0
#define VERBOSE 1
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

void check_heap(const Mesh* mesh, vector<int> indices, int start, int end, int axis)
{
#if CHECK_HEAP
    ostringstream oss_idx, oss_val, oss_err;
    bool ok = true;
    for (int j = start + 1; j < end; j++)
    {
        float tv1 = mesh->get_triangle_centroid(indices[j - 1])[axis];
        float tv2 = mesh->get_triangle_centroid(indices[j])[axis];

        oss_val << tv1 << " ";
        oss_idx << (j - 1) << " ";

        bool local_ok;
        if (tv1 == tv2)
        {
            local_ok = (indices[j - 1] < indices[j]);
            ok = ok && local_ok;

            if (!local_ok) 
            {
                oss_err << (j-1) << ", " << j << " violate the ordering" 
                        << "(" << tv1 << ", " << tv2 << ")" << endl;
            }
        }
        else
        {
            local_ok = (tv1 < tv2);
            ok = ok && local_ok;

            if (!local_ok)
            {
                oss_err << (j-1) << ", " << j << " violate the ordering"
                        << "(" << tv1 << ", " << tv2 << ")" << endl;
            }
        }

    }

    oss_val << endl;
    oss_idx << endl;

    if (!ok)
    {
        cout << oss_val.str() << oss_idx.str() << oss_err.str();
        cout << "start = " << start << endl;
        cout << "end   = " << end   << endl;
        cout << "axis  = " << axis  << endl;
        assert(false);
    }
#endif
}

bool check_adjacent_elems(triangle_less& tl, int i, int j)
{
    if (!tl(i, j))
    {
        cout << "axis    = " << i << ", i, j" << i << ", " << j << endl
             << "i index = " << i << endl
             << "i val   = " << tl.mesh->get_triangle_centroid(i) << endl
             << "j index = " << j << endl
             << "j val   = " << tl.mesh->get_triangle_centroid(j)<< endl;

        return false;
    }

    return true;
}

void triangle_printer(const Mesh* mesh, vector<int> indices[3], int mid_idx, 
        int start, int end, int axis, int non_split_axis)
{
    cout << "axis = " << non_split_axis;

    if (non_split_axis == axis)
        cout << "*: ";
    else
        cout << " : ";

    for (int j = start; j < end; j++)
    {
        cout.width(2);
        cout << indices[non_split_axis][j];

        if (j == mid_idx-1)
            cout << "|";
        else
            cout << " ";
    }
    cout << endl;

    cout << "axis = " << non_split_axis;

    if (non_split_axis == axis)
        cout << "*: ";
    else
        cout << " : ";

    for (int j = start; j < end; j++)
    {
        Vector3 centroid = mesh->get_triangle_centroid(indices[non_split_axis][j]);
        cout.width(2);
        cout << centroid;

        if (j == mid_idx-1)
            cout << "|";
        else
            cout << " ";
    }
    cout << endl;
}

BvhNode::BvhNode(const Mesh *_mesh, vector<int> *_indices, int start, 
        int end, int recursion_depth, ostringstream& parent_poss)
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
            check_heap(mesh, indices[i], 0, indices[i].size(), i);

            /*for (int j = 0; j < num_triangles; j++)
            {
                cout << indices[i][j] << " ";
            }
            cout << endl; */
        }


        double done = CycleTimer::currentSeconds();
        cout << "Creating BVH for model of " << num_triangles << " triangles" << endl;
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
    int mid_tri_id, len = end - start, axis;
    float mid_val;
    float mincost = numeric_limits<float>::max();

    vector<Box> left_boxes(len);
    vector<Box> right_boxes(len);
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
        for (int j = 1; j < len; j++)
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
    ///////////////////////////////////

    bool should_print = false;
    ostringstream oss;
    oss << "Expected left partition size = " << (mid_idx - start) << endl;
    int counts[3], zero_counts[3];
    triangle_less tl(mesh, axis);
    for (int i = 0; i < 3; i++)
    {
        counts[i] = 0;
        zero_counts[i] = 0;
        for (int j = start; j < end; j++)
        {
            if (indices[i][j] == 0)
                zero_counts[i]++;

            float tri_val = mesh->get_triangle_centroid(indices[i][j])[axis];
            if (tri_val == mid_val)
            {
                if (indices[i][j] < mid_tri_id)
                {
                    counts[i]++;
                }
            }
            else if (tri_val < mid_val)
            {
                counts[i]++;
            }
        }

        if (counts[i] != mid_idx - start)
            should_print = true;

        if (zero_counts[i] > 1)
            should_print = true;

        oss << "Axis " << i; 

        if (i == axis)
            oss << "*";
        else
            oss << " ";

        oss << " count = " << counts[i] << endl;
    }

    if (should_print)
    {
        cout << oss.str();

        for (int i = 0; i < 3; i++)
        {
            triangle_printer(mesh, indices, mid_idx, start, end, axis, i);
        }
    }

    ostringstream poss;

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
                bool is_less;

                // Check which partition this triangle goes in. First check
                // based on the axis we are using to partition, and if some
                // triangles have the same value on that axis, use triangle
                // id to do the tire breaking
                if (tri_val != mid_val)
                {
                    is_less = (tri_val < mid_val);
                }
                else
                {
                    is_less = (indices[i][j] < mid_tri_id);
                }

#if VERBOSE
                poss << "p1            = " << p1 << endl
                     << "p2            = " << p2 << endl 
                     << "end - start   = " << end - start << endl
                     << "indices[" << i << "][" << j << "] = " << indices[i][j] << endl
                     << "mid_idx       = " << mid_idx       << endl
                     << "mid_tri_id    = " << mid_tri_id    << endl
                     << "tri_val       = " << tri_val       << mesh->get_triangle_centroid(indices[i][j]) << endl
                     << "mid_val       = " << mid_val       << mesh->get_triangle_centroid(mid_tri_id) << endl
                     << "is_less       = " << is_less       << endl
                     << endl;
#endif

                if (is_less)
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

            check_heap(mesh, indices[i], start, mid_idx, i);
            check_heap(mesh, indices[i], mid_idx, end, i);

            for (int j = start; j < end; j++)
            {
                indices[i][j] = tmp[j - start];
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        triangle_less tl(mesh, i);
        for (int j = start+1; j < mid_idx; j++)
        {
            if (!check_adjacent_elems(tl, indices[i][j-1], indices[i][j]))
            {
                cout << "stopped checking left half" << endl;
                cout << "mid_idx = " << mid_idx << endl;

                cout << "start, end = " << start << ", " << end << endl;
                cout << parent_poss.str();

                throw;
            }
        }
        for (int j = mid_idx+1; j < end; j++)
        {
            if (!check_adjacent_elems(tl, indices[i][j-1], indices[i][j]))
            {
                cout << "stopped checking right half" << endl;
                cout << "mid_idx = " << mid_idx << endl;

                cout << "start, end = " << start << ", " << end << endl;
                cout << parent_poss.str();

                throw;
            }
        }
    }

    /*
    if (recursion_counter >= 100)
    {
        cout << "Recursion stopped at " << recursion_counter << endl;
        return;
    }
    */

    cout << "Recursion depth " << recursion_depth << ". start, end = " << start << ", " << end << endl;
    left = new BvhNode(mesh, indices, start, mid_idx, recursion_depth + 1, poss);
    right = new BvhNode(mesh, indices, mid_idx, end, recursion_depth + 1, poss);
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
    else
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

bool BvhNode::intersect_ray(Vector3 eye, Vector3 ray, float &min_time, size_t &min_index,
                            float &min_beta, float &min_gamma)
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

            if (triangle_ray_intersect(eye, ray, p0, p1, p2, min_time,
                                       min_gamma, min_beta))
            {
                min_index = indices[0][s];
                ret = true;
            }
        }

        return ret;
    }

    if (left_bbox.intersect_ray(eye, ray))
    {
        bool l_inter = left->intersect_ray(eye, ray, min_time, min_index,
                                           min_beta, min_gamma);
        ret = ret || l_inter;
    }

    if (right_bbox.intersect_ray(eye, ray))
    {
        bool r_inter = right->intersect_ray(eye, ray, min_time, min_index,
                                            min_beta, min_gamma);
        ret = ret || r_inter;
    }

    return ret;
}

// this test will exit early if any triangle is hit
bool BvhNode::shadow_test(Vector3 eye, Vector3 ray, float &min_time, size_t &min_index,
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

            if (triangle_ray_intersect(eye, ray, p0, p1, p2, min_time,
                                       min_gamma, min_beta))
            {
                return true;
            }
        }

        return false;
    }

    if (left_bbox.intersect_ray(eye, ray))
    {
        bool l_inter = left->intersect_ray(eye, ray, min_time, min_index,
                                           min_beta, min_gamma);
        ret = ret || l_inter;

    }

    // no need to check the right side if left intersected
    if (ret)
    {
        return true;
    }

    if (right_bbox.intersect_ray(eye, ray))
    {
        bool r_inter = right->intersect_ray(eye, ray, min_time, min_index,
                                            min_beta, min_gamma);
        ret = ret || r_inter;
    }

    return ret;
}


}
