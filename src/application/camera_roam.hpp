/**
 * @file camera_roam.cpp
 * @brief CameraRoamControl class
 *
 * @author Zeyang Li (zeyangl)
 */

#pragma once

#include "application/application.hpp"
#include "math/camera.hpp"

namespace _462
{

class CameraRoamControl
{
public:

    CameraRoamControl();
    ~CameraRoamControl();

    void update(real_t dt);
    void handle_event(const Application* app, const SDL_Event& event,
            bool& raytrace_mouse_update);

    // the camera of this control
    Camera camera;

private:

    enum Direction { DZERO=0, DPOS=1, DNEG=2 };
    enum Rotation { RNONE, RPITCHYAW, RROLL };

    void set_dir( bool pressed, int index, Direction newdir );

    // current directions in the local camera axes, (x, y, z)
    Direction direction[3];
    // the current rotation
    Rotation rotation;
};

} /* _462 */

