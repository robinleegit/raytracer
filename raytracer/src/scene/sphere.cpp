#include "scene/sphere.hpp"
#include "application/opengl.hpp"
#include "math.h"

namespace _462
{

#define SPHERE_NUM_LAT 80
#define SPHERE_NUM_LON 100

#define SPHERE_NUM_VERTICES ( ( SPHERE_NUM_LAT + 1 ) * ( SPHERE_NUM_LON + 1 ) )
#define SPHERE_NUM_INDICES ( 6 * SPHERE_NUM_LAT * SPHERE_NUM_LON )
// index of the x,y sphere where x is lat and y is lon
#define SINDEX(x,y) ((x) * (SPHERE_NUM_LON + 1) + (y))
#define VERTEX_SIZE 8
#define TCOORD_OFFSET 0
#define NORMAL_OFFSET 2
#define VERTEX_OFFSET 5

static unsigned int Indices[SPHERE_NUM_INDICES];
static float Vertices[VERTEX_SIZE * SPHERE_NUM_VERTICES];

static void init_sphere()
{
    static bool initialized = false;
    if ( initialized )
        return;

    for ( int i = 0; i <= SPHERE_NUM_LAT; i++ )
    {
        for ( int j = 0; j <= SPHERE_NUM_LON; j++ )
        {
            real_t lat = real_t( i ) / SPHERE_NUM_LAT;
            real_t lon = real_t( j ) / SPHERE_NUM_LON;
            float* vptr = &Vertices[VERTEX_SIZE * SINDEX(i,j)];

            vptr[TCOORD_OFFSET + 0] = lon;
            vptr[TCOORD_OFFSET + 1] = 1-lat;

            lat *= PI;
            lon *= 2 * PI;
            real_t sinlat = sin( lat );

            vptr[NORMAL_OFFSET + 0] = vptr[VERTEX_OFFSET + 0] = sinlat * sin( lon );
            vptr[NORMAL_OFFSET + 1] = vptr[VERTEX_OFFSET + 1] = cos( lat ),
                                      vptr[NORMAL_OFFSET + 2] = vptr[VERTEX_OFFSET + 2] = sinlat * cos( lon );
        }
    }

    for ( int i = 0; i < SPHERE_NUM_LAT; i++ )
    {
        for ( int j = 0; j < SPHERE_NUM_LON; j++ )
        {
            unsigned int* iptr = &Indices[6 * ( SPHERE_NUM_LON * i + j )];

            unsigned int i00 = SINDEX(i,  j  );
            unsigned int i10 = SINDEX(i+1,j  );
            unsigned int i11 = SINDEX(i+1,j+1);
            unsigned int i01 = SINDEX(i,  j+1);

            iptr[0] = i00;
            iptr[1] = i10;
            iptr[2] = i11;
            iptr[3] = i11;
            iptr[4] = i01;
            iptr[5] = i00;
        }
    }

    initialized = true;
}

Sphere::Sphere()
    : radius(0), material(0) {}

Sphere::~Sphere() {}

void Sphere::render() const
{
    // create geometry if we haven't already
    init_sphere();

    if ( material )
        material->set_gl_state();

    // just scale by radius and draw unit sphere
    glPushMatrix();
    glScaled( radius, radius, radius );
    glInterleavedArrays( GL_T2F_N3F_V3F, VERTEX_SIZE * sizeof Vertices[0], Vertices );
    glDrawElements( GL_TRIANGLES, SPHERE_NUM_INDICES, GL_UNSIGNED_INT, Indices );
    glPopMatrix();

    if ( material )
        material->reset_gl_state();
}

bool Sphere::intersect_ray(Vector3 eye, Vector3 ray, intersect_info *info) const
{
    Vector3 instance_eye = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    // spheres are centered at zero in object space
    float discriminant = pow(dot(instance_ray, instance_eye), 2)
                         - dot(instance_ray, instance_ray) * (dot(instance_eye, instance_eye) - pow(radius, 2));

    if (discriminant < 0)
    {
        return false;
    }

    // return lower value of t (ie minus discriminant) if it isn't negative (behind origin)
    float t = (-1.0 * dot(instance_ray, instance_eye) - sqrt(discriminant))
              / dot(instance_ray, instance_ray);

    // if t is negative, try the other t (we might be inside sphere)
    if (t < 0)
    {
        t = (-1.0 * dot(instance_ray, instance_eye) + sqrt(discriminant))
            / dot(instance_ray, instance_ray);
    }

    // return info
    Vector3 normal = (instance_eye + t * instance_ray) / radius;
    info->i_normal = normalize(normal_matrix * normal);
    info->i_ambient = material->ambient;
    info->i_diffuse = material->diffuse;
    info->i_specular = material->specular;
    info->i_refractive = material->refractive_index;
    info->i_time = t;

    // texture
    real_t theta = acos(normal.y);
    real_t phi = atan2(normal.x, normal.z);
    real_t tex_u = fmod(phi / (2 * PI), 1.0);
    real_t tex_v = fmod((PI - theta) / PI, 1.0);
    int width, height;
    material->get_texture_size(&width, &height);
    tex_u *= width;
    tex_v *= height;
    info->i_texture = material->get_texture_pixel(tex_u, tex_v);

    return true;
}

bool Sphere::shadow_test(Vector3 eye, Vector3 ray) const
{
    Vector3 instance_eye = inverse_transform_matrix.transform_point(eye);
    Vector3 instance_ray = inverse_transform_matrix.transform_vector(ray);

    float discriminant = pow(dot(instance_ray, instance_eye), 2)
                         - dot(instance_ray, instance_ray) * (dot(instance_eye, instance_eye) - pow(radius, 2));

    if (discriminant < 0)
    {
        return false;
    }

    float t = (-1.0 * dot(instance_ray, instance_eye) - sqrt(discriminant))
              / dot(instance_ray, instance_ray);

    if (t < 0 || t > 1)
    {
        return false;
    }

    return true;
}

void Sphere::make_bounding_volume()
{
    return;
}

bool Sphere::intersect_frustum(Frustum frustum) const
{
    Frustum instance_frustum;

    // check center/radius against all planes
    for (int i = 0; i < 6; i++)
    {
        instance_frustum.planes[i].point =
            inverse_transform_matrix.transform_point(frustum.planes[i].point);
        Matrix3 N;
        make_normal_matrix(&N, inverse_transform_matrix);
        instance_frustum.planes[i].normal = normalize(N * frustum.planes[i].normal);

        Plane plane = instance_frustum.planes[i];
        Vector3 point = position - plane.point; // position is the sphere's center

        // check if center is outside plane
        if (dot(point, plane.normal) > 0.0)
        {
            // check if distance from center to plane is greater than radius
            // if so, then it doesn't intersect
            float distance = dot(point, plane.normal) / length(plane.normal);

            if (distance > radius)
            {
                return false;
            }
        }
    }

    return true;
}



} /* _462 */

