/*******************************************************************************************
 *
 *   rhio benchmark suite - full frame path (windowed)
 *
 *   Measures the complete CPU-side frame cycle (acquire, view, record, submit,
 *   present) against a real window. Requires a display; skips gracefully
 *   without one. The RI_SWAPCHAIN_PRESENT_FLAG_NO_VSYNC flag keeps the present
 *   unthrottled so the number reflects CPU work, not the monitor refresh.
 *
 *   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
 *
 *******************************************************************************************/

#include "rhio.h"

#include <benchmark/benchmark.h>

#if !defined( GLFW_INCLUDE_NONE )
#    define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#if defined( RHIO_BACKEND_VULKAN )
#    if defined( _WIN32 )
#        define GLFW_EXPOSE_NATIVE_WIN32
#    else
#        define GLFW_EXPOSE_NATIVE_X11
#    endif
#    include <GLFW/glfw3native.h>
#endif

#include <cstdio>
#include <cstring>
#include <vector>

#if defined( RHIO_BACKEND_VULKAN )
#    define RHIO_BENCH_BACKEND RI_BACKEND_VULKAN
#elif defined( RHIO_BACKEND_OPENGL )
#    define RHIO_BENCH_BACKEND RI_BACKEND_OPENGL
#else
#    error "frame bench requires a backend define"
#endif

typedef struct FrameVertex
{
    float position[4];
    float color[4];

} FrameVertex;

static const FrameVertex s_frameVertices[3] = {
    {{ -0.8f, -0.8f, 0.0f, 1.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }},
    {{  0.8f, -0.8f, 0.0f, 1.0f }, { 0.1f, 0.9f, 0.2f, 1.0f }},
    {{  0.0f,  0.8f, 0.0f, 1.0f }, { 0.1f, 0.3f, 1.0f, 1.0f }},
};

// GL hosts perform the buffer swap through the present callback
static riStatus
SwapBuffersCallback( riSwapchain swapchain, void * userData )
{
    (void)swapchain;
    glfwSwapBuffers( ( GLFWwindow * )userData );
    return RI_SUCCESS;
}

// Shared window + device + swapchain fixture for the full frame path
class FrameFixture : public ::benchmark::Fixture
{
public:
    void
    SetUp( const ::benchmark::State & ) override
    {
        if( device != nullptr )
            {
                return;
            }

        if( glfwInit() == GLFW_FALSE )
            {
                state_ = "glfwInit failed";
                return;
            }

        glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
#if defined( RHIO_BACKEND_VULKAN )
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        if( glfwVulkanSupported() == GLFW_FALSE )
            {
                state_ = "no Vulkan support";
                return;
            }
#else
        glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_API );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#endif

        window = glfwCreateWindow( 320, 240, "rhio bench frame", nullptr, nullptr );
        if( window == nullptr )
            {
                state_ = "window creation failed";
                return;
            }

#if defined( RHIO_BACKEND_OPENGL )
        glfwMakeContextCurrent( window );
        glfwSwapInterval( 0 );
#endif

        riDeviceInfo info         = { };
        info.base.appName         = "rhio frame benchmark";
        info.backend              = RHIO_BENCH_BACKEND;

#if defined( RHIO_BACKEND_VULKAN )
        // Instance Extensions and Native Surface
        //----------------------------------------------------------
        uint32_t    count       = 0;
        const char ** extensions = glfwGetRequiredInstanceExtensions( &count );
        if( extensions == nullptr || count == 0u )
            {
                state_ = "no GLFW instance extensions";
                return;
            }

        // GLFW may report VK_KHR_xcb_surface only; rhio surfaces the X11
        // handle through VK_KHR_xlib_surface
        instanceExtensions.assign( extensions, extensions + count );
#    if defined( __linux__ )
        {
            const char * xlib = "VK_KHR_xlib_surface";
            bool present = false;
            for( const char * extension : instanceExtensions )
                {
                    if( std::strcmp( extension, xlib ) == 0 ) present = true;
                }
            if( !present && glfwGetX11Display() != nullptr ) instanceExtensions.push_back( xlib );
        }
#    endif
        if( instanceExtensions.empty() )
            {
                state_ = "no instance extensions";
                return;
            }

        info.base.instance.extensions     = instanceExtensions.data();
        info.base.instance.extensionCount = (riU32)instanceExtensions.size();
#endif

        if( RI_FAILED( rhioCreateDevice( &info, &device ) ) )
            {
                state_ = "device creation failed";
                return;
            }

        rhioCreateCommandQueue( device, &queue );
        rhioCreateCommandList( queue, &list );

        riBufferInfo bufferInfo  = { };
        bufferInfo.size          = sizeof( s_frameVertices );
        bufferInfo.usage         = RI_BUFFER_USAGE_SHADER_READ;
        bufferInfo.data          = s_frameVertices;
        rhioCreateBuffer( device, &bufferInfo, &buffer );

        riRenderPipelineInfo pipelineInfo  = { };
        pipelineInfo.cullMode              = RI_PIPELINE_CULL_MODE_NONE;
        pipelineInfo.frontFace             = RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineInfo.topology              = RI_PIPELINE_TOPOLOGY_TRIANGLE_LIST;
        pipelineInfo.fillMode              = RI_PIPELINE_FILL_MODE_SOLID;
        pipelineInfo.renderTargetCount     = 1u;
        pipelineInfo.renderTargetFormats[0] = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        rhioCreateRenderPipeline( device, &pipelineInfo, &pipeline );

        riSwapchainInfo swapchainInfo  = { };
        swapchainInfo.width            = 320u;
        swapchainInfo.height           = 240u;
        swapchainInfo.maxFramesInFlight = 2u;
        // Unthrottled present keeps the measurement CPU-bound
        RI_FLAG_SET( swapchainInfo.presentFlags, RI_SWAPCHAIN_PRESENT_FLAG_NO_VSYNC );

#if defined( RHIO_BACKEND_VULKAN )
#    if defined( _WIN32 )
        swapchainInfo.handleType             = RHIO_SWAPCHAIN_HANDLE_TYPE_HWND;
        swapchainInfo.handle.hwnd.instance   = GetModuleHandle( nullptr );
        swapchainInfo.handle.hwnd.window     = glfwGetWin32Window( window );
#    elif defined( __linux__ )
        swapchainInfo.handleType         = RHIO_SWAPCHAIN_HANDLE_TYPE_X11;
        swapchainInfo.handle.x11.display = glfwGetX11Display();
        swapchainInfo.handle.x11.window  = (riUPtr)glfwGetX11Window( window );
#    else
        state_ = "unsupported platform";
        return;
#    endif
#else
        swapchainInfo.handleType       = RHIO_SWAPCHAIN_HANDLE_TYPE_NONE;
        swapchainInfo.presentCallback  = SwapBuffersCallback;
        swapchainInfo.presentUserData  = window;
#endif

        if( RI_FAILED( rhioCreateSwapchain( device, queue, &swapchainInfo, &swapchain ) ) )
            {
                state_ = "swapchain creation failed";
                return;
            }
    }

    void
    TearDown( const ::benchmark::State & ) override
    {
        rhioDestroySwapchain( swapchain );
        swapchain = nullptr;

        rhioDestroyRenderPipeline( pipeline );
        pipeline = nullptr;

        rhioDestroyBuffer( buffer );
        buffer = nullptr;

        rhioDestroyCommandList( list );
        list = nullptr;

        rhioDestroyCommandQueue( queue );
        queue = nullptr;

        rhioDestroyDevice( device );
        device = nullptr;

        if( window != nullptr )
            {
                glfwDestroyWindow( window );
                window = nullptr;
            }
        glfwTerminate();
    }

    const char *
    Failure( void ) const
    {
        return state_;
    }

    riDevice       device    = nullptr;
    riCommandQueue queue     = nullptr;
    riCommandList  list      = nullptr;
    riBuffer       buffer    = nullptr;
    riRenderPipeline pipeline = nullptr;
    riSwapchain    swapchain = nullptr;
    GLFWwindow *   window    = nullptr;
    const char *   state_    = nullptr;
    std::vector< const char * > instanceExtensions;
};

// Full frame cycle: acquire, view, record, submit, present (CPU side)
BENCHMARK_F( FrameFixture, FrameRecordPresent )
( ::benchmark::State & state )
{
    const char * failure = Failure();
    if( failure != nullptr )
        {
            state.SkipWithError( failure );
            return;
        }

    for( auto _ : state )
        {
            riTexture        backbuffer = nullptr;
            riTextureView    view       = nullptr;
            riRenderPass     pass       = nullptr;
            riTextureViewInfo viewInfo  = { };
            riRenderPassInfo  passInfo  = { };
            riTextureInfo    texInfo    = { };
            riStatus         status     = RI_ERROR_UNKNOWN;

            status = rhioSwapchainGetCurrentTexture( swapchain, &backbuffer );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "acquire failed" );
                    return;
                }

            rhioGetTextureInfo( backbuffer, &texInfo );

            viewInfo.texture    = backbuffer;
            viewInfo.usage      = RI_TEXTURE_USAGE_RENDER_TARGET;
            viewInfo.dimensions = RI_TEXTURE_DIMENSIONS_2D;
            status              = rhioCreateTextureView( device, &viewInfo, &view );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "view creation failed" );
                    return;
                }

            status = rhioCommandListReset( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list reset failed" );
                    return;
                }

            status = rhioCommandListBegin( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list begin failed" );
                    return;
                }

            passInfo.colorAttachments[0].textureView   = view;
            passInfo.colorAttachments[0].loadAction    = RI_ATTACHMENT_LOAD_ACTION_CLEAR;
            passInfo.colorAttachments[0].storeAction   = RI_ATTACHMENT_STORE_ACTION_STORE;
            passInfo.colorAttachments[0].clearColor[0] = 0.16f;
            passInfo.colorAttachments[0].clearColor[1] = 0.16f;
            passInfo.colorAttachments[0].clearColor[2] = 0.16f;
            passInfo.colorAttachments[0].clearColor[3] = 1.0f;
            passInfo.colorAttachmentCount              = 1u;
            passInfo.renderWidth                       = texInfo.width;
            passInfo.renderHeight                      = texInfo.height;

            status = rhioCommandListBeginRenderPass( list, &passInfo, &pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass begin failed" );
                    return;
                }

            status = rhioRenderPassSetViewport( pass, 0u, 0u, texInfo.width, texInfo.height );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "viewport failed" );
                    return;
                }

            status = rhioRenderPassSetPipeline( pass, pipeline );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pipeline bind failed" );
                    return;
                }

            status = rhioRenderPassSetShaderBuffer( pass, 0u, buffer, 0u );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "shader buffer bind failed" );
                    return;
                }

            status = rhioRenderPassDraw( pass, 3u, 1u, 0u, 0u );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "draw failed" );
                    return;
                }

            status = rhioRenderPassEnd( pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass end failed" );
                    return;
                }

            status = rhioCommandListEnd( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list end failed" );
                    return;
                }

            status = rhioCommandQueueSubmit( queue, list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "submit failed" );
                    return;
                }

            status = rhioSwapchainPresent( swapchain );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "present failed" );
                    return;
                }

            rhioDestroyTextureView( view );
        }
}

// Record-path only: frame armed once, recording looped without submit or
// present. Isolates exactly what the record-side optimizations target
BENCHMARK_F( FrameFixture, FrameRecordOnly )
( ::benchmark::State & state )
{
    const char * failure = Failure();
    if( failure != nullptr )
        {
            state.SkipWithError( failure );
            return;
        }

    riTexture     backbuffer = nullptr;
    riTextureView view       = nullptr;
    riTextureViewInfo viewInfo = { };
    riRenderPassInfo passInfo = { };
    riTextureInfo texInfo    = { };
    riStatus      status     = rhioSwapchainGetCurrentTexture( swapchain, &backbuffer );
    if( RI_FAILED( status ) )
        {
            state.SkipWithError( "acquire failed" );
            return;
        }

    rhioGetTextureInfo( backbuffer, &texInfo );
    viewInfo.texture    = backbuffer;
    viewInfo.usage      = RI_TEXTURE_USAGE_RENDER_TARGET;
    viewInfo.dimensions = RI_TEXTURE_DIMENSIONS_2D;
    status              = rhioCreateTextureView( device, &viewInfo, &view );
    if( RI_FAILED( status ) )
        {
            state.SkipWithError( "view creation failed" );
            return;
        }

    passInfo.colorAttachments[0].textureView   = view;
    passInfo.colorAttachments[0].loadAction    = RI_ATTACHMENT_LOAD_ACTION_CLEAR;
    passInfo.colorAttachments[0].storeAction   = RI_ATTACHMENT_STORE_ACTION_STORE;
    passInfo.colorAttachments[0].clearColor[0] = 0.16f;
    passInfo.colorAttachments[0].clearColor[1] = 0.16f;
    passInfo.colorAttachments[0].clearColor[2] = 0.16f;
    passInfo.colorAttachments[0].clearColor[3] = 1.0f;
    passInfo.colorAttachmentCount              = 1u;
    passInfo.renderWidth                       = texInfo.width;
    passInfo.renderHeight                      = texInfo.height;

    for( auto _ : state )
        {
            riRenderPass pass   = nullptr;
            status              = rhioCommandListReset( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list reset failed" );
                    return;
                }

            status = rhioCommandListBegin( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list begin failed" );
                    return;
                }

            status = rhioCommandListBeginRenderPass( list, &passInfo, &pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass begin failed" );
                    return;
                }

            status = rhioRenderPassSetViewport( pass, 0u, 0u, texInfo.width, texInfo.height );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "viewport failed" );
                    return;
                }

            status = rhioRenderPassSetPipeline( pass, pipeline );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pipeline bind failed" );
                    return;
                }

            status = rhioRenderPassSetShaderBuffer( pass, 0u, buffer, 0u );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "shader buffer bind failed" );
                    return;
                }

            status = rhioRenderPassDraw( pass, 3u, 1u, 0u, 0u );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "draw failed" );
                    return;
                }

            status = rhioRenderPassEnd( pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass end failed" );
                    return;
                }

            status = rhioCommandListEnd( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list end failed" );
                    return;
                }
        }

    rhioDestroyTextureView( view );
}

// Record-path with a forced descriptor rewrite every iteration (alternating
// offsets). The hit-vs-miss delta of FrameRecordOnly quantifies the cost the
// binding cache skips when the app keeps the same buffer
BENCHMARK_F( FrameFixture, FrameRecordOnlyRewrite )
( ::benchmark::State & state )
{
    const char * failure = Failure();
    if( failure != nullptr )
        {
            state.SkipWithError( failure );
            return;
        }

    riTexture     backbuffer = nullptr;
    riTextureView view       = nullptr;
    riTextureViewInfo viewInfo = { };
    riRenderPassInfo passInfo = { };
    riTextureInfo texInfo    = { };
    riStatus      status     = rhioSwapchainGetCurrentTexture( swapchain, &backbuffer );
    if( RI_FAILED( status ) )
        {
            state.SkipWithError( "acquire failed" );
            return;
        }

    rhioGetTextureInfo( backbuffer, &texInfo );
    viewInfo.texture    = backbuffer;
    viewInfo.usage      = RI_TEXTURE_USAGE_RENDER_TARGET;
    viewInfo.dimensions = RI_TEXTURE_DIMENSIONS_2D;
    status              = rhioCreateTextureView( device, &viewInfo, &view );
    if( RI_FAILED( status ) )
        {
            state.SkipWithError( "view creation failed" );
            return;
        }

    passInfo.colorAttachments[0].textureView   = view;
    passInfo.colorAttachments[0].loadAction    = RI_ATTACHMENT_LOAD_ACTION_CLEAR;
    passInfo.colorAttachments[0].storeAction   = RI_ATTACHMENT_STORE_ACTION_STORE;
    passInfo.colorAttachments[0].clearColor[0] = 0.16f;
    passInfo.colorAttachments[0].clearColor[1] = 0.16f;
    passInfo.colorAttachments[0].clearColor[2] = 0.16f;
    passInfo.colorAttachments[0].clearColor[3] = 1.0f;
    passInfo.colorAttachmentCount              = 1u;
    passInfo.renderWidth                       = texInfo.width;
    passInfo.renderHeight                      = texInfo.height;

    for( auto _ : state )
        {
            riRenderPass pass = nullptr;
            riU32 offset = ( state.iterations() & 1u ) != 0u ? 64u : 0u;

            status = rhioCommandListReset( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list reset failed" );
                    return;
                }

            status = rhioCommandListBegin( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list begin failed" );
                    return;
                }

            status = rhioCommandListBeginRenderPass( list, &passInfo, &pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass begin failed" );
                    return;
                }

            status = rhioRenderPassSetViewport( pass, 0u, 0u, texInfo.width, texInfo.height );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "viewport failed" );
                    return;
                }

            status = rhioRenderPassSetPipeline( pass, pipeline );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pipeline bind failed" );
                    return;
                }

            status = rhioRenderPassSetShaderBuffer( pass, 0u, buffer, offset );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "shader buffer bind failed" );
                    return;
                }

            status = rhioRenderPassDraw( pass, 3u, 1u, 0u, 0u );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "draw failed" );
                    return;
                }

            status = rhioRenderPassEnd( pass );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "pass end failed" );
                    return;
                }

            status = rhioCommandListEnd( list );
            if( RI_FAILED( status ) )
                {
                    state.SkipWithError( "list end failed" );
                    return;
                }
        }

    rhioDestroyTextureView( view );
}

BENCHMARK_MAIN();
