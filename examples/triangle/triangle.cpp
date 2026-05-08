/*******************************************************************************************
*
*   rhio example - triangle
*
*   Example licensed under an unmodified zlib/libpng license.
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
********************************************************************************************/

#include "example.hpp"

#include "rhio.h"

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

static riDevice rhioDevice    = nullptr;

const ExampleInfo exampleInfo = {
    .screenWidth  = 800,
    .screenHeight = 450,
    .appName      = "rhio example - triangle",
};

bool
InitExample()
{
    const auto deviceInfo = riDeviceInfo {
        .base = {
            .appName = exampleInfo.appName,
        },
#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
        .backend = RI_BACKEND_VULKAN,
#elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
        .backend = RI_BACKEND_OPENGLES,
#else
        .backend = RI_BACKEND_OPENGL,
#endif
    };

    rhioDevice = rhioCreate( &deviceInfo );
    return rhioDevice != nullptr;
}

void
OnFramebufferResize( [[maybe_unused]] int width, [[maybe_unused]] int height )
{
}

void
OnKey( [[maybe_unused]] int key, [[maybe_unused]] int action )
{
}

void
ShutdownExample()
{
    if( rhioDevice != nullptr )
        {
            rhioDestroyDevice( rhioDevice );
            rhioDevice = nullptr;
        }
}
