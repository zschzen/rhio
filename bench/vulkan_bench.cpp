/*******************************************************************************************
 *
 *   rhio benchmark suite - Vulkan CPU-side API overhead
 *
 *   Runs headless (device without a surface): measures creation, upload,
 *   recording and dispatch costs, not GPU frame throughput.
 *
 *   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
 *
 *******************************************************************************************/

#include "rhio.h"

#include <benchmark/benchmark.h>

#include <cstdio>

//----------------------------------------------------------------------------------
// Fixture
//----------------------------------------------------------------------------------

// Shared headless device/queue/list created once per benchmark
class VulkanFixture : public ::benchmark::Fixture
{
public:
    void
    SetUp( const ::benchmark::State & ) override
    {
        if( device != nullptr )
            {
                return;
            }

        riDeviceInfo info         = { };
        info.base.appName         = "rhio-benchmark";
        info.backend              = RI_BACKEND_VULKAN;

        riStatus       status     = rhioCreateDevice( &info, &device );
        if( RI_FAILED( status ) )
            {
                std::fprintf( stderr, "benchmark: device creation failed (%d)\n", (int)status );
                std::abort();
            }

        rhioCreateCommandQueue( device, &queue );
        rhioCreateCommandList( queue, &list );

        riBufferInfo bufferInfo   = { };
        bufferInfo.size           = 64u;
        bufferInfo.usage          = RI_BUFFER_USAGE_SHADER_READ;

        rhioCreateBuffer( device, &bufferInfo, &scratch );
    }

    void
    TearDown( const ::benchmark::State & ) override
    {
        if( device == nullptr )
            {
                return;
            }

        rhioDestroyBuffer( scratch );
        scratch = nullptr;

        rhioDestroyCommandList( list );
        list = nullptr;

        rhioDestroyCommandQueue( queue );
        queue = nullptr;

        rhioDestroyDevice( device );
        device = nullptr;
    }

    riDevice       device  = nullptr;
    riCommandQueue queue   = nullptr;
    riCommandList  list    = nullptr;
    riBuffer       scratch = nullptr;
};

//----------------------------------------------------------------------------------
// Benchmarks
//----------------------------------------------------------------------------------

// Device-local storage buffer with a staging upload from host memory
BENCHMARK_F( VulkanFixture, BufferCreateUploadDestroy )
( ::benchmark::State & state )
{
    static const unsigned char payload[ 64 * 1024 ] = { };

    riBufferInfo info  = { };
    info.size          = sizeof( payload );
    info.usage         = RI_BUFFER_USAGE_SHADER_READ;
    info.data          = payload;

    for( auto _ : state )
        {
            riBuffer buffer = nullptr;
            if( RI_FAILED( rhioCreateBuffer( device, &info, &buffer ) ) )
                {
                    state.SkipWithError( "buffer creation failed" );
                    return;
                }
            rhioDestroyBuffer( buffer );
        }
}

// Builtin render pipeline over the embedded SPIR-V blobs
BENCHMARK_F( VulkanFixture, PipelineCreateDestroy )
( ::benchmark::State & state )
{
    riRenderPipelineInfo info  = { };
    info.cullMode              = RI_PIPELINE_CULL_MODE_NONE;
    info.frontFace             = RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE;
    info.topology              = RI_PIPELINE_TOPOLOGY_TRIANGLE_LIST;
    info.fillMode              = RI_PIPELINE_FILL_MODE_SOLID;
    info.renderTargetCount     = 1u;
    info.renderTargetFormats[0] = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;

    for( auto _ : state )
        {
            riRenderPipeline pipeline = nullptr;
            if( RI_FAILED( rhioCreateRenderPipeline( device, &info, &pipeline ) ) )
                {
                    state.SkipWithError( "pipeline creation failed" );
                    return;
                }
            rhioDestroyRenderPipeline( pipeline );
        }
}

// Command queue and its command pool lifetime
BENCHMARK_F( VulkanFixture, QueueCreateDestroy )
( ::benchmark::State & state )
{
    for( auto _ : state )
        {
            riCommandQueue localQueue = nullptr;
            if( RI_FAILED( rhioCreateCommandQueue( device, &localQueue ) ) )
                {
                    state.SkipWithError( "queue creation failed" );
                    return;
                }
            rhioDestroyCommandQueue( localQueue );
        }
}

// Command list reset/begin/end cycle (empty command buffer)
BENCHMARK_F( VulkanFixture, ListRecordCycle )
( ::benchmark::State & state )
{
    for( auto _ : state )
        {
            if( RI_FAILED( rhioCommandListReset( list ) )
                || RI_FAILED( rhioCommandListBegin( list ) )
                || RI_FAILED( rhioCommandListEnd( list ) ) )
                {
                    state.SkipWithError( "record cycle failed" );
                    return;
                }
        }
}

// Frontend dispatch wrapper overhead through the vtable
BENCHMARK_F( VulkanFixture, DispatchOverhead )
( ::benchmark::State & state )
{
    riBufferInfo outInfo = { };

    for( auto _ : state )
        {
            rhioGetBufferInfo( scratch, &outInfo );
        }
}

BENCHMARK_MAIN();
