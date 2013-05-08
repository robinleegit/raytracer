#ifndef _462_RAYTRACER_HPP_
#define _462_RAYTRACER_HPP_

#include "math/color.hpp"
#include "scene/scene.hpp"
#include "tsqueue.hpp"

namespace _462
{

struct Int2
{
    int x, y;
    Int2()
    {
        x = 0;
        y = 0;
    }
    Int2(int _x, int _y) : x(_x), y(_y) { }
};

struct Plane
{
    // define elements of a Plane
};

struct Frustum
{
    // define top, bottom, left, right, front, back planes
};

class Scene;

class Raytracer
{
public:

    Raytracer();

    ~Raytracer();

    bool initialize( Scene* scene, size_t width, size_t height );

    Color3 trace_pixel(const Scene* scene, Int2 pixel, size_t width,
                       size_t height, int recursions, Vector3 start_e, Vector3 start_ray,
                       float refractive, bool extras);

    void trace_pixel_worker(tsqueue<Int2> *pixel_queue, unsigned char *buffer);

    bool raytrace(unsigned char* buffer, real_t* max_time, bool extras, int numthreads);

    Vector3 get_viewing_ray(Vector3 e, Int2 pixel, size_t width, size_t height);

    void get_viewing_frustum(Int2 ul, Int2 ur, Int2 ll,
        Int2 lr, Vector3 e, size_t width, size_t height, Frustum frustum);

    Color3 get_diffuse(Vector3 intersection_point, Vector3 min_normal,
                       Color3 min_diffuse, float eps);

    bool refract(Vector3 d, Vector3 normal, float n, Vector3 *t);


private:

    // the scene to trace
    Scene* scene;

    // the dimensions of the image to trace
    size_t width, height;

};

} /* _462 */

#endif /* _462_RAYTRACER_HPP_ */

