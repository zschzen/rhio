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

#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
#    include "window.hpp"
#endif

#include "rhio.h"

//----------------------------------------------------------------------------------
// Module Internal State
//----------------------------------------------------------------------------------

static riDevice       rhioDevice        = nullptr;
static riCommandQueue rhioQueue         = nullptr;
static riCommandList  rhioCommandList   = nullptr;
static riSwapchain    rhioSwapchain     = nullptr;
static int            framebufferWidth  = 0;
static int            framebufferHeight = 0;

//----------------------------------------------------------------------------------
// Module Data Definition
//----------------------------------------------------------------------------------

const ExampleInfo exampleInfo = {
    .screenWidth  = 800,
    .screenHeight = 450,
    .appName      = "rhio example - triangle",
};

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------

static bool CreateSwapchain( int width, int height );
static void DestroySwapchain();
#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
static riStatus PresentSwapchain( riSwapchain swapchain, void * userData );
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

bool
InitExample()
{
    riStatus status = RI_ERROR_UNKNOWN;

    // Device Configuration
    //----------------------------------------------------------
    // Backend is selected by the example build flags.
    riDeviceInfo deviceInfo = {};
    deviceInfo.base.appName = exampleInfo.appName;
    RI_FLAG_SET( deviceInfo.flags, RI_DEVICE_FLAG_DEBUG );

    // Backend Selection
    //----------------------------------------------------------
#if defined( RHIO_EXAMPLE_BACKEND_VULKAN )
    deviceInfo.backend = RI_BACKEND_VULKAN;
#elif defined( RHIO_EXAMPLE_OPENGL_ES_VERSION )
    deviceInfo.backend = RI_BACKEND_OPENGLES;
#else
    deviceInfo.backend = RI_BACKEND_OPENGL;
#endif

    // RHIO Setup
    //----------------------------------------------------------
    status = rhioCreateDevice( &deviceInfo, &rhioDevice );
    if( RI_FAILED( status ) ) return false;

    status = rhioCreateCommandQueue( rhioDevice, &rhioQueue );
    if( RI_FAILED( status ) )
        {
            ShutdownExample();
            return false;
        }

    status = rhioCreateCommandList( rhioQueue, &rhioCommandList );
    if( RI_FAILED( status ) )
        {
            ShutdownExample();
            return false;
        }

    if( !CreateSwapchain( exampleInfo.screenWidth, exampleInfo.screenHeight ) )
        {
            ShutdownExample();
            return false;
        }

    return true;
}

void
DrawExample()
{
    riTexture     displayTexture = nullptr;
    riTextureView displayView    = nullptr;
    riRenderPass  renderPass     = nullptr;
    riStatus      status         = RI_ERROR_UNKNOWN;

    riTextureViewInfo viewInfo   = {};
    riRenderPassInfo  passInfo   = {};

    // Frame Validation
    //----------------------------------------------------------
    if( rhioSwapchain == nullptr || rhioCommandList == nullptr || rhioQueue == nullptr ) return;
    if( framebufferWidth <= 0 || framebufferHeight <= 0 ) return;

    // Display Target Acquire
    //----------------------------------------------------------
    status = rhioSwapchainGetCurrentTexture( rhioSwapchain, &displayTexture );
    if( RI_FAILED( status ) ) return;

    // Display View Creation
    //----------------------------------------------------------
    viewInfo.texture    = displayTexture;
    viewInfo.usage      = RI_TEXTURE_USAGE_RENDER_TARGET;
    viewInfo.dimensions = RI_TEXTURE_DIMENSIONS_2D;

    status              = rhioCreateTextureView( rhioDevice, &viewInfo, &displayView );
    if( RI_FAILED( status ) ) return;

    // Command Recording
    //----------------------------------------------------------
    status = rhioCommandListReset( rhioCommandList );
    if( RI_FAILED( status ) ) goto cleanup;

    status = rhioCommandListBegin( rhioCommandList );
    if( RI_FAILED( status ) ) goto cleanup;

    passInfo.colorAttachments[0].textureView   = displayView;
    passInfo.colorAttachments[0].loadAction    = RI_ATTACHMENT_LOAD_ACTION_CLEAR;
    passInfo.colorAttachments[0].storeAction   = RI_ATTACHMENT_STORE_ACTION_STORE;
    passInfo.colorAttachments[0].clearColor[0] = 0.16f;
    passInfo.colorAttachments[0].clearColor[1] = 0.16f;
    passInfo.colorAttachments[0].clearColor[2] = 0.16f;
    passInfo.colorAttachments[0].clearColor[3] = 1.0f;
    passInfo.colorAttachmentCount              = 1u;
    passInfo.renderWidth                       = (riU32)framebufferWidth;
    passInfo.renderHeight                      = (riU32)framebufferHeight;

    status = rhioCommandListBeginRenderPass( rhioCommandList, &passInfo, &renderPass );
    if( RI_FAILED( status ) ) goto cleanup;

    status     = rhioRenderPassEnd( renderPass );
    renderPass = nullptr;
    if( RI_FAILED( status ) ) goto cleanup;

    status = rhioCommandListEnd( rhioCommandList );
    if( RI_FAILED( status ) ) goto cleanup;

    // Submit / Present
    //----------------------------------------------------------
    status = rhioCommandQueueSubmit( rhioQueue, rhioCommandList );
    if( RI_FAILED( status ) ) goto cleanup;

    status = rhioSwapchainPresent( rhioSwapchain );
    if( RI_FAILED( status ) ) goto cleanup;

cleanup:
    // Frame Resource Cleanup
    //----------------------------------------------------------
    if( renderPass != nullptr )
        {
            rhioRenderPassEnd( renderPass );
        }

    if( displayView != nullptr )
        {
            rhioDestroyTextureView( displayView );
        }
}

void
OnFramebufferResize( int width, int height )
{
    if( width <= 0 || height <= 0 )
        {
            framebufferWidth  = width;
            framebufferHeight = height;
            return;
        }

    if( rhioDevice != nullptr && rhioQueue != nullptr )
        {
            CreateSwapchain( width, height );
        }
    else
        {
            framebufferWidth  = width;
            framebufferHeight = height;
        }
}

void
OnKey( [[maybe_unused]] int key, [[maybe_unused]] int action )
{
}

void
ShutdownExample()
{
    DestroySwapchain();

    if( rhioCommandList != nullptr )
        {
            rhioDestroyCommandList( rhioCommandList );
            rhioCommandList = nullptr;
        }

    if( rhioQueue != nullptr )
        {
            rhioDestroyCommandQueue( rhioQueue );
            rhioQueue = nullptr;
        }

    if( rhioDevice != nullptr )
        {
            rhioDestroyDevice( rhioDevice );
            rhioDevice = nullptr;
        }
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

static bool
CreateSwapchain( int width, int height )
{
    riSwapchainInfo swapchainInfo = {};
    riStatus        status        = RI_ERROR_UNKNOWN;

    DestroySwapchain();

    framebufferWidth                = width;
    framebufferHeight               = height;

    swapchainInfo.handleType        = RHIO_SWAPCHAIN_HANDLE_TYPE_NONE;
    swapchainInfo.width             = (riU32)width;
    swapchainInfo.height            = (riU32)height;
    swapchainInfo.format            = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    swapchainInfo.maxFramesInFlight = 2u;

#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
    swapchainInfo.presentCallback = PresentSwapchain;
#endif

    status = rhioCreateSwapchain( rhioDevice, rhioQueue, &swapchainInfo, &rhioSwapchain );

    return RI_SUCCEEDED( status );
}

static void
DestroySwapchain()
{
    if( rhioSwapchain != nullptr )
        {
            rhioDestroySwapchain( rhioSwapchain );
            rhioSwapchain = nullptr;
        }
}

#if defined( RHIO_EXAMPLE_BACKEND_OPENGL )
static riStatus
PresentSwapchain( riSwapchain swapchain, void * userData )
{
    UNUSED( swapchain );
    UNUSED( userData );

    PresentWindow();

    return RI_SUCCESS;
}
#endif
