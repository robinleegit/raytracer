/**
 * @file scene_loader.hpp
 * @brief Scene Loader
 *
 * @author Eric Butler (edbutler)
 */

#pragma once

namespace _462
{

class Scene;

/**
 * Loads a scene from a .scene file.
 * Clears away the old scene. Prints a message to stdout if an error occurs.
 * @return True on success, false on error.
 * Will clear the scene on error.
 */
bool load_scene( Scene* scene, const char* filename );

} /* _462 */

