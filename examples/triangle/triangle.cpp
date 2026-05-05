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

const ExampleInfo exampleInfo = {
    .screenWidth  = 800,
    .screenHeight = 450,
    .appName      = "rhio example - triangle",
};

bool
InitExample()
{
    const auto initInfo = riInitInfo {
        .appName = exampleInfo.appName,
    };

    return rhioInit( &initInfo );
}

void
OnFramebufferResize( [[maybe_unused]] int width, [[maybe_unused]] int height )
{
}

void
ShutdownExample()
{
    rhioShutdown();
}
