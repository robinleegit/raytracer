#include "box.hpp"

#include <algorithm>

using namespace std;

namespace _462
{


bool Box::intersect(Vector3 e, Vector3 r) const
{
    double tmin = -INFINITY, tmax = INFINITY;


    if (r.x != 0.0)
    {
        double tx1 = (min_corner.x - e.x)/r.x;
        double tx2 = (max_corner.x - e.x)/r.x;

        tmin = max(tmin, min(tx1, tx2));
        tmax = min(tmax, max(tx1, tx2));
    }

    if (r.y != 0.0)
    {
        double ty1 = (min_corner.y - e.y)/r.y;
        double ty2 = (max_corner.y - e.y)/r.y;

        tmin = max(tmin, min(ty1, ty2));
        tmax = min(tmax, max(ty1, ty2));
    }

    if (r.z != 0.0)
    {
        double tz1 = (min_corner.z - e.z)/r.z;
        double tz2 = (max_corner.z - e.z)/r.z;

        tmin = max(tmin, min(tz1, tz2));
        tmax = min(tmax, max(tz1, tz2));
    }

    return tmax >= tmin;
}

Box::Box(const Mesh* mesh, int n, int m)
{
    min_corner.x = INFINITY;
    min_corner.y = INFINITY;
    min_corner.z = INFINITY;

    max_corner.x = -INFINITY;
    max_corner.y = -INFINITY;
    max_corner.z = -INFINITY;

    cout << "Creating bounding box for triangles " << n << " -> " << m << endl;

    for (size_t i = n; i < m; i++)
    {
        MeshTriangle t = mesh->get_triangles()[i];

        for (size_t j = 0; j < 3; j++)
        {
            int vidx = t.vertices[j];
            MeshVertex v = mesh->get_vertices()[vidx];

            cout << "Vertex = " << v.position << endl;

            min_corner.x = min(min_corner.x, v.position.x);
            max_corner.x = max(max_corner.x, v.position.x);

            min_corner.y = min(min_corner.y, v.position.y);
            max_corner.y = max(max_corner.y, v.position.y);

            min_corner.z = min(min_corner.z, v.position.z);
            max_corner.z = max(max_corner.z, v.position.z);
        }
    }
}

}
