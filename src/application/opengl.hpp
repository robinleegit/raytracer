/**
 * @file opengl.hpp
 * @brief Includes the correct opengl header files depending on the
 *  platform. Use this file to include any gl header files.
 */

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#ifdef _462_USING_GLEW
#include <GL/glew.h>
#endif

#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>

