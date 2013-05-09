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
    Vector3 instance_e = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float a = vertices[0].position.x - vertices[1].position.x;
    float b = vertices[0].position.y - vertices[1].position.y;
    float c = vertices[0].position.z - vertices[1].position.z;
    float d = vertices[0].position.x - vertices[2].position.x;
    float e = vertices[0].position.y - vertices[2].position.y;
    float f = vertices[0].position.z - vertices[2].position.z;
    float g = instance_ray.x;
    float h = instance_ray.y;
    float i = instance_ray.z;
    float j = vertices[0].position.x - instance_e.x;
    float k = vertices[0].position.y - instance_e.y;
    float l = vertices[0].position.z - instance_e.z;
    float ei_minus_hf = e * i - h * f;
    float gf_minus_di = g * f - d * i;
    float dh_minus_eg = d * h - e * g;
    float ak_minus_jb = a * k - j * b;
    float jc_minus_al = j * c - a * l;
    float bl_minus_kc = b * l - k * c;
    float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
    float t = -1.0 * (f * ak_minus_jb + e * jc_minus_al + d * bl_minus_kc) / m;

    if (t < 0.0)
    {
        return false;
    }

    float gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

    if (gamma < 0.0 || gamma > 1.0)
    {
        return false;
    }

    float beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

    if (beta < 0.0 || beta > 1.0 - gamma)
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
    Vector3 instance_e = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float a = vertices[0].position.x - vertices[1].position.x;
    float b = vertices[0].position.y - vertices[1].position.y;
    float c = vertices[0].position.z - vertices[1].position.z;
    float d = vertices[0].position.x - vertices[2].position.x;
    float e = vertices[0].position.y - vertices[2].position.y;
    float f = vertices[0].position.z - vertices[2].position.z;
    float g = instance_ray.x;
    float h = instance_ray.y;
    float i = instance_ray.z;
    float j = vertices[0].position.x - instance_e.x;
    float k = vertices[0].position.y - instance_e.y;
    float l = vertices[0].position.z - instance_e.z;
    float ei_minus_hf = e * i - h * f;
    float gf_minus_di = g * f - d * i;
    float dh_minus_eg = d * h - e * g;
    float ak_minus_jb = a * k - j * b;
    float jc_minus_al = j * c - a * l;
    float bl_minus_kc = b * l - k * c;
    float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
    float t = -1.0 * (f * ak_minus_jb + e * jc_minus_al + d * bl_minus_kc) / m;

    if (t < 0 || t > 1)
    {
        return false;
    }

    float gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

    if (gamma < 0 || gamma > 1)
    {
        return false;
    }

    float beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

    if (beta < 0 || beta > 1 - gamma)
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
    // don't care right now
    return true;
}

} /* _462 */

