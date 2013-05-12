#ifndef __PACKET_H__
#define __PACKET_H__

#include<iostream>

namespace _462
{

const int packet_dim = 16;
const int rays_per_packet = (packet_dim * packet_dim);

struct Int2
{
    int x, y;
    Int2() : x(0), y(0) { }
    Int2(int _x, int _y) : x(_x), y(_y) { }
};

struct intersect_info
{
    float i_time;
    Vector3 i_normal;
    Color3 i_ambient;
    Color3 i_diffuse;
    Color3 i_specular;
    Color3 i_texture;
    real_t i_refractive;
    size_t i_index;
    float i_beta;
    float i_gamma;
};

struct RayPacket
{
    RayPacket()
    {
        for (int i = 0; i < rays_per_packet; i++)
        {
            active[i] = false;
            infos[i].i_time = INFINITY;
        }
    }
    Vector3        eye;
    Vector3        rays[rays_per_packet];
    Int2           pixels[rays_per_packet];
    bool           active[rays_per_packet];
    intersect_info infos[rays_per_packet];
};

struct Packet
{
    Int2 ll, lr, ul, ur;
    Frustum frustum;
    RayPacket ray_packet;
    Packet() : ll(0, 0), lr(0, 0), ul(0, 0), ur(0, 0) { }
    Packet(Int2 _ll, Int2 _lr, Int2 _ul, Int2 _ur) :
        ll(_ll), lr(_lr), ul( _ul), ur(_ur) { }
};

}

#endif
