//#ifndef _462_RAYTRACER_HPP_
//#define _462_RAYTRACER_HPP_

#pragma once

#include "raytracer/ray.hpp"
#include "math/color.hpp"
#include "scene/scene.hpp"
#include "tsqueue.hpp"
#include "geom_utils.hpp"

namespace _462
{

class Raytracer
{
public:

    Raytracer();

    ~Raytracer();

    bool initialize(Scene* _scene, size_t _width, size_t _height, bool _extras);

    bool raytrace(unsigned char* buffer, real_t* max_time, int numthreads);

    void trace_packet_worker(tsqueue<PacketRegion> *packet_queue, unsigned char *buffer);

    void trace_packet(PacketRegion packet, float refractive, unsigned char* buffer);

    void get_viewing_frustum(Int2 ll, Int2 lr, Int2 ul, Int2 ur,
                             Frustum& frustum);

    Vector3 get_viewing_ray(Int2 pixel);

    Color3 trace_pixel(int recursions, const Ray& ray, float refractive);

    Color3 trace_pixel_end(int recursions, const Ray& ray, float refractive,
            IsectInfo min_info);

    Color3 get_diffuse(Vector3 intersection_point, Vector3 min_normal,
                       Color3 min_diffuse, float eps);

    bool refract(Vector3 d, Vector3 normal, float n, Vector3 *t);

private:

    // the scene to trace
    Scene* scene;

    // the dimensions of the image to trace
    size_t width, height;

    // for things like anti-aliasing
    bool extras;
};

} /* _462 */

//#endif /* _462_RAYTRACER_HPP_ */

