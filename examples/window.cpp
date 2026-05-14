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
    WindowKeyCallback    keyCallback    = nullptr;
    std::string          title;
};

WindowState windowState;

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------

void        SetWindowHints();
std::string BuildWindowTitle( const char * title );
void        FramebufferSizeCallback( GLFWwindow *, int width, int height );
void        ErrorCallback( int error, const char * description );
void        KeyCallback( GLFWwindow * window, int key, int scancode, int action, int mods );

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

    // Center window on screen (match raylib behavior)
    GLFWmonitor * primary = glfwGetPrimaryMonitor();
    if( primary )
        {
            int xpos, ypos, width, height;
            glfwGetMonitorWorkarea( primary, &xpos, &ypos, &width, &height );
            glfwSetWindowPos( windowState.handle,
                              xpos + ( width - info.screenWidth ) / 2,
                              ypos + ( height - info.screenHeight ) / 2 );
        }

#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    glfwMakeContextCurrent( windowState.handle );
    glfwSwapInterval( 1 );
#endif

    glfwSetFramebufferSizeCallback( windowState.handle, FramebufferSizeCallback );
    glfwSetKeyCallback( windowState.handle, KeyCallback );

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

void
SetWindowKeyCallback( WindowKeyCallback callback )
{
    windowState.keyCallback = callback;
}

RI_FORCE_INLINE GLFWwindow *
GetWindowHandle()
{
    return windowState.handle;
}

WindowGlProc
GetWindowGlProcAddress( const char * name )
{
#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    return reinterpret_cast< WindowGlProc >( glfwGetProcAddress( name ) );
#else
    UNUSED( name );
    return nullptr;
#endif
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

void
SetWindowHints()
{
    glfwDefaultWindowHints();

    // Common window hints
    glfwWindowHint( GLFW_FLOATING, GLFW_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    glfwWindowHint( GLFW_AUTO_ICONIFY, GLFW_FALSE );

#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    // Vulkan: no graphics API context
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
#else
    // OpenGL or OpenGL ES path
#    if defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
    glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_ES_API );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, RHIO_EXAMPLE_OPENGL_ES_VERSION );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );

#    else
    // Desktop OpenGL 3.3 Core Profile
    glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_API );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

#        if defined( __APPLE__ )
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
#        endif

#    endif
#endif
}

std::string
BuildWindowTitle( const char * title )
{
    // Default title if none provided
    const char * baseTitle = ( title != nullptr ) ? title : "rhio example";

    // Determine backend name
#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    const char * backend = "Vulkan";

#elif defined( __EMSCRIPTEN__ )
#    if defined( RHIO_EXAMPLE_OPENGL_ES_VERSION ) && ( RHIO_EXAMPLE_OPENGL_ES_VERSION == 2 )
    const char * backend = "WebGL 1";
#    elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION ) && ( RHIO_EXAMPLE_OPENGL_ES_VERSION == 3 )
    const char * backend = "WebGL 2";
#    else
    const char * backend = "WebGL";
#    endif

#elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
#    if RHIO_EXAMPLE_OPENGL_ES_VERSION == 2
    const char * backend = "OpenGL ES 2";
#    elif RHIO_EXAMPLE_OPENGL_ES_VERSION == 3
    const char * backend = "OpenGL ES 3";
#    else
    const char * backend = "OpenGL ES";
#    endif

#elif defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    const char * backend = "OpenGL";

#else
    const char * backend = "Unknown";

#endif

    // Build the final window title
    return std::string( baseTitle ) + " [" + backend + "]";
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
KeyCallback( GLFWwindow * window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods )
{
    if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
        {
            glfwSetWindowShouldClose( window, GLFW_TRUE );
        }

    if( windowState.keyCallback != nullptr )
        {
            windowState.keyCallback( key, action );
        }
}

void
ErrorCallback( int error, const char * description )
{
    std::fprintf( stderr, "[rhio] GLFW error %d: %s\n", error, description );
}
