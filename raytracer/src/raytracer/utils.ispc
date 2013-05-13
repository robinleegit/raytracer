const float eps = 0.0001;

struct IsectInfo
{
    float time;
    float gamma;
    float beta;
};

struct Ray
{
    float eye[3];
    float dir[3];
};

export void set_indices(uniform int a[], uniform int length)
{
    foreach (i = 0 ... length)
    {
        a[i] = i;
    }
}

bool triangle_ray_intersect(float eye[3], 
                            float ray[3], 
                            float p0[3], 
                            float p1[3],
                            float p2[3], 
                            float &min_time, 
                            float &min_gamma,
                            float &min_beta)
{
    float x0, y0, z0, x1, y1, z1, x2, y2, z2;
    float beta = 0.0, gamma = 0.0;
    float t;

    x0 = p0[0];
    y0 = p0[1];
    z0 = p0[2];

    x1 = p1[0];
    y1 = p1[1];
    z1 = p1[2];

    x2 = p2[0];
    y2 = p2[1];
    z2 = p2[2];

    float a = x0 - x1;
    float b = y0 - y1;
    float c = z0 - z1;
    float d = x0 - x2;
    float e = y0 - y2;
    float f = z0 - z2;
    float g = ray[0];
    float h = ray[1];
    float i = ray[2];
    float j = x0 - eye[0];
    float k = y0 - eye[1];
    float l = z0 - eye[2];
    float ei_minus_hf = e * i - h * f;
    float gf_minus_di = g * f - d * i;
    float dh_minus_eg = d * h - e * g;
    float ak_minus_jb = a * k - j * b;
    float jc_minus_al = j * c - a * l;
    float bl_minus_kc = b * l - k * c;
    float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
    t = -1.0 * (f * ak_minus_jb + e * jc_minus_al + d * bl_minus_kc) / m;

    if (t < 0.0)
    {
        return false;
    }

    gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

    if (gamma < 0.0 || gamma > 1.0)
    {
        return false;
    }

    beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

    if (beta < 0.0 || beta > 1.0 - gamma)
    {
        return false;
    }

    if (t < min_time && t > eps)
    {
        min_time = t;
        min_gamma = gamma;
        min_beta = beta;

        return true;
    }

    return false;
}

export void triangle_packet_intersect(uniform Ray rays[], 
                                      uniform IsectInfo infos[], 
                                      uniform int8 intersected[], 
                                      uniform int num_rays, 
                                      uniform float p0[3], 
                                      uniform float p1[3], 
                                      uniform float p2[3])
{
    foreach(r = 0 ... num_rays)
    {
        float eye[3] = rays[r].eye;
        float ray[3] = rays[r].dir;

        float x0, y0, z0, x1, y1, z1, x2, y2, z2;
        float beta = 0.0, gamma = 0.0;
        float t;

        x0 = p0[0];
        y0 = p0[1];
        z0 = p0[2];

        x1 = p1[0];
        y1 = p1[1];
        z1 = p1[2];

        x2 = p2[0];
        y2 = p2[1];
        z2 = p2[2];

        float a = x0 - x1;
        float b = y0 - y1;
        float c = z0 - z1;
        float d = x0 - x2;
        float e = y0 - y2;
        float f = z0 - z2;
        float g = ray[0];
        float h = ray[1];
        float i = ray[2];
        float j = x0 - eye[0];
        float k = y0 - eye[1];
        float l = z0 - eye[2];
        float ei_minus_hf = e * i - h * f;
        float gf_minus_di = g * f - d * i;
        float dh_minus_eg = d * h - e * g;
        float ak_minus_jb = a * k - j * b;
        float jc_minus_al = j * c - a * l;
        float bl_minus_kc = b * l - k * c;
        float m = a * ei_minus_hf + b * gf_minus_di + c * dh_minus_eg;
        t = -1.0 * (f * ak_minus_jb + e * jc_minus_al + d * bl_minus_kc) / m;

        if (t < 0.0)
        {
            intersected[i] = false;
        }

        gamma = (i * ak_minus_jb + h * jc_minus_al + g * bl_minus_kc) / m;

        if (gamma < 0.0 || gamma > 1.0)
        {
            intersected[i] = false;
        }

        beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / m;

        if (beta < 0.0 || beta > 1.0 - gamma)
        {
            intersected[i] = false;
        }

        if (t < infos[i].time && t > eps)
        {
            infos[i].time = t;
            infos[i].gamma = gamma;
            infos[i].beta = beta;

            intersected[i] = true;
        }

        intersected[i] = false;
    }
}
