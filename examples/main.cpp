/*******************************************************************************************
*
*   rhio example - shared entry point
*
*   This example structure compiles for desktop OpenGL, desktop OpenGL ES, Vulkan and
*   Emscripten/WebGL.
*
*   Example licensed under an unmodified zlib/libpng license.
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
********************************************************************************************/

#include "example.hpp"
#include "window.hpp"

#include <cstdlib>

#if defined( __EMSCRIPTEN__ )
#    include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

void UpdateDrawFrame();

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------

int
main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const auto windowInfo = WindowInfo {
        .screenWidth  = exampleInfo.screenWidth,
        .screenHeight = exampleInfo.screenHeight,
        .title        = exampleInfo.appName,
    };

    if( !InitWindow( windowInfo ) )
        {
            return 1;
        }

    SetWindowResizeCallback( OnFramebufferResize );
    SetWindowKeyCallback( OnKey );

    if( !InitExample() )
        {
            CloseWindow();
            return 1;
        }

#if defined( __EMSCRIPTEN__ )
    emscripten_set_main_loop( UpdateDrawFrame, 0, 1 );
#else
    // Frame Cap
    //----------------------------------------------------------
    // Optional test harness limit (CI smoke); 0 or unset runs forever
    long maxFrames  = 0;
    long frameCount = 0;
    if( const char * envValue = std::getenv( "RHIO_EXAMPLE_MAX_FRAMES" ) )
        {
            maxFrames = std::atol( envValue );
        }

    // Main game loop
    while( !WindowShouldClose() )
        {
            UpdateDrawFrame();

            if( ( maxFrames > 0 ) && ( ++frameCount >= maxFrames ) )
                {
                    break;
                }
        }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    ShutdownExample();
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

void
UpdateDrawFrame()
{
    // Update
    //----------------------------------------------------------------------------------
    if( WindowShouldClose() )
        {
#if defined( __EMSCRIPTEN__ )
            ShutdownExample();
            CloseWindow();
            emscripten_cancel_main_loop();
#endif
            return;
        }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginWindowFrame();
    DrawExample();
    EndWindowFrame();
    //----------------------------------------------------------------------------------
}
