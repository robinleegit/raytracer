#include "scene/triangle.hpp"
#include "application/opengl.hpp"

namespace _462
{

Triangle::Triangle()
{
    vertices[0].material = 0;
    vertices[1].material = 0;
    vertices[2].material = 0;
}

Triangle::~Triangle() { }

void Triangle::render() const
{
    bool materials_nonnull = true;
    for ( int i = 0; i < 3; ++i )
        materials_nonnull = materials_nonnull && vertices[i].material;

    // this doesn't interpolate materials. Ah well.
    if ( materials_nonnull )
        vertices[0].material->set_gl_state();

    glBegin(GL_TRIANGLES);

    glNormal3dv( &vertices[0].normal.x );
    glTexCoord2dv( &vertices[0].tex_coord.x );
    glVertex3dv( &vertices[0].position.x );

    glNormal3dv( &vertices[1].normal.x );
    glTexCoord2dv( &vertices[1].tex_coord.x );
    glVertex3dv( &vertices[1].position.x);

    glNormal3dv( &vertices[2].normal.x );
    glTexCoord2dv( &vertices[2].tex_coord.x );
    glVertex3dv( &vertices[2].position.x);

    glEnd();

    if ( materials_nonnull )
        vertices[0].material->reset_gl_state();
}

bool Triangle::intersect_ray(Vector3 eye, Vector3 ray, intersect_info *info) const
{
    Vector3 instance_eye = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float t = INFINITY, beta = INFINITY, gamma = INFINITY;
    Vector3 p0, p1, p2;
    p0 = vertices[0].position;
    p1 = vertices[1].position;
    p2 = vertices[2].position;

    if (!triangle_ray_intersect(instance_eye, instance_ray, p0, p1, p2,
                t, gamma, beta))
    {
        return false;
    }

    float alpha = 1.0 - gamma - beta;
    info->i_time = t;
    Vector3 normal = alpha * vertices[0].normal
                     + beta * vertices[1].normal
                     + gamma * vertices[2].normal;

    // need to make sure normal is returned in the direction we want
    if (dot(instance_ray, normal) > 0.0)
    {
        normal *= -1.0;
    }

    info->i_normal = normalize(normal_matrix * normal);
    info->i_ambient = alpha * vertices[0].material->ambient
                      + beta * vertices[1].material->ambient
                      + gamma * vertices[2].material->ambient;
    info->i_diffuse = alpha * vertices[0].material->diffuse
                      + beta * vertices[1].material->diffuse
                      + gamma * vertices[2].material->diffuse;
    info->i_specular = alpha * vertices[0].material->specular
                       + beta * vertices[1].material->specular
                       + gamma * vertices[2].material->specular;
    info->i_refractive = alpha * vertices[0].material->refractive_index
                         + beta * vertices[1].material->refractive_index
                         + gamma * vertices[2].material->refractive_index;

    // texture
    Vector2 tex_coord = alpha * vertices[0].tex_coord
                        + beta * vertices[1].tex_coord
                        + gamma * vertices[2].tex_coord;
    double integer_coord;
    float dec_x = modf(tex_coord.x, &integer_coord);
    float dec_y = modf(tex_coord.y, &integer_coord);
    int width0, width1, width2, height0, height1, height2;
    vertices[0].material->get_texture_size(&width0, &height0);
    vertices[1].material->get_texture_size(&width1, &height1);
    vertices[2].material->get_texture_size(&width2, &height2);
    float dec_x0 = dec_x * width0;
    float dec_y0 = dec_y * height0;
    float dec_x1 = dec_x * width1;
    float dec_y1 = dec_y * height1;
    float dec_x2 = dec_x * width2;
    float dec_y2 = dec_y * height2;
    Color3 tc0 = vertices[0].material->get_texture_pixel(dec_x0, dec_y0);
    Color3 tc1 = vertices[1].material->get_texture_pixel(dec_x1, dec_y1);
    Color3 tc2 = vertices[2].material->get_texture_pixel(dec_x2, dec_y2);
    info->i_texture = alpha * tc0 + beta * tc1 + gamma * tc2;

    return true;
}

bool Triangle::shadow_test(Vector3 eye, Vector3 ray) const
{
    Vector3 instance_eye = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float t = INFINITY, beta = INFINITY, gamma = INFINITY;
    Vector3 p0, p1, p2;
    p0 = vertices[0].position;
    p1 = vertices[1].position;
    p2 = vertices[2].position;

    if (!triangle_ray_intersect(instance_eye, instance_ray, p0, p1, p2,
                t, gamma, beta))
    {
        return false;
    }

    return true;
}

void Triangle::make_bounding_volume()
{
    return;
}

bool Triangle::intersect_frustum(Frustum frustum) const
{
    Frustum instance_frustum;

    // check each vertex against all instanced planes
    for (int i = 0; i < 6; i++)
    {
        instance_frustum.planes[i].point =
            inverse_transform_matrix.transform_point(frustum.planes[i].point);
        instance_frustum.planes[i].normal =
            inverse_transform_matrix.transform_vector(frustum.planes[i].normal);

        int outside = 0;

        for (int j = 0; j < 3; j++)
        { 
            Vector3 pos = vertices[j].position;
            Plane plane = instance_frustum.planes[i];

            if (dot(pos - plane.point, plane.normal) < 0.0)
            {
                outside++;
            }
        } 

        if (outside == 3)
        {
            return false;
        }
    }

    return true;
}

} /* _462 */

