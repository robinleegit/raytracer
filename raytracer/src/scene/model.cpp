#ifdef __linux
#include <GL/gl.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#endif

#include "scene/model.hpp"
#include "scene/material.hpp"

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>


namespace _462
{

Model::Model() : mesh( 0 ), material( 0 ) { }
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
    // first check intersection with bounding sphere
    Vector3 instance_e = inverse_transform_matrix.transform_point(e);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float discriminant = pow(dot(instance_ray, instance_e - bound.centroid), 2)
                         - dot(instance_ray, instance_ray) *
                         (dot(instance_e - bound.centroid, instance_e - bound.centroid) - pow(bound.radius, 2));

    if (discriminant < 0)
    {
        return false;
    }

    // if it intersects, loop over all triangles in the mesh and find closest hit, if any
    size_t num_triangles = mesh->num_triangles();
    float min_time = -1.0;
    unsigned int v0, v1, v2;
    float x0, y0, z0, x1, y1, z1, x2, y2, z2;
    float beta = 0.0, gamma = 0.0;
    float min_alpha, min_beta = 0.0, min_gamma = 0.0;
    float t;
    size_t min_index = 0;

    for (size_t s = 0; s < num_triangles; s++)
    {
        v0 = mesh->get_triangles()[s].vertices[0];
        v1 = mesh->get_triangles()[s].vertices[1];
        v2 = mesh->get_triangles()[s].vertices[2];
        x0 = mesh->get_vertices()[v0].position.x;
        y0 = mesh->get_vertices()[v0].position.y;
        z0 = mesh->get_vertices()[v0].position.z;
        x1 = mesh->get_vertices()[v1].position.x;
        y1 = mesh->get_vertices()[v1].position.y;
        z1 = mesh->get_vertices()[v1].position.z;
        x2 = mesh->get_vertices()[v2].position.x;
        y2 = mesh->get_vertices()[v2].position.y;
        z2 = mesh->get_vertices()[v2].position.z;

        float a = x0 - x1;
        float b = y0 - y1;
        float c = z0 - z1;
        float d = x0 - x2;
        float e0 = y0 - y2;
        float f = z0 - z2;
        float g = instance_ray.x;
        float h = instance_ray.y;
        float i = instance_ray.z;
        float j = x0 - instance_e.x;
        float k = y0 - instance_e.y;
        float l = z0 - instance_e.z;
        float ei_minus_hf = e0 * i - h * f;
        float gf_minus_di = g * f - d * i;
        float dh_minus_eg = d * h - e0 * g;
        float ak_minus_jb = a * k - j * b;
        float jc_minus_al = j * c - a * l;
        float bl_minus_kc = b * l - k * c;
        float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
        t = -1.0 * (f * ak_minus_jb + e0 * jc_minus_al + d * bl_minus_kc) / m;

        if (t < 0.0)
        {
            continue;
        }

        gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

        if (gamma < 0.0 || gamma > 1.0)
        {
            continue;
        }

        beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

        if (beta < 0.0 || beta > 1.0 - gamma)
        {
            continue;
        }

        if (t < min_time || min_time == -1.0)
        {
            min_time = t;
            min_index = s;
            min_gamma = gamma;
            min_beta = beta;
        }
    }

    // find the info of minimum if any were hit
    if (min_time > 0)
    {
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

    float discriminant = pow(dot(instance_ray, instance_e - bound.centroid), 2)
                         - dot(instance_ray, instance_ray) *
                         (dot(instance_e - bound.centroid, instance_e - bound.centroid) - pow(bound.radius, 2));

    if (discriminant < 0)
    {
        return false;
    }

    // if it intersects, loop over all triangles in the mesh and find closest hit, if any
    size_t num_triangles = mesh->num_triangles();
    unsigned int v0, v1, v2;
    float x0, y0, z0, x1, y1, z1, x2, y2, z2;
    float beta, gamma;
    float t;

    for (size_t s = 0; s < num_triangles; s++)
    {
        v0 = mesh->get_triangles()[s].vertices[0];
        v1 = mesh->get_triangles()[s].vertices[1];
        v2 = mesh->get_triangles()[s].vertices[2];
        x0 = mesh->get_vertices()[v0].position.x;
        y0 = mesh->get_vertices()[v0].position.y;
        z0 = mesh->get_vertices()[v0].position.z;
        x1 = mesh->get_vertices()[v1].position.x;
        y1 = mesh->get_vertices()[v1].position.y;
        z1 = mesh->get_vertices()[v1].position.z;
        x2 = mesh->get_vertices()[v2].position.x;
        y2 = mesh->get_vertices()[v2].position.y;
        z2 = mesh->get_vertices()[v2].position.z;

        float a = x0 - x1;
        float b = y0 - y1;
        float c = z0 - z1;
        float d = x0 - x2;
        float e0 = y0 - y2;
        float f = z0 - z2;
        float g = instance_ray.x;
        float h = instance_ray.y;
        float i = instance_ray.z;
        float j = x0 - instance_e.x;
        float k = y0 - instance_e.y;
        float l = z0 - instance_e.z;
        float ei_minus_hf = e0 * i - h * f;
        float gf_minus_di = g * f - d * i;
        float dh_minus_eg = d * h - e0 * g;
        float ak_minus_jb = a * k - j * b;
        float jc_minus_al = j * c - a * l;
        float bl_minus_kc = b * l - k * c;
        float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
        t = -1.0 * (f * ak_minus_jb + e0 * jc_minus_al + d * bl_minus_kc) / m;

        if (t < 0.0)
        {
            continue;
        }

        gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

        if (gamma < 0.0 || gamma > 1.0)
        {
            continue;
        }

        beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

        if (beta < 0.0 || beta > 1.0 - gamma)
        {
            continue;
        }

        return true;
    }

    return false;
}

void Model::make_bounding_volume()
{
    // loop through all the triangles in the mesh, calculate bound
    size_t num_vertices = mesh->num_vertices();
    MeshVertex v;
    float sum_x = 0.0;
    float sum_y = 0.0;
    float sum_z = 0.0;
    float avg_x, avg_y, avg_z;
    float xdis, ydis, zdis;
    float distance;
    float max_distance = 0.0;

    for (size_t i = 0; i < num_vertices; i++)
    {
        v = mesh->get_vertices()[i];
        sum_x += v.position.x;
        sum_y += v.position.y;
        sum_z += v.position.z;
    }

    avg_x = sum_x / num_vertices;
    avg_y = sum_y / num_vertices;
    avg_z = sum_z / num_vertices;

    bound.centroid = Vector3(avg_x, avg_y, avg_z);

    for (size_t i = 0; i < num_vertices; i++)
    {
        v = mesh->get_vertices()[i];
        xdis = pow((bound.centroid.x - v.position.x), 2);
        ydis = pow((bound.centroid.y - v.position.y), 2);
        zdis = pow((bound.centroid.z - v.position.z), 2);
        distance = sqrt(xdis + ydis + zdis);

        if (distance > max_distance)
        {
            max_distance = distance;
        }
    }

    bound.radius = max_distance;

    return;
}

} /* _462 */

