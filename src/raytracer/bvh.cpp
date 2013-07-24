#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include "raytracer/CycleTimer.hpp"
#include "raytracer/bvh.hpp"
#include "scene/model.hpp"
#include "raytracer/geom_utils.hpp"

#define ISPC
#undef ISPC

#define VERBOSE
#undef VERBOSE

#ifdef ISPC
#include "raytracer/utils.h"
#endif

#define STEP_SIZE 10
#define LEAF_SIZE 8

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

    for (int i = n; i < m; i++)
    {
        MeshTriangle t = mesh->get_triangles()[indices[i]];

        // Iterate over the 3 vertices in the triangle
        for (int j = 0; j < 3; j++)
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
#ifdef ISPC
            ispc::set_indices((int *)&indices[i][0], num_triangles);
#else
            for (int j = 0; j < num_triangles; j++)
            {
                indices[i][j] = j;
            }
#endif
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

    left_node = NULL;
    right_node = NULL;

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

    left_node = new BvhNode(mesh, indices, start, mid_idx);
    right_node = new BvhNode(mesh, indices, mid_idx, end);
}

BvhNode::~BvhNode()
{
    if (left_node)
        delete left_node;
    if (right_node)
        delete right_node;

    if (indices && root)
        delete [] indices;
}

void BvhNode::print()
{
    cout << "{";
    if (!left_node && !right_node)
    {
        for (int i = start_triangle; i < end_triangle; i++)
        {
            cout << indices[0][i];

            if (i + 1 != end_triangle)
                cout << " ";
        }
    }
    if (!(left_node == NULL && right_node == NULL))
    {
        if (left_node != NULL)
        {
            left_node->print();
        }
        if (right_node != NULL)
        {
            right_node->print();
        }
    }
    cout << "}";
}


///////////////////////////////

void BvhNode::intersect_packet(const Packet& packet, BvhNode::IsectInfo *info, bool *intersected)
{
    // leaf node
    if (!left_node && !right_node)
    {
#ifdef ISPC
        intersect_leaf_simd(packet, info, intersected);
#else
        for (int i = 0; i < rays_per_packet; i++)
        {
            if (intersected[i])
            {
                intersected[i] = intersect_leaf(
                        packet.rays[i].eye,
                        packet.rays[i].dir,
                        info[i].time,
                        info[i].index,
                        info[i].beta,
                        info[i].gamma);
            }
        }
#endif

        return;
    }

    // left node
    bool left_active[rays_per_packet];
    bool any_active_left = false;

    for (int i = 0; i < rays_per_packet; i++)
    {
        left_active[i] = false;

        if (intersected[i])
        {
            left_active[i] = 
                left_bbox.intersect_ray(packet.rays[i].eye, packet.rays[i].dir);

            if (left_active[i])
            {
                any_active_left = true;
            }
        }
    }

    if (any_active_left)
    {
        left_node->intersect_packet(packet, info, left_active);
    }

    // right node
    bool right_active[rays_per_packet];
    bool any_active_right = false;

    for (int i = 0; i < rays_per_packet; i++)
    {
        right_active[i] = false;

        if (intersected[i])
        {
            right_active[i] =
                right_bbox.intersect_ray(packet.rays[i].eye, packet.rays[i].dir);

            if (right_active[i])
            {
                any_active_right = true;
            }
        }
    }

    if (any_active_right)
    {
        right_node->intersect_packet(packet, info, right_active);
    }

    for (int i = 0; i < rays_per_packet; i++)
    {
        intersected[i] = left_active[i] || right_active[i];
    }
}

bool BvhNode::intersect_leaf(const Vector3& eye, const Vector3& ray,
                             float& min_time, size_t& min_index,
                             float& min_beta, float& min_gamma)
{
    bool ret = false;

    //TODO SIMD
    for (int s = start_triangle; s < end_triangle; s++)
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

#ifdef ISPC
inline void to_ispc(const Vector3& v, float *ret)
{
    for (int i = 0; i < 3; i++)
    {
        ret[i] = v[i];
    }
}

inline void from_ispc(const Vector3& v, float *ret)
{
    for (int i = 0; i < 3; i++)
    {
        ret[i] = v[i];
    }
}

inline void to_ispc(const Ray& ray, ispc::Ray& ret)
{
    for (int i = 0; i < 3; i++)
    {
        ret.eye[i] = ray.eye[i];
        ret.dir[i] = ray.dir[i];
    }
}

inline void from_ispc(const ispc::Ray& ray, Ray& ret)
{
    for (int i = 0; i < 3; i++)
    {
        ret.eye[i] = ray.eye[i];
        ret.dir[i] = ray.dir[i];
    }
}

inline void to_ispc(const BvhNode::IsectInfo& info, ispc::IsectInfo& ret)
{
    ret.time = info.time;
    ret.gamma = info.gamma;
    ret.beta = info.beta;
}

inline void from_ispc(const ispc::IsectInfo& info, BvhNode::IsectInfo& ret)
{
    ret.time = info.time;
    ret.gamma = info.gamma;
    ret.beta = info.beta;
}

void BvhNode::intersect_leaf_simd(const Packet& packet, BvhNode::IsectInfo *infos, bool *intersected)
{
    for (size_t s = start_triangle; s < end_triangle; s++)
    {
        unsigned int v0, v1, v2;
        ispc::Ray simd_rays[rays_per_packet];
        ispc::IsectInfo simd_infos[rays_per_packet];
        float p0[3], p1[3], p2[3];

        MeshTriangle triangle = mesh->get_triangles()[indices[0][s]];
        v0 = triangle.vertices[0];
        v1 = triangle.vertices[1];
        v2 = triangle.vertices[2];

        to_ispc(mesh->get_vertices()[v0].position, p0);
        to_ispc(mesh->get_vertices()[v1].position, p1);
        to_ispc(mesh->get_vertices()[v2].position, p2);

        for (int i = 0; i < rays_per_packet; i++)
        {
            to_ispc(packet.rays[i], simd_rays[i]);
            to_ispc(infos[i], simd_infos[i]);
        }

        signed char tmp_isect[rays_per_packet];

        ispc::triangle_packet_intersect(simd_rays, simd_infos, (signed char *)tmp_isect, rays_per_packet, p0, p1, p2);

        for (int i = 0; i < rays_per_packet; i++)
        {
            if (tmp_isect[i])
            {
#ifdef VERBOSE
                cout << "simd_info: " 
                     << simd_infos[i].time << ", " 
                     << simd_infos[i].gamma << ", " 
                     << simd_infos[i].beta << endl;
                cout << "info: " 
                     << infos[i].time << ", " 
                     << infos[i].gamma << ", " 
                     << infos[i].beta << endl;
#endif
                from_ispc(simd_infos[i], infos[i]);
                infos[i].index = indices[0][s];
                intersected[i] = true;
            }
        }
    }
}
#endif

/////////////////////////////////////////////


bool BvhNode::intersect_ray(const Ray& ray, BvhNode::IsectInfo& info)
{
    bool ret = false;

    // leaf node case
    if (!left_node && !right_node)
    {
        //TODO SIMD
        for (int s = start_triangle; s < end_triangle; s++)
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
        bool l_inter = left_node->intersect_ray(ray, info);
        ret = ret || l_inter;
    }

    if (right_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool r_inter = right_node->intersect_ray(ray, info);;
        ret = ret || r_inter;
    }

    return ret;
}

// this test will exit early if any triangle is hit
bool BvhNode::shadow_test(const Ray& ray)
{
    bool ret = false;
    BvhNode::IsectInfo info;

    if (!left_node && !right_node)
    {
        for (int s = start_triangle; s < end_triangle; s++)
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
        bool l_inter = left_node->intersect_ray(ray, info);
        ret = ret || l_inter;

    }

    // no need to check the right side if left intersected
    if (ret)
    {
        return true;
    }

    if (right_bbox.intersect_ray(ray.eye, ray.dir))
    {
        bool r_inter = right_node->intersect_ray(ray, info);
        ret = ret || r_inter;
    }

    return ret;
}

}
