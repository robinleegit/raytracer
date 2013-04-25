#include "raytracer.hpp"
#include "scene/scene.hpp"
#include "CycleTimer.hpp"

#include <SDL/SDL_timer.h>
#include <iostream>
#include <math.h>
#include <algorithm>

using namespace std;

namespace _462
{

Raytracer::Raytracer()
    : scene( 0 ), width( 0 ), height( 0 ) { }

Raytracer::~Raytracer() { }

/**
 * Initializes the raytracer for the given scene. Overrides any previous
 * initializations. May be invoked before a previous raytrace completes.
 * @param scene The scene to raytrace.
 * @param width The width of the image being raytraced.
 * @param height The height of the image being raytraced.
 * @return true on success, false on error. The raytrace will abort if
 *  false is returned.
 */
bool Raytracer::initialize(Scene* scene0, size_t width0, size_t height0)
{
    this->scene = scene0;
    this->width = width0;
    this->height = height0;

    current_row = 0;
    size_t num_geometries = scene->num_geometries();

    // invert scene transformation matrices
    for (size_t i = 0; i < num_geometries; i++)
    {
        make_inverse_transformation_matrix(&scene->get_geometries()[i]->inverse_transform_matrix,
                                           scene->get_geometries()[i]->position, scene->get_geometries()[i]->orientation,
                                           scene->get_geometries()[i]->scale);
        make_transformation_matrix(&scene->get_geometries()[i]->transform_matrix,
                                   scene->get_geometries()[i]->position,
                                   scene->get_geometries()[i]->orientation,
                                   scene->get_geometries()[i]->scale);
        make_normal_matrix(&scene->get_geometries()[i]->normal_matrix,
                           scene->get_geometries()[i]->transform_matrix);

        // calculate bounding sphere for models
        scene->get_geometries()[i]->make_bounding_volume();
    }

    return true;
}


/**
 * Performs a raytrace on the given pixel on the current scene.
 * The pixel is relative to the bottom-left corner of the image.
 * @param scene The scene to trace.
 * @param x The x-coordinate of the pixel to trace.
 * @param y The y-coordinate of the pixel to trace.
 * @param width The width of the screen in pixels.
 * @param height The height of the screen in pixels.
 * @param recursions The number of times the function has been called.
 * @param start_e The ray origin
 * @param start_ray The ray direction
 * @param refractive The index of refraction surrounding the ray
 * @param extras Whether to turn extras on
 * @return The color of that pixel in the final image.
 */
Color3 Raytracer::trace_pixel(const Scene* scene, size_t x, size_t y,
                              size_t width, size_t height, int recursions, Vector3 start_e,
                              Vector3 start_ray, float refractive, bool extras)
{
    assert(0 <= x && x < width);
    assert(0 <= y && y < height);

    int max_recursion_depth = 10;
    float eps = 0.0001; // "slop factor"
    Vector3 e;          // origin of our viewing ray
    Vector3 ray;        // viewing ray
    size_t num_geometries = scene->num_geometries();
    bool hit = false;
    struct SceneInfo info; // everything we're calculating from intersection
    float min_time = -1.0;
    Vector3 min_normal = Vector3::Zero;
    Color3 min_ambient = Color3::Black;
    Color3 min_diffuse = Color3::Black;
    Color3 min_specular = Color3::Black;
    Color3 min_texture = Color3::Black;
    real_t min_refractive = 0.0;
    Vector3 intersection_point = Vector3::Zero;

    // if this is the first tracing pass, calculate viewing ray from camera
    if (recursions == 0)
    {
        e = scene->camera.get_position();
        ray = get_viewing_ray(e, x, y, width, height);
    }
    // otherwise use the ray that's passed in
    else
    {
        e = start_e;
        ray = start_ray;
    }

    // run intersection test on every object in scene
    for (size_t i = 0; i < num_geometries; i++)
    {
        // intersect returns true if there's a hit, false if not, and sets
        //  values in info struct
        hit = scene->get_geometries()[i]->intersect(e, ray, &info);

        if (hit && info.i_time > eps)
        {
            // check for new min hit time or if it's the first object hit
            if (info.i_time < min_time || min_time == -1.0)
            {
                min_time = info.i_time;
                min_normal = info.i_normal;
                min_ambient = info.i_ambient;
                min_diffuse = info.i_diffuse;
                min_specular = info.i_specular;
                min_texture = info.i_texture;
                min_refractive = info.i_refractive;
                intersection_point = e + (min_time * ray);
            }
        }
    }

    // found a hit
    if (min_time > 0)
    {
        Color3 direct;
        Color3 diffuse = Color3::Black;
        Color3 ambient = scene->ambient_light * min_ambient;
        float angle = dot(ray, min_normal);
        // compute reflected ray
        Vector3 incident_ray = ray - 2 * angle * min_normal;
        incident_ray = normalize(incident_ray);
        Vector3 reflection_point = intersection_point + eps * incident_ray;

        // no-refraction case
        if (min_refractive == 0.0)
        {
            diffuse = get_diffuse(intersection_point, min_normal, min_diffuse, eps);
            direct = min_texture * (ambient + diffuse);

            // return direct light and reflected light if we have recursions left
            if (recursions >= max_recursion_depth)
            {
                return direct;
            }

            return direct + min_texture * min_specular * trace_pixel(scene,
                    x, y, width, height, recursions + 1, reflection_point,
                    incident_ray, refractive, extras);

        }
        // refraction case
        else
        {
            // return black if no more recursions
            if (recursions >= max_recursion_depth)
            {
                return Color3::Black;
            }

            float c;
            Vector3 transmitted_ray;
            float refract_ratio = refractive / min_refractive;

            // negative dot product between ray and normal indicates entering object
            if (angle < 0.0)
            {
                refract(ray, min_normal, refract_ratio, &transmitted_ray);
                c = dot(-1.0 * ray, min_normal);
            }
            else
            {
                // exiting object
                if (refract(ray, (-1.0 * min_normal), min_refractive, &transmitted_ray))
                {
                    c = dot(transmitted_ray, min_normal);
                }
                // total internal reflection
                else
                {
                    return trace_pixel(scene, x, y, width, height, recursions + 1,
                                       reflection_point, incident_ray, refractive, extras);
                }
            }

            // schlick approximation to fresnel equations
            float R_0 = pow(refract_ratio - 1, 2) / pow(refract_ratio + 1, 2);
            float R = R_0 + (1 - R_0) * pow(1 - c, 5);
            Vector3 refraction_point = intersection_point + eps * transmitted_ray;

            // return reflected and refracted rays
            return R * trace_pixel(scene, x, y, width, height, recursions + 1,
                                   reflection_point, incident_ray, refractive, extras) +
                   (1 - R) * trace_pixel(scene, x, y, width, height, recursions + 1,
                                         refraction_point, transmitted_ray, min_refractive, extras);
        }
    }
    // didn't hit anything - return background color
    else
    {
        return scene->background_color;
    }
}

// calculate direction of initial viewing ray from camera
Vector3 Raytracer::get_viewing_ray(Vector3 e, size_t x, size_t y, size_t width, size_t height)
{
    Vector3 g = scene->camera.get_direction();
    Vector3 c_u = scene->camera.get_up();
    float fov = scene->camera.get_fov_radians();
    Vector3 w = normalize(g);
    Vector3 u = normalize(cross(c_u, w));
    Vector3 v = cross(w, u);
    u = -1.0 * u;
    // shirley uses the near plane for the below calculation; we'll just use 1
    real_t t = tan((width / height) * fov / 2.0);
    real_t r = (t * width) / height;
    real_t b = -1.0 * t;
    real_t l = -1.0 * r;
    real_t u_s = l + (r - l) * (x + 0.5) / width;
    real_t v_s = b + (t - b) * (y + 0.5) / height;
    Vector3 s_minus_e = u_s * u + v_s * v + w;

    return normalize(s_minus_e);
}

// calculate contribution of all lights to diffuse light
Color3 Raytracer::get_diffuse(Vector3 intersection_point, Vector3 min_normal,
                              Color3 min_diffuse, float eps)
{
    size_t num_geometries = scene->num_geometries();
    size_t num_lights = scene->num_lights();
    Vector3 light_direction;
    Vector3 light_direction_norm;
    real_t light_distance;
    float front_face;
    bool in_shadow = false;
    Color3 light_color;
    real_t light_attenuation;
    Color3 attenuated_color;
    Color3 diffuse = Color3::Black;

    for (size_t j = 0; j < num_lights; j++)
    {
        light_direction = scene->get_lights()[j].position -
                          intersection_point;
        light_distance = length(light_direction);
        light_direction_norm = normalize(light_direction);
        front_face = std::max(dot(min_normal,
                                  light_direction_norm), 0.0);

        // first check if it's front facing the light
        if (front_face > 0)
        {
            // second, check if the light is blocked:
            for (size_t k = 0; k < num_geometries; k++)
            {
                //  send a ray from intersection point to that light
                in_shadow = scene->get_geometries()[k]->shadow_test(intersection_point
                            + (eps * light_direction), light_direction);

                //  if any object blocks the ray, that light contributes 0
                if (in_shadow)
                {
                    break;
                }
            }

            // third, get attenuated color
            if (!in_shadow)
            {
                light_color = scene->get_lights()[j].color;
                light_attenuation =
                    scene->get_lights()[j].attenuation.constant +
                    scene->get_lights()[j].attenuation.linear * light_distance +
                    scene->get_lights()[j].attenuation.quadratic * pow(light_distance, 2);
                attenuated_color = light_color * (1.0 / light_attenuation);
                diffuse += front_face * attenuated_color * min_diffuse;
            }
        }
    }

    return diffuse;
}

// return false if there is total internal reflection
// if there isn't, set the transmitted ray vector and return true
bool Raytracer::refract(Vector3 d, Vector3 normal, float n, Vector3 *t)
{
    float radicand = 1 - pow(n, 2) * (1 - pow(dot(d, normal), 2));

    if (radicand < 0.0)
    {
        return false;
    }

    *t = normalize(n * (d - normal * dot(d, normal)) - normal * sqrt(radicand));

    return true;
}

/**
 * Raytraces some portion of the scene. Should raytrace for about
 * max_time duration and then return, even if the raytrace is not copmlete.
 * The results should be placed in the given buffer.
 * @param buffer The buffer into which to place the color data. It is
 *  32-bit RGBA (4 bytes per pixel), in row-major order.
 * @param max_time, If non-null, the maximum suggested time this
 *  function raytrace before returning, in seconds. If null, the raytrace
 *  should run to completion.
 * @return true if the raytrace is complete, false if there is more
 *  work to be done.
 */
bool Raytracer::raytrace(unsigned char *buffer, real_t* max_time, bool extras)
{
    static const size_t PRINT_INTERVAL = 64;

    // the time in milliseconds that we should stop
    unsigned int end_time = 0;
    Vector3 start_e = Vector3(0.0, 0.0, 0.0);
    Vector3 start_ray = Vector3(0.0, 0.0, 0.0);

    if ( max_time )
    {
        // convert duration to milliseconds
        unsigned int duration = (unsigned int) ( *max_time * 1000 );
        end_time = SDL_GetTicks() + duration;
    }

    // until time is up, run the raytrace. we render an entire row at once
    // for simplicity and efficiency.
    int n = 4;
    Color3 c = Color3::Black;
    Color3 color;
    double r;
    srand((unsigned)time(NULL));

    double start = CycleTimer::currentSeconds();

    for ( current_row = 0; current_row < height; ++current_row )
    {

        if ( current_row % PRINT_INTERVAL == 0 )
        {
            printf( "Raytracing (row %lu)...\n", current_row );
        }

        for ( size_t x = 0; x < width; ++x )
        {
            // trace a pixel

            if (extras)
            {
                // anti-aliasing
                c = Color3::Black;

                for (int p = -1 * (n / 2); p < n / 2; p++)
                {
                    for (int q = -1 * (n / 2); q < n / 2; q++)
                    {
                        r = (double) rand() / (double) RAND_MAX - 0.5;
                        c += trace_pixel(scene, x + (p + r) / n, current_row + (q + r) / n,
                                         width, height, 0, start_e, start_ray, 1.0, true);
                    }
                }

                color = c * (1.0 / pow((float)n, 2));
            }
            else
            {
                color = trace_pixel(scene, x, current_row,
                                    width, height, 0, start_e, start_ray, 1.0, false);
            }
            // write the result to the buffer, always use 1.0 as the alpha
            color.to_array( &buffer[4 * ( current_row * width + x )] );
        }
    }

    cout << "Total time: " << (CycleTimer::currentSeconds()) - start << endl;

    return true;
}


} /* _462 */

