#include "geom_utils.hpp"
#include "raytracer/ray.hpp"

using namespace std;

namespace _462
{

bool triangle_ray_intersect(Vector3 eye, Vector3 ray, Vector3 p0, Vector3 p1,
                            Vector3 p2, float &min_time, float &min_gamma,
                            float &min_beta)
{
    float x0, y0, z0, x1, y1, z1, x2, y2, z2;
    float beta = 0.0, gamma = 0.0;
    float t;

    x0 = p0.x;
    y0 = p0.y;
    z0 = p0.z;
    x1 = p1.x;
    y1 = p1.y;
    z1 = p1.z;
    x2 = p2.x;
    y2 = p2.y;
    z2 = p2.z;

    float a = x0 - x1;
    float b = y0 - y1;
    float c = z0 - z1;
    float d = x0 - x2;
    float e = y0 - y2;
    float f = z0 - z2;
    float g = ray.x;
    float h = ray.y;
    float i = ray.z;
    float j = x0 - eye.x;
    float k = y0 - eye.y;
    float l = z0 - eye.z;
    float ei_minus_hf = e * i - h * f;
    float gf_minus_di = g * f - d * i;
    float dh_minus_eg = d * h - e * g;
    float ak_minus_jb = a * k - j * b;
    float jc_minus_al = j * c - a * l;
    float bl_minus_kc = b * l - k * c;
    float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
    t = -1.0 * (f * ak_minus_jb + e * jc_minus_al + d * bl_minus_kc) / m;

    gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;
    beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;
    float alpha = 1 - beta - gamma;

    bool gamma_ok = gamma >= 0.0 && gamma <= 1.0;
    bool beta_ok = beta >= 0.0 && beta <= 1.0;
    bool alpha_ok = alpha >= 0.0 && alpha <= 1.0;
    bool t_better = t < min_time && t > eps;

    bool replace = alpha_ok && gamma_ok && beta_ok && t_better;

    min_time  = replace ? t     : min_time;
    min_gamma = replace ? gamma : min_gamma;
    min_beta  = replace ? beta  : min_beta;;
    
    return replace;
}

bool frustum_box_intersect(Frustum frustum, Vector3 box_min, Vector3 box_max)
{
    Vector3 box[8];
    
    box[0] = box_min;
    box[1] = Vector3(box_min.x, box_min.y, box_max.z);
    box[2] = Vector3(box_min.x, box_max.y, box_min.z);
    box[3] = Vector3(box_min.x, box_max.y, box_max.z);
    box[4] = Vector3(box_max.x, box_min.y, box_min.z);
    box[5] = Vector3(box_max.x, box_min.y, box_max.z);
    box[6] = Vector3(box_max.x, box_max.y, box_min.z);
    box[7] = box_max;

    // test each plane of frustum individually; if the point is on the wrong
    // side of the plane, the box is outside the frustum and we can exit
    int right_side = 0;

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Vector3 point = box[j];
            Plane plane = frustum.planes[i];

            if (dot(point - plane.point, plane.normal) < 0.0)
            {
                right_side++;
                break;
            }
        }
    }

    if (right_side == 6)
    {
        return true;
    }

    return false;
}

}

