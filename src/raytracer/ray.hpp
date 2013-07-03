#pragma once

#include "raytracer/geom_utils.hpp"
#include "math/color.hpp"

namespace _462
{

const int packet_dim = 8;
const int rays_per_packet = (packet_dim * packet_dim);
const float eps = 0.0001; // "slop factor"
const int max_recursion_depth = 1;

struct Ray
{
    Vector3 eye;
    Vector3 dir;
};

struct PacketRegion
{
    PacketRegion() {}
    PacketRegion(Int2 _ll, Int2 _lr, Int2 _ul, Int2 _ur)
                : ll(_ll), lr(_lr), ul(_ul), ur(_ur)
    {
    }
    Int2 ll, lr, ul, ur;
};

struct Packet
{
    Frustum frustum;
    Ray rays[rays_per_packet];
};

struct IsectInfo
{
    IsectInfo() : time(INFINITY), normal(Vector3::Zero), ambient(Color3::Black), 
                  diffuse(Color3::Black), specular(Color3::Black), 
                  texture(Color3::Black), refractive(0)
    {
    }
    float time;
    Vector3 normal;
    Color3 ambient;
    Color3 diffuse;
    Color3 specular;
    Color3 texture;
    real_t refractive;
};

}
