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
    // Device Configuration
    //----------------------------------------------------------
    // Backend is selected by the example build flags.
    riDeviceInfo deviceInfo = {};
    deviceInfo.base.appName = exampleInfo.appName;
    RI_FLAG_SET( deviceInfo.flags, RI_DEVICE_FLAG_DEBUG );

// Determine backend
#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    deviceInfo.backend = RI_BACKEND_VULKAN;
#elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
    deviceInfo.backend = RI_BACKEND_OPENGLES;
#else
    deviceInfo.backend = RI_BACKEND_OPENGL;
#endif

    return RI_SUCCEEDED( rhioCreateDevice( &deviceInfo, &rhioDevice ) );
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
