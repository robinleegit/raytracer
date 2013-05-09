#include <SDL/SDL_timer.h>
#include <iostream>
#include <math.h>
#include <algorithm>

#include "raytracer.hpp"
#include "CycleTimer.hpp"

#define PACKET_DIM 128

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
bool Raytracer::initialize(Scene* _scene, size_t _width, size_t _height)
{
    this->scene = _scene;
    this->width = _width;
    this->height = _height;

    size_t num_geometries = scene->num_geometries();

    // invert scene transformation matrices
    for (size_t i = 0; i < num_geometries; i++)
    {
        make_inverse_transformation_matrix(&scene->get_geometries()[i]->inverse_transform_matrix,
                                           scene->get_geometries()[i]->position,
                                           scene->get_geometries()[i]->orientation,
                                           scene->get_geometries()[i]->scale);
        make_transformation_matrix(&scene->get_geometries()[i]->transform_matrix,
                                   scene->get_geometries()[i]->position,
                                   scene->get_geometries()[i]->orientation,
                                   scene->get_geometries()[i]->scale);
        make_normal_matrix(&scene->get_geometries()[i]->normal_matrix,
                           scene->get_geometries()[i]->transform_matrix);

        // calculate bounding volume for models
        scene->get_geometries()[i]->make_bounding_volume();
        cout << "Created bounding volume for geometry " << i << endl;
    }

    return true;
}


/**
 * Performs a raytrace on the given pixel on the current scene.
 * The pixel is relative to the bottom-left corner of the image.
 * @param scene The scene to trace.
 * @param pixel The x and y screen coordinates of the pixel to trace.
 * @param width The width of the screen in pixels.
 * @param height The height of the screen in pixels.
 * @param recursions The number of times the function has been called.
 * @param start_e The ray origin
 * @param start_ray The ray direction
 * @param refractive The index of refraction surrounding the ray
 * @param extras Whether to turn extras on
 * @return The color of that pixel in the final image.
 */
Color3 Raytracer::trace_pixel(Int2 pixel, size_t width, size_t height,
        int recursions, Vector3 start_e, Vector3 start_ray,
        float refractive, bool extras)
{
    assert(0 <= pixel.x && pixel.x < width);
    assert(0 <= pixel.y && pixel.y < height);

    int max_recursion_depth = 3;
    float eps = 0.0001; // "slop factor"
    Vector3 eye; // origin of our viewing ray
    Vector3 ray; // viewing ray
    size_t num_geometries = scene->num_geometries();
    bool hit = false;
    intersect_info info; // everything we're calculating from intersection
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
        eye = scene->camera.get_position();
        ray = get_viewing_ray(pixel, width, height);
    }
    // otherwise use the ray that's passed in
    else
    {
        eye = start_e;
        ray = start_ray;
    }

    // run intersection test on every object in scene
    for (size_t i = 0; i < num_geometries; i++)
    {
        // intersect returns true if there's a hit, false if not, and sets
        //  values in info struct
        hit = scene->get_geometries()[i]->intersect_ray(eye, ray, &info);

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
                intersection_point = eye + (min_time * ray);
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

            return direct + min_texture * min_specular * trace_pixel( pixel,
                    width, height, recursions + 1, reflection_point,
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
                    return trace_pixel(pixel, width, height, recursions + 1,
                                       reflection_point, incident_ray, refractive, extras);
                }
            }

            // schlick approximation to fresnel equations
            float R_0 = pow(refract_ratio - 1, 2) / pow(refract_ratio + 1, 2);
            float R = R_0 + (1 - R_0) * pow(1 - c, 5);
            Vector3 refraction_point = intersection_point + eps * transmitted_ray;

            // return reflected and refracted rays
            return R * trace_pixel(pixel, width, height, recursions + 1,
                                   reflection_point, incident_ray, refractive, extras) +
                   (1.0 - R) * trace_pixel(pixel, width, height, recursions + 1,
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
Vector3 Raytracer::get_viewing_ray(Int2 pixel, size_t width, size_t height)
{
    // normalized camera direction
    Vector3 gaze = normalize(scene->camera.get_direction());
    // normalized camera up direction
    Vector3 up = normalize(scene->camera.get_up());
    // normalized camera right direction
    Vector3 right = cross(gaze, up);
    // camera field of view
    float fov = scene->camera.get_fov_radians();
    // t, b, r, and l are the border disances of the image plane
    real_t t = tan((width / height) * fov / 2.0);
    real_t r = (t * width) / height;
    real_t b = -1.0 * t;
    real_t l = -1.0 * r;
    // the pixel's horizontal coordinate on image plane
    real_t u = l + (r - l) * (pixel.x + 0.5) / width; 
    // the pixel's vertical coordinate on image plane
    real_t v = b + (t - b) * (pixel.y + 0.5) / height;
    // Shirley uses the near plane for the below calculation; we'll just use 1
    Vector3 view_ray = gaze + (u * right) + (v * up);

    return normalize(view_ray); // viewing ray direction
}

// parameters are the four ray origin coordinates on the screen, the camera/eye
// position, the dimensions of the screen, and a place to store the frustum.
void Raytracer::get_viewing_frustum(Int2 ll, Int2 lr, Int2 ul, Int2 ur,
                                    Vector3 eye, size_t width, size_t height,
                                    Frustum &frustum)
{
    // normalized camera direction
    Vector3 gaze = normalize(scene->camera.get_direction());
    // near and far clipping planes
    real_t near = scene->camera.get_near_clip();
    real_t far = scene->camera.get_far_clip();

    // directions of the frustum's corner rays
    Vector3 ll_ray = get_viewing_ray(ll, width, height);
    Vector3 lr_ray = get_viewing_ray(lr, width, height);
    Vector3 ul_ray = get_viewing_ray(ul, width, height);
    Vector3 ur_ray = get_viewing_ray(ur, width, height);

    // get side planes' normals by crossing them
    // these normals will point INWARD
    frustum.planes[TOP].normal = cross(ul_ray, ur_ray);
    frustum.planes[RIGHT].normal = cross(ur_ray, lr_ray);
    frustum.planes[BOTTOM].normal = cross(lr_ray, ll_ray);
    frustum.planes[LEFT].normal = cross(ll_ray, ul_ray);

    // gaze and negative gaze are normals for front and back
    frustum.planes[FRONT].normal = gaze;
    frustum.planes[BACK].normal = -1.0 * gaze;

    // centers of the front and back planes
    frustum.planes[FRONT].point = eye + gaze * near;
    frustum.planes[BACK].point = eye + gaze * far;

    // camera position is a point on top, bottom, left, and right planes
    frustum.planes[TOP].point = eye;
    frustum.planes[BOTTOM].point = eye;
    frustum.planes[LEFT].point = eye;
    frustum.planes[RIGHT].point = eye;
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

void Raytracer::trace_packet_worker(tsqueue<Packet> *packet_queue, unsigned char *buffer)
{
    while (true)
    {
        bool empty;
        Packet packet = packet_queue->Pop(empty);

        if (empty)
        {
            break;
        }

        trace_packet(packet, width, height, 0, 1.0, false, buffer);
    }
}

void Raytracer::trace_packet(Packet packet, size_t width, size_t height,
        int recursions, float refractive, bool extras, unsigned char *buffer)
{
    // these are just placeholders; trace_pixels finds its own starting ray
    Vector3 start_e = Vector3(0.0, 0.0, 0.0);
    Vector3 start_ray = Vector3(0.0, 0.0, 0.0);

    Int2 ul = packet.ul;
    Int2 ur = packet.ur;
    Int2 ll = packet.ll;
    Int2 lr = packet.lr;
    Frustum frustum;
    get_viewing_frustum(ll, lr, ul, ur, start_e,
            width, height, frustum);

    // run frustum intersection test on every object in scene
    for (size_t i = 0; i < scene->num_geometries(); i++)
    {
        bool hit = scene->get_geometries()[i]->intersect_frustum(frustum);

        if (! hit) // no intersection
        {
            // TODO keep up/down straight
            // TODO <=?
            // TODO SIMD
            // set all of packet's pixels to background color
            Color3 color = scene->background_color;

            for (int y = ll.y; y <= ur.y; y++)
            {
                for (int x = ll.x; x <= lr.x; x++)
                {
                    color.to_array(&buffer[4 * (y * width + x)]);
                }
            }
        }
        else // trace each pixel in the packet
        {
            for (int y = ll.y; y <= ur.y; y++)
            {
                for (int x = ll.x; x <= lr.x; x++)
                {
                    Int2 pixel = Int2(x, y);
                    Color3 color = trace_pixel(pixel, width, height,
                            0, start_e, start_ray, 1.0, false);
                    color.to_array(&buffer[4 * (y * width + x)]);
                }
            }
        }
    }
}

/**
 * Raytraces some portion of the scene. Should raytrace for about
 * max_time duration and then return, even if the raytrace is not complete.
 * The results should be placed in the given buffer.
 * @param buffer The buffer into which to place the color data. It is
 *  32-bit RGBA (4 bytes per pixel), in row-major order.
 * @param max_time, If non-null, the maximum suggested time this
 *  function raytrace before returning, in seconds. If null, the raytrace
 *  should run to completion.
 * @return true if the raytrace is complete, false if there is more
 *  work to be done.
 */
bool Raytracer::raytrace(unsigned char *buffer, real_t* max_time, bool extras, int numthreads)
{
    boost::thread *thread = new boost::thread[numthreads];
    tsqueue<Packet> packet_queue;

    double tot_start = CycleTimer::currentSeconds();

    for (size_t y = 0; y < height; y += PACKET_DIM)
    {
        int ymax = y + PACKET_DIM - 1;

        if (ymax >= height)
        {
            ymax = height - 1;
        }

        for (size_t x = 0; x < width; x += PACKET_DIM )
        {
            int xmax = x + PACKET_DIM - 1;

            if (xmax >= width)
            {
                xmax = width - 1;
            }

            Int2 ll(x, y);
            Int2 lr(xmax, y);
            Int2 ul(x, ymax);
            Int2 ur(xmax, ymax);
            Packet packet(ll, lr, ul, ur);
            packet_queue.Push(packet);

            /*
            cout << "ll: (" << ll.x << ", " << ll.y << "), ";
            cout << "lr: (" << lr.x << ", " << lr.y << "), ";
            cout << "ul: (" << ul.x << ", " << ul.y << "), ";
            cout << "ur: (" << ur.x << ", " << ur.y << ")";
            cout << endl;
            */
        }
    }

    double push_duration = CycleTimer::currentSeconds() - tot_start;
    double thread_start = CycleTimer::currentSeconds();

    for (int i = 0; i < numthreads; i++)
    {
        //cout << "Launching thread " << i << endl;
        thread[i] = boost::thread(&Raytracer::trace_packet_worker, this, &packet_queue, buffer);
    }

    for (int i = 0; i < numthreads; i++)
    {
        //cout << "Joining thread " << i << endl;
        thread[i].join();
    }

    double thread_duration = CycleTimer::currentSeconds() - thread_start;
    double tot_duration = CycleTimer::currentSeconds() - tot_start;

    cout << numthreads << " Total time:    " << tot_duration    << endl
         << numthreads << " Push time:     " << push_duration   << endl
         << numthreads << " Thread time:   " << thread_duration << endl;

    delete [] thread;

    return true;
}


} /* _462 */

