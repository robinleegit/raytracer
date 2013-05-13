#include <SDL/SDL_timer.h>
#include <iostream>
#include <math.h>
#include <algorithm>

#include "raytracer.hpp"
#include "CycleTimer.hpp"

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
bool Raytracer::initialize(Scene* _scene, size_t _width, size_t _height, bool _extras)
{
    this->scene = _scene;
    this->width = _width;
    this->height = _height;
    this->extras = _extras;

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

    //cout << scene->camera.orientation << endl;

    return true;
}


/**
 * Performs a raytrace on the given pixel on the current scene.
 * The pixel is relative to the bottom-left corner of the image.
 */
Color3 Raytracer::trace_pixel(int recursions, const Ray& ray, float refractive)
{
    size_t num_geometries = scene->num_geometries();
    bool hit_any = false; // if any geometries were hit
    IsectInfo min_info; // everything we're calculating from intersection
    Vector3 intersection_point = Vector3::Zero;

    // run intersection test on every object in scene
    for (size_t i = 0; i < num_geometries; i++)
    {
        IsectInfo info;
        // intersect returns true if there's a hit, false if not, and sets
        // values in info struct
        bool hit = scene->get_geometries()[i]->intersect_ray(ray, info);

        if (hit && info.time < min_info.time) // min_info.time initializes to inf
        {
            min_info = info;
            intersection_point = ray.eye + (min_info.time * ray.dir);
            hit_any = true;
        }
    }

    // found a hit
    if (hit_any)
    {
        Color3 direct;
        Color3 diffuse = Color3::Black;
        Color3 ambient = scene->ambient_light * min_info.ambient;
        float angle = dot(ray.dir, min_info.normal);
        // compute reflected ray
        Ray incident_ray;
        incident_ray.dir = ray.dir - 2 * angle * min_info.normal;
        incident_ray.dir = normalize(incident_ray.dir);
        incident_ray.eye = intersection_point + eps * incident_ray.dir;

        // no-refraction case
        if (min_info.refractive == 0.0)
        {
            diffuse = get_diffuse(incident_ray.eye, min_info.normal,
                    min_info.diffuse, eps);
            direct = min_info.texture * (ambient + diffuse);

            // return direct light and reflected light if we have recursions left
            if (recursions >= max_recursion_depth)
            {
                return direct;
            }

            return direct + min_info.texture * min_info.specular *
                trace_pixel(recursions + 1, incident_ray, refractive);

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
            Ray transmitted_ray;
            float refract_ratio = refractive / min_info.refractive;

            // negative dot product between ray and normal indicates entering object
            if (angle < 0.0)
            {
                refract(ray.dir, min_info.normal, refract_ratio, &transmitted_ray.dir);
                c = dot(-1.0 * ray.dir, min_info.normal);
            }
            else
            {
                // exiting object
                if (refract(ray.dir, (-1.0 * min_info.normal), min_info.refractive,
                            &transmitted_ray.dir))
                {
                    c = dot(transmitted_ray.dir, min_info.normal);
                }
                // total internal reflection
                else
                {
                    return trace_pixel(recursions + 1, incident_ray, refractive);
                }
            }

            // schlick approximation to fresnel equations
            float R_0 = pow(refract_ratio - 1, 2) / pow(refract_ratio + 1, 2);
            float R = R_0 + (1 - R_0) * pow(1 - c, 5);
            transmitted_ray.eye = intersection_point + eps * transmitted_ray.dir;

            // return reflected and refracted rays
            return R * trace_pixel(recursions + 1, incident_ray, refractive) +
                (1.0 - R) * trace_pixel(recursions + 1, transmitted_ray,
                        min_info.refractive);
        }
    }
    // didn't hit anything - return background color
    else
    {
        return scene->background_color;
    }
}


Color3 Raytracer::trace_pixel_end(int recursions, const Ray& ray, float refractive, 
        IsectInfo min_info)
{
    Color3 direct;
    Color3 diffuse = Color3::Black;
    Color3 ambient = scene->ambient_light * min_info.ambient;
    float angle = dot(ray.dir, min_info.normal);

    // compute reflected ray
    Vector3 intersection_point = ray.eye + (min_info.time * ray.dir);
    Ray incident_ray;
    incident_ray.dir = ray.dir - 2 * angle * min_info.normal;
    incident_ray.dir = normalize(incident_ray.dir);
    incident_ray.eye = intersection_point + eps * incident_ray.dir;

    // no-refraction case
    if (min_info.refractive == 0.0)
    {
        diffuse = get_diffuse(incident_ray.eye, min_info.normal,
                min_info.diffuse, eps);
        direct = min_info.texture * (ambient + diffuse);

        // return direct light and reflected light if we have recursions left
        if (recursions >= max_recursion_depth)
        {
            return direct;
        }

        return direct + min_info.texture * min_info.specular *
            trace_pixel(recursions + 1, incident_ray, refractive);

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
        Ray transmitted_ray;
        float refract_ratio = refractive / min_info.refractive;

        // negative dot product between ray and normal indicates entering object
        if (angle < 0.0)
        {
            refract(ray.dir, min_info.normal, refract_ratio, &transmitted_ray.dir);
            c = dot(-1.0 * ray.dir, min_info.normal);
        }
        else
        {
            // exiting object
            if (refract(ray.dir, (-1.0 * min_info.normal), min_info.refractive,
                        &transmitted_ray.dir))
            {
                c = dot(transmitted_ray.dir, min_info.normal);
            }
            // total internal reflection
            else
            {
                return trace_pixel(recursions + 1, incident_ray, refractive);
            }
        }

        // schlick approximation to fresnel equations
        float R_0 = pow(refract_ratio - 1, 2) / pow(refract_ratio + 1, 2);
        float R = R_0 + (1 - R_0) * pow(1 - c, 5);
        transmitted_ray.eye = intersection_point + eps * transmitted_ray.dir;

        // return reflected and refracted rays
        return R * trace_pixel(recursions + 1, incident_ray, refractive) +
            (1.0 - R) * trace_pixel(recursions + 1, transmitted_ray,
                    min_info.refractive);
    }
}




// calculate direction of initial viewing ray from camera
Vector3 Raytracer::get_viewing_ray(Int2 pixel)
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
                                    Frustum &frustum)
{
    // camera position
    Vector3 eye = scene->camera.get_position();
    // normalized camera direction
    Vector3 gaze = normalize(scene->camera.get_direction());
    // near and far clipping planes
    real_t near = scene->camera.get_near_clip();
    real_t far = scene->camera.get_far_clip();

    // directions of the frustum's corner rays
    Vector3 ll_ray = get_viewing_ray(ll);
    Vector3 lr_ray = get_viewing_ray(lr);
    Vector3 ul_ray = get_viewing_ray(ul);
    Vector3 ur_ray = get_viewing_ray(ur);

    // get side planes' normals by crossing them
    // these normals will point OUTWARD
    frustum.planes[TOP].normal = cross(ur_ray, ul_ray);
    frustum.planes[RIGHT].normal = cross(lr_ray, ur_ray);
    frustum.planes[BOTTOM].normal = cross(ll_ray, lr_ray);
    frustum.planes[LEFT].normal = cross(ul_ray, ll_ray);

    // gaze and negative gaze are normals for back and front
    frustum.planes[FRONT].normal = -1.0 * gaze;
    frustum.planes[BACK].normal = gaze;

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
        Ray shadow_ray;
        shadow_ray.eye = intersection_point + (eps * light_direction);
        shadow_ray.dir = light_direction; 

        // first check if it's front facing the light
        if (front_face > 0)
        {
            // second, check if the light is blocked:
            for (size_t k = 0; k < num_geometries; k++)
            {
                //  send a ray from intersection point to that light
                in_shadow = scene->get_geometries()[k]->shadow_test(shadow_ray);

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

void Raytracer::trace_packet_worker(tsqueue<PacketRegion> *packet_queue, unsigned char *buffer)
{
    while (true)
    {
        bool empty;
        PacketRegion packet = packet_queue->Pop(empty);

        if (empty)
        {
            break;
        }

        trace_packet(packet, 1.0, buffer);
    }
}

void Raytracer::trace_packet(PacketRegion region, float refractive, unsigned char *buffer)
{
    Int2 ll = region.ll;
    Int2 lr = region.lr;
    Int2 ul = region.ul;
    Int2 ur = region.ur;

    IsectInfo infos[rays_per_packet];
    bool intersected[rays_per_packet];
    Packet packet;
    get_viewing_frustum(ll, lr, ul, ur, packet.frustum);
    Vector3 eye = scene->camera.get_position();
    Int2 pixels[rays_per_packet];
    int r = 0; // counter for rays in packet

    for (int y = ll.y; y <= ul.y; y++)
    {
        for (int x = ll.x; x <= lr.x; x++)
        {
            Int2 pixel(x, y);
            packet.rays[r].eye = eye;
            packet.rays[r].dir = get_viewing_ray(pixel);
            pixels[r] = pixel;
            intersected[r] = false;
            r++;
        }
    }

    for (size_t i = 0; i < scene->num_geometries(); i++)
    {
        scene->get_geometries()[i]->intersect_packet(packet, infos, intersected);
    }

    for (int i = 0; i < rays_per_packet; i++)
    {
        if (intersected[i])
        {
            Color3 color = trace_pixel_end(0, packet.rays[i], refractive, infos[i]);
            color.to_array(&buffer[4 * (pixels[i].y * width + pixels[i].x)]);
        }
        else
        {
            Color3 color = scene->background_color;
            color.to_array(&buffer[4 * (pixels[i].y * width + pixels[i].x)]);
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
bool Raytracer::raytrace(unsigned char *buffer, real_t* max_time, int numthreads)
{
    boost::thread *thread = new boost::thread[numthreads];
    tsqueue<PacketRegion> packet_queue;

    double tot_start = CycleTimer::currentSeconds();

    for (size_t y = 0; y < height; y += packet_dim)
    {
        int ymax = y + packet_dim - 1;

        if (ymax >= height)
        {
            ymax = height - 1;
        }

        for (size_t x = 0; x < width; x += packet_dim )
        {
            int xmax = x + packet_dim - 1;

            if (xmax >= width)
            {
                xmax = width - 1;
            }

            Int2 ll(x, y);
            Int2 lr(xmax, y);
            Int2 ul(x, ymax);
            Int2 ur(xmax, ymax);
            PacketRegion packet(ll, lr, ul, ur);
            packet_queue.Push(packet);
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

