#ifdef __linux
#include <GL/gl.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#endif

#include "scene/model.hpp"
#include "scene/material.hpp"
#include "raytracer/bvh.hpp"

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

bool Model::intersect(Vector3 e, Vector3 ray, struct SceneInfo *info) const
{
    // first check intersection with bounding box
    Vector3 instance_e = inverse_transform_matrix.transform_point(e);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float min_time = INFINITY, min_beta = INFINITY, min_gamma = INFINITY;
    size_t min_index = 0;

    if (!bvh->intersect(instance_e, instance_ray, min_time, min_index,
                min_beta, min_gamma))
    {
        return false;
    }


    // find the info of minimum if any were hit
    if (min_time > 0)
    {
        float min_alpha;
        size_t min_v0 = mesh->get_triangles()[min_index].vertices[0];
        size_t min_v1 = mesh->get_triangles()[min_index].vertices[1];
        size_t min_v2 = mesh->get_triangles()[min_index].vertices[2];
        Vector3 normal0 = mesh->get_vertices()[min_v0].normal;
        Vector3 normal1 = mesh->get_vertices()[min_v1].normal;
        Vector3 normal2 = mesh->get_vertices()[min_v2].normal;
        min_alpha = 1.0 - min_gamma - min_beta;
        Vector3 normal = min_alpha * normal0 + min_beta * normal1 + min_gamma * normal2;
        info->i_normal = normalize(normal_matrix * normal);
        info->i_ambient = material->ambient;
        info->i_diffuse = material->diffuse;
        info->i_specular = material->specular;
        info->i_refractive = material->refractive_index;
        info->i_time = min_time;

        // texture
        Vector2 tex_coord = min_alpha * mesh->get_vertices()[min_v0].tex_coord
                            + min_beta * mesh->get_vertices()[min_v1].tex_coord
                            + min_gamma * mesh->get_vertices()[min_v2].tex_coord;
        double dec_x = fmod(tex_coord.x, 1.0);
        double dec_y = fmod(tex_coord.y, 1.0);
        int width, height;
        material->get_texture_size(&width, &height);
        dec_x *= width;
        dec_y *= height;
        info->i_texture = material->get_texture_pixel(dec_x, dec_y);
    }
    // if none hit, return false
    else
    {
        return false;
    }

    return true;
}

bool Model::shadow_test(Vector3 e, Vector3 ray) const
{
    // first check intersection with bounding sphere
    Vector3 instance_e = inverse_transform_matrix.transform_point(e);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float min_time = INFINITY, min_beta = INFINITY, min_gamma = INFINITY;
    size_t min_index = 0;

    return bvh->intersect(instance_e, instance_ray, min_time, min_index,
                min_beta, min_gamma);
}

void Model::make_bounding_volume()
{
    if (bvh)
    {
        delete bvh;
    }

    int num_triangles = mesh->num_triangles();

    cout << "num_triangles " << num_triangles << endl;
    int *indices = new int[num_triangles];

    cout << "Assigning indices" << endl;
    for (int i = 0; i < num_triangles; i++)
    {
        indices[i] = i;
    }

    cout << "Creating BVH node " << endl;
    bvh = new BvhNode(mesh, indices, 0, num_triangles);
    //cout << bvh << endl;
    bvh->print();
    cout << endl;

    cout << "min corner: " << bvh->left->left->left_bbox.min_corner << endl;
    cout << "max corner: " << bvh->left_bbox.max_corner << endl;
    delete indices;
}

} /* _462 */

