/*******************************************************************************************
*
*   rhio example - GLFW window module
*
*   Example module for creating a GLFW window for OpenGL, OpenGL ES/WebGL and Vulkan
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
********************************************************************************************/

#include "window.hpp"

#include "rhio.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <string>

//----------------------------------------------------------------------------------
// Module Internal State
//----------------------------------------------------------------------------------

struct WindowState
{
    GLFWwindow *         handle         = nullptr;
    WindowResizeCallback resizeCallback = nullptr;
    std::string          title;
};

WindowState windowState;

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------

void         SetWindowHints();
const char * BackendName();
std::string  BuildWindowTitle( const char * title );
void         FramebufferSizeCallback( GLFWwindow *, int width, int height );
void         ErrorCallback( int error, const char * description );

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

bool
InitWindow( const WindowInfo & info )
{
    glfwSetErrorCallback( ErrorCallback );

    if( !glfwInit() )
        {
            return false;
        }

    SetWindowHints();

#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    if( !glfwVulkanSupported() )
        {
            glfwTerminate();
            return false;
        }
#endif

    windowState.title = BuildWindowTitle( info.title );

    windowState.handle
        = glfwCreateWindow( info.screenWidth, info.screenHeight, windowState.title.c_str(), nullptr, nullptr );
    if( windowState.handle == nullptr )
        {
            windowState.title.clear();
            glfwTerminate();
            return false;
        }

#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    glfwMakeContextCurrent( windowState.handle );
    glfwSwapInterval( 1 );
#endif

    glfwSetFramebufferSizeCallback( windowState.handle, FramebufferSizeCallback );

    return true;
}

void
CloseWindow()
{
    if( windowState.handle != nullptr )
        {
            glfwDestroyWindow( windowState.handle );
            windowState.handle = nullptr;
        }

    windowState.resizeCallback = nullptr;
    windowState.title.clear();
    glfwTerminate();
}

bool
WindowShouldClose()
{
    return ( windowState.handle == nullptr ) || glfwWindowShouldClose( windowState.handle );
}

void
BeginWindowFrame()
{
    glfwPollEvents();
}

void
EndWindowFrame()
{
#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    glfwSwapBuffers( windowState.handle );
#endif
}

void
SetWindowResizeCallback( WindowResizeCallback callback )
{
    windowState.resizeCallback = callback;

    if( ( windowState.resizeCallback != nullptr ) && ( windowState.handle != nullptr ) )
        {
            int width  = 0;
            int height = 0;

            glfwGetFramebufferSize( windowState.handle, &width, &height );
            windowState.resizeCallback( width, height );
        }
}

GLFWwindow *
GetWindowHandle()
{
    return windowState.handle;
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

void
SetWindowHints()
{
    glfwDefaultWindowHints();

#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
#else
    glfwWindowHint( GLFW_CLIENT_API,
#    if defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
                    GLFW_OPENGL_ES_API
#    else
                    GLFW_OPENGL_API
#    endif
    );

#    if defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, RHIO_EXAMPLE_OPENGL_ES_VERSION );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
#    else
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#        if defined( __APPLE__ )
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
#        endif
#    endif
#endif
}

const char *
BackendName()
{
#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    return "Vulkan";
#elif defined( __EMSCRIPTEN__ )
#    if defined( RHIO_EXAMPLE_OPENGL_ES_VERSION ) && ( RHIO_EXAMPLE_OPENGL_ES_VERSION == 2 )
    return "WebGL 1";
#    elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION ) && ( RHIO_EXAMPLE_OPENGL_ES_VERSION == 3 )
    return "WebGL 2";
#    else
    return "WebGL";
#    endif
#elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
#    if RHIO_EXAMPLE_OPENGL_ES_VERSION == 2
    return "OpenGL ES 2";
#    elif RHIO_EXAMPLE_OPENGL_ES_VERSION == 3
    return "OpenGL ES 3";
#    else
    return "OpenGL ES";
#    endif
#elif defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    return "OpenGL";
#else
    return "Unknown";
#endif
}

std::string
BuildWindowTitle( const char * title )
{
    auto windowTitle = std::string { ( title != nullptr ) ? title : "rhio example" };

    windowTitle += " [";
    windowTitle += BackendName();
    windowTitle += "]";

    return windowTitle;
}

void
FramebufferSizeCallback( GLFWwindow *, int width, int height )
{
    if( windowState.resizeCallback != nullptr )
        {
            windowState.resizeCallback( width, height );
        }
}

void
ErrorCallback( int error, const char * description )
{
    std::fprintf( stderr, "[rhio] GLFW error %d: %s\n", error, description );
}
