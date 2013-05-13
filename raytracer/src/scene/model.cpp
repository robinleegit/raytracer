#ifdef __linux
#include <GL/gl.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#endif

#include "scene/model.hpp"
#include "raytracer/ray.hpp"
#include "raytracer/bvh.hpp"
#include "raytracer/CycleTimer.hpp"

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

namespace _462
{

Model::Model() : mesh( 0 ), material( 0 )
{
    bvh = NULL;
}
Model::~Model() { }

void Model::render() const
{
    if (!mesh)
        return;
    if (material)
        material->set_gl_state();
    mesh->render();
    if (material)
        material->reset_gl_state();
}

void Model::intersect_packet(const Packet& packet, IsectInfo *infos, bool *intersected) const
{
    if (intersect_frustum(packet.frustum))
    {
        Packet instance_packet;
        BvhNode::IsectInfo temp_info[rays_per_packet];
        bool temp_intersected[rays_per_packet];
        
        for (int i = 0; i < rays_per_packet; i++)
        {
            instance_packet.rays[i].eye = 
                inverse_transform_matrix.transform_point(packet.rays[i].eye);
            instance_packet.rays[i].dir = 
                inverse_transform_matrix.transform_vector(packet.rays[i].dir);
            temp_intersected[i] = true;
        }

        // packetized version that currently doesn't work
        bvh->intersect_packet(instance_packet, temp_info, temp_intersected);

        // TODO make this simd
        for (int i = 0; i < rays_per_packet; i++)
        {
            if (temp_intersected[i] && temp_info[i].time < infos[i].time)
            {
                compute_ray_info(temp_info[i], infos[i]);
                intersected[i] = true;
            }
        }
    }
}

void Model::compute_ray_info(const BvhNode::IsectInfo& bvh_info, IsectInfo& info) const
{
    float min_alpha;
    size_t min_v0 = mesh->get_triangles()[bvh_info.index].vertices[0];
    size_t min_v1 = mesh->get_triangles()[bvh_info.index].vertices[1];
    size_t min_v2 = mesh->get_triangles()[bvh_info.index].vertices[2];
    Vector3 normal0 = mesh->get_vertices()[min_v0].normal;
    Vector3 normal1 = mesh->get_vertices()[min_v1].normal;
    Vector3 normal2 = mesh->get_vertices()[min_v2].normal;
    min_alpha = 1.0 - bvh_info.gamma - bvh_info.beta;
    Vector3 normal = min_alpha * normal0 + bvh_info.beta * normal1 + bvh_info.gamma * normal2;

    info.normal = normalize(normal_matrix * normal);
    info.ambient = material->ambient;
    info.diffuse = material->diffuse;
    info.specular = material->specular;
    info.refractive = material->refractive_index;
    info.time = bvh_info.time;

    // texture
    Vector2 tex_coord = min_alpha * mesh->get_vertices()[min_v0].tex_coord
        + bvh_info.beta * mesh->get_vertices()[min_v1].tex_coord
        + bvh_info.gamma * mesh->get_vertices()[min_v2].tex_coord;
    double dec_x = fmod(tex_coord.x, 1.0);
    double dec_y = fmod(tex_coord.y, 1.0);
    int width, height;
    material->get_texture_size(&width, &height);
    dec_x *= width;
    dec_y *= height;

    info.texture = material->get_texture_pixel(dec_x, dec_y);
}

bool Model::intersect_ray(const Ray& ray, IsectInfo& info) const
{
    // first check intersection with bounding box
    Ray instance_ray;
    instance_ray.eye = inverse_transform_matrix.transform_point(ray.eye);
    instance_ray.dir = inverse_transform_matrix.transform_vector(ray.dir);

    BvhNode::IsectInfo bvh_info;
    if (!bvh->intersect_ray(instance_ray, bvh_info))
    {
        return false;
    }

    if (bvh_info.time < info.time && bvh_info.time > eps)
    {
        compute_ray_info(bvh_info, info);
    }
    else
    {
        return false;
    }

    return true;
}

bool Model::shadow_test(const Ray& ray) const
{
    Ray instance_ray;
    instance_ray.eye = inverse_transform_matrix.transform_point(ray.eye);
    instance_ray.dir = inverse_transform_matrix.transform_vector(ray.dir);

    return bvh->shadow_test(instance_ray);
}

void Model::make_bounding_volume()
{
    if (bvh)
    {
        delete bvh;
    }

    double bvh_create_start = CycleTimer::currentSeconds();

    bvh = new BvhNode(mesh, NULL, 0, 0);

    double done = CycleTimer::currentSeconds();

    cout << "Bvh creation took       " << (done - bvh_create_start) << "s" << endl;
}

bool Model::intersect_frustum(const Frustum& frustum) const
{
    Frustum instance_frustum;

    for (int i = 0; i < 6; i++)
    {
        instance_frustum.planes[i].point =
            inverse_transform_matrix.transform_point(frustum.planes[i].point);
        Matrix3 N;
        make_normal_matrix(&N, inverse_transform_matrix);
        instance_frustum.planes[i].normal = normalize(N * frustum.planes[i].normal);
    }

    if (frustum_box_intersect(instance_frustum, bvh->left_bbox.min_corner,
            bvh->left_bbox.max_corner))
    {
        return true;
    }

    if (frustum_box_intersect(instance_frustum, bvh->right_bbox.min_corner,
            bvh->right_bbox.max_corner))
    {
        return true;
    }

    return false;
}

} /* _462 */

