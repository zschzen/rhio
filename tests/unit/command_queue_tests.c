// ============================================================================
// rhio - command queue unit tests
// ============================================================================
// Validates the backend-neutral command queue wrapper and base-style dispatch.
// ============================================================================

#include "rhio.h"

#include <rktest/rktest.h>
#include <stdint.h>

//----------------------------------------------------------------------------------
// Custom Backend Test Double
//----------------------------------------------------------------------------------

typedef struct CommandQueueObservations
{
    int deviceInitCalls;
    int deviceShutdownCalls;
    int queueCreateCalls;
    int queueCreateFailureCalls;
    int queueDestroyCalls;
    int queueSubmitCalls;

} CommandQueueObservations;

static CommandQueueObservations g_commandQueue;

static void
reset_command_queue_observations( void )
{
    static const CommandQueueObservations emptyObservations = { 0 };

    g_commandQueue                                          = emptyObservations;
}

static riStatus
command_queue_backend_init( void * backendDevice, const riBackendInitInfo * info )
{
    UNUSED( backendDevice );
    UNUSED( info );

    ++g_commandQueue.deviceInitCalls;

    return RI_SUCCESS;
}

static void
command_queue_backend_shutdown( void * backendDevice )
{
    UNUSED( backendDevice );

    ++g_commandQueue.deviceShutdownCalls;
}

static riStatus
command_queue_create( void * backendDevice, riCommandQueue queue )
{
    UNUSED( backendDevice );
    UNUSED( queue );

    ++g_commandQueue.queueCreateCalls;

    return RI_SUCCESS;
}

static riStatus
command_queue_create_failure( void * backendDevice, riCommandQueue queue )
{
    UNUSED( backendDevice );
    UNUSED( queue );

    ++g_commandQueue.queueCreateFailureCalls;

    return RI_ERROR_BACKEND_INIT;
}

static void
command_queue_destroy( riCommandQueue queue )
{
    UNUSED( queue );

    ++g_commandQueue.queueDestroyCalls;
}

static riStatus
command_queue_submit( riCommandQueue queue, riCommandList commandList )
{
    UNUSED( queue );
    UNUSED( commandList );

    ++g_commandQueue.queueSubmitCalls;

    return RI_SUCCESS;
}

//----------------------------------------------------------------------------------
// Command Queue Argument Validation
//----------------------------------------------------------------------------------

// Rejects missing input/output pointers and clears stale output handles
TEST( command_queue, rejects_null_create_args )
{
    riCommandQueue queue = (riCommandQueue)(uintptr_t)1;

    riSetTraceLogLevel( RI_LOG_NONE );

    EXPECT_EQ( rhioCreateCommandQueue( NULL, NULL ), RI_ERROR_INVALID_PARAM );

    EXPECT_EQ( rhioCreateCommandQueue( NULL, &queue ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( queue, NULL );
}

// Rejects queue creation if the owning device has no queue dispatch table
TEST( command_queue, rejects_missing_queue_vtable )
{
    riDevice       device = NULL;
    riCommandQueue queue  = (riCommandQueue)(uintptr_t)1;
    riDeviceInfo   info   = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable.init                 = command_queue_backend_init;
    info.vtable.shutdown             = command_queue_backend_shutdown;
    info.vtable.create_command_queue = command_queue_create;

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );
    EXPECT_EQ( g_commandQueue.deviceInitCalls, 1 );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_ERROR_INVALID_STATE );
    EXPECT_PTR_EQ( queue, NULL );

    rhioDestroyDevice( device );
}

//----------------------------------------------------------------------------------
// Command Queue Dispatch
//----------------------------------------------------------------------------------

// Verifies command queue creation dispatches through the device vtable, then stores
// the custom queue vtable for later queue operations
TEST( command_queue, dispatches_custom_queue_vtable )
{
    riDevice       device      = NULL;
    riCommandQueue queue       = NULL;
    riCommandList  commandList = (riCommandList)(uintptr_t)1;
    riDeviceInfo   info        = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable.init                              = command_queue_backend_init;
    info.vtable.shutdown                          = command_queue_backend_shutdown;
    info.vtable.create_command_queue              = command_queue_create;

    info.commandQueueVTable.destroy_command_queue = command_queue_destroy;
    info.commandQueueVTable.submit_command_list   = command_queue_submit;

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );
    EXPECT_EQ( g_commandQueue.deviceInitCalls, 1 );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_SUCCESS );
    EXPECT_PTR_NE( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateCalls, 1 );

    EXPECT_EQ( rhioCommandQueueSubmit( queue, commandList ), RI_SUCCESS );
    EXPECT_EQ( g_commandQueue.queueSubmitCalls, 1 );

    rhioDestroyCommandQueue( queue );
    EXPECT_EQ( g_commandQueue.queueDestroyCalls, 1 );

    rhioDestroyDevice( device );
    EXPECT_EQ( g_commandQueue.deviceShutdownCalls, 1 );
}

// Leaves the output handle null if the backend rejects command queue creation
TEST( command_queue, rejects_backend_create_failure )
{
    riDevice       device = NULL;
    riCommandQueue queue  = (riCommandQueue)(uintptr_t)1;
    riDeviceInfo   info   = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable.init                              = command_queue_backend_init;
    info.vtable.shutdown                          = command_queue_backend_shutdown;
    info.vtable.create_command_queue              = command_queue_create_failure;

    info.commandQueueVTable.destroy_command_queue = command_queue_destroy;
    info.commandQueueVTable.submit_command_list   = command_queue_submit;

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_ERROR_BACKEND_INIT );
    EXPECT_PTR_EQ( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateFailureCalls, 1 );
    EXPECT_EQ( g_commandQueue.queueDestroyCalls, 0 );

    rhioDestroyDevice( device );
    EXPECT_EQ( g_commandQueue.deviceShutdownCalls, 1 );
}
