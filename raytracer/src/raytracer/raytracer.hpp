#ifndef _462_RAYTRACER_HPP_
#define _462_RAYTRACER_HPP_

#include "math/color.hpp"
#include "scene/scene.hpp"
#include "tsqueue.hpp"
#include "geom_utils.hpp"

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

struct Packet
{
    Int2 ll, lr, ul, ur;
    Packet()
    {
        ll = Int2(0, 0);
        lr = Int2(0, 0);
        ul = Int2(0, 0);
        ur = Int2(0, 0);
    }
    Packet(Int2 _ll, Int2 _lr, Int2 _ul, Int2 _ur) :
        ll(_ll), lr(_lr), ul( _ul), ur(_ur) { }
};

class Scene;

class Raytracer
{
public:

    Raytracer();

    ~Raytracer();

    bool initialize(Scene* _scene, size_t _width, size_t _height);

    Color3 trace_pixel(Int2 pixel, int recursions, Vector3 start_eye,
            Vector3 start_ray, float refractive, bool extras);

    void trace_packet_worker(tsqueue<Packet> *packet_queue, unsigned char *buffer);

    bool raytrace(unsigned char* buffer, real_t* max_time, bool extras, int numthreads);

    Vector3 get_viewing_ray(Int2 pixel);

    void get_viewing_frustum(Int2 ll, Int2 lr, Int2 ul, Int2 ur,
                             Frustum& frustum);

    Color3 get_diffuse(Vector3 intersection_point, Vector3 min_normal,
                       Color3 min_diffuse, float eps);

    bool refract(Vector3 d, Vector3 normal, float n, Vector3 *t);

    void trace_packet(Packet packet, float refractive, bool extras,
            unsigned char* buffer);

private:

    // the scene to trace
    Scene* scene;

    // the dimensions of the image to trace
    size_t width, height;
};

} /* _462 */

#endif /* _462_RAYTRACER_HPP_ */

