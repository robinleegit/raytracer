#ifndef __BOX_H__
#define __BOX_H__

namespace _462
{

class Box
{
public:
    Box() { }
    Box(Vector3 min, Vector3 max) : min_corner(min), max_corner(max) { }
    Vector3 min_corner, max_corner;
    bool intersect(Vector3 e, Vector3 d) const
    {
        Vector3 tmin, tmax;

        tmin.x = (min_corner.x - e.x) / d.x;
        tmax.x = (max_corner.x - e.x) / d.x;
        if (d.x < 0)
            std::swap(tmin.x, tmax.x);

        tmin.y = (min_corner.y - e.y) / d.y;
        tmax.y = (max_corner.y - e.y) / d.y;
        if (d.y < 0)
            std::swap(tmin.y, tmax.y);

        tmin.z = (min_corner.z - e.z) / d.z;
        tmax.z = (max_corner.z - e.z) / d.z;
        if (d.z < 0)
            std::swap(tmin.z, tmax.z);

        bool xzok, yzok, yxok;
        xzok = !(tmin.z > tmax.x || tmin.x > tmax.z);
        yzok = !(tmin.z > tmax.y || tmin.y > tmax.z);
        yxok = !(tmin.x > tmax.y || tmin.y > tmax.x);

        return xzok && yzok && yxok;
    }
};

}

#endif
