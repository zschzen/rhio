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
#include "window.hpp"

//----------------------------------------------------------------------------------
// Module Internal State
//----------------------------------------------------------------------------------

static riDevice         rhioDevice         = nullptr;
static riCommandQueue   rhioQueue          = nullptr;
static riCommandList    rhioCommandList    = nullptr;
static riSwapchain      rhioSwapchain      = nullptr;
static riBuffer         rhioTriangleBuffer = nullptr;
static riRenderPipeline rhioPipeline       = nullptr;
static int              framebufferWidth   = 0;
static int              framebufferHeight  = 0;

typedef struct TriangleVertex
{
    float position[4];
    float color[4];

} TriangleVertex;

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

static bool     CreateSwapchain( int width, int height );
static void     DestroySwapchain();
static bool     CreateTriangleResources();
static void     DestroyTriangleResources();
static riStatus PresentSwapchain( riSwapchain swapchain, void * userData );

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

    if( !CreateTriangleResources() )
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
    riTexture         displayTexture = nullptr;
    riTextureView     displayView    = nullptr;
    riRenderPass      renderPass     = nullptr;
    riStatus          status         = RI_ERROR_UNKNOWN;
    riTextureViewInfo viewInfo       = {};
    riRenderPassInfo  passInfo       = {};

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
#define CHECK( call )                                                                                                  \
    do                                                                                                                 \
        {                                                                                                              \
            status = ( call );                                                                                         \
            if( RI_FAILED( status ) ) goto cleanup;                                                                    \
        }                                                                                                              \
    while( 0 )

    CHECK( rhioCommandListReset( rhioCommandList ) );
    CHECK( rhioCommandListBegin( rhioCommandList ) );

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

    CHECK( rhioCommandListBeginRenderPass( rhioCommandList, &passInfo, &renderPass ) );
    CHECK( rhioRenderPassSetViewport( renderPass, 0u, 0u, (riU32)framebufferWidth, (riU32)framebufferHeight ) );
    CHECK( rhioRenderPassSetPipeline( renderPass, rhioPipeline ) );
    CHECK( rhioRenderPassSetShaderBuffer( renderPass, 0u, rhioTriangleBuffer, 0u ) );
    CHECK( rhioRenderPassDraw( renderPass, 3u, 1u, 0u, 0u ) );

    status     = rhioRenderPassEnd( renderPass );
    renderPass = nullptr;
    if( RI_FAILED( status ) ) goto cleanup;

    CHECK( rhioCommandListEnd( rhioCommandList ) );

    // Submit / Present
    //----------------------------------------------------------
    CHECK( rhioCommandQueueSubmit( rhioQueue, rhioCommandList ) );
    CHECK( rhioSwapchainPresent( rhioSwapchain ) );

#undef CHECK

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
    DestroyTriangleResources();

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
    swapchainInfo.presentCallback   = PresentSwapchain;

    status                          = rhioCreateSwapchain( rhioDevice, rhioQueue, &swapchainInfo, &rhioSwapchain );

    return RI_SUCCEEDED( status );
}

static bool
CreateTriangleResources()
{
    static const TriangleVertex vertices[3] = {
        {{ -0.8f, -0.8f, 0.0f, 1.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }},
        { { 0.8f, -0.8f, 0.0f, 1.0f }, { 0.1f, 0.9f, 0.2f, 1.0f }},
        {  { 0.0f, 0.8f, 0.0f, 1.0f }, { 0.1f, 0.3f, 1.0f, 1.0f }},
    };

    riBufferInfo         bufferInfo   = {};
    riRenderPipelineInfo pipelineInfo = {};
    riStatus             status       = RI_ERROR_UNKNOWN;

    DestroyTriangleResources();

    RHIO_STATIC_ASSERT( sizeof( TriangleVertex ) == sizeof( float ) * 8u,
                        "TriangleVertex must match GL std140 pull layout" );

    bufferInfo.size  = (riSize)sizeof( vertices );
    bufferInfo.usage = RI_BUFFER_USAGE_SHADER_READ;
    bufferInfo.data  = vertices;
    bufferInfo.name  = "triangle shader buffer";

    status           = rhioCreateBuffer( rhioDevice, &bufferInfo, &rhioTriangleBuffer );
    if( RI_FAILED( status ) ) return false;

    pipelineInfo.fillMode               = RI_PIPELINE_FILL_MODE_SOLID;
    pipelineInfo.cullMode               = RI_PIPELINE_CULL_MODE_NONE;
    pipelineInfo.frontFace              = RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineInfo.topology               = RI_PIPELINE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.renderTargetCount      = 1u;
    pipelineInfo.renderTargetFormats[0] = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    pipelineInfo.name                   = "triangle passthrough pipeline";

    status                              = rhioCreateRenderPipeline( rhioDevice, &pipelineInfo, &rhioPipeline );
    if( RI_FAILED( status ) )
        {
            DestroyTriangleResources();
            return false;
        }

    return true;
}

static void
DestroyTriangleResources()
{
    if( rhioPipeline != nullptr )
        {
            rhioDestroyRenderPipeline( rhioPipeline );
            rhioPipeline = nullptr;
        }

    if( rhioTriangleBuffer != nullptr )
        {
            rhioDestroyBuffer( rhioTriangleBuffer );
            rhioTriangleBuffer = nullptr;
        }
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

static riStatus
PresentSwapchain( riSwapchain swapchain, void * userData )
{
    UNUSED( swapchain );
    UNUSED( userData );

    PresentWindow();

    return RI_SUCCESS;
}
