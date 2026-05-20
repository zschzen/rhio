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
    int commandListCreateCalls;

} CommandQueueObservations;

typedef struct CommandQueueTestDevice
{
    riDeviceBase base; // Must be first for frontend dispatch
} CommandQueueTestDevice;

typedef struct CommandQueueTestQueue
{
    riCommandQueueBase base;   // Must be first for frontend dispatch
    riDevice           device; // Owning device
} CommandQueueTestQueue;

static CommandQueueObservations g_commandQueue;

static const riDeviceVTable       s_command_queue_device_vtable;
static const riDeviceVTable       s_command_queue_missing_queue_vtable;
static const riDeviceVTable       s_command_queue_missing_command_list_vtable;
static const riDeviceVTable       s_command_queue_failing_device_vtable;
static const riCommandQueueVTable s_command_queue_vtable;
static const riCommandQueueVTable s_command_queue_no_command_list_vtable;

static void
reset_command_queue_observations( void )
{
    static const CommandQueueObservations emptyObservations = { 0 };

    g_commandQueue                                          = emptyObservations;
}

static riStatus
command_queue_backend_init( riDevice device, const riBackendInitInfo * info )
{
    UNUSED( device );
    UNUSED( info );

    ++g_commandQueue.deviceInitCalls;

    return RI_SUCCESS;
}

static void
command_queue_backend_shutdown( riDevice device )
{
    UNUSED( device );

    ++g_commandQueue.deviceShutdownCalls;
}

static riStatus
command_queue_create( riDevice device, riCommandQueue * outQueue )
{
    CommandQueueTestQueue * queue = NULL;

    // Input validation
    // ----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outQueue, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.queueCreateCalls;

    // Queue allocation
    // ----------------------------------------------------------
    // Test backends allocate the complete queue object, matching the real backend contract.
    queue = (CommandQueueTestQueue *)RI_CALLOC( 1, sizeof( *queue ) );
    if( queue == NULL ) return RI_ERROR_OUT_OF_MEMORY;

    // Queue state initialization
    // ----------------------------------------------------------
    queue->base.vtable = &s_command_queue_vtable;
    queue->device      = device;

    // Queue handle handoff
    // ----------------------------------------------------------
    *outQueue = (riCommandQueue)queue;

    return RI_SUCCESS;
}

static riStatus
command_queue_create_without_vtable( riDevice device, riCommandQueue * outQueue )
{
    CommandQueueTestQueue * queue = NULL;

    // Input validation
    // ----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outQueue, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.queueCreateCalls;

    // Queue allocation
    // ----------------------------------------------------------
    // Intentionally leaves the base vtable unset so frontend validation rejects it.
    queue = (CommandQueueTestQueue *)RI_CALLOC( 1, sizeof( *queue ) );
    if( queue == NULL ) return RI_ERROR_OUT_OF_MEMORY;

    // Queue state initialization
    // ----------------------------------------------------------
    queue->device = device;

    // Queue handle handoff
    // ----------------------------------------------------------
    *outQueue = (riCommandQueue)queue;

    return RI_SUCCESS;
}

static riStatus
command_queue_create_without_command_list_create( riDevice device, riCommandQueue * outQueue )
{
    CommandQueueTestQueue * queue = NULL;

    // Input validation
    // ----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outQueue, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.queueCreateCalls;

    // Queue allocation
    // ----------------------------------------------------------
    // This queue intentionally omits command-list creation from its dispatch table.
    queue = (CommandQueueTestQueue *)RI_CALLOC( 1, sizeof( *queue ) );
    if( queue == NULL ) return RI_ERROR_OUT_OF_MEMORY;

    // Queue state initialization
    // ----------------------------------------------------------
    queue->base.vtable = &s_command_queue_no_command_list_vtable;
    queue->device      = device;

    // Queue handle handoff
    // ----------------------------------------------------------
    *outQueue = (riCommandQueue)queue;

    return RI_SUCCESS;
}

static riStatus
command_queue_create_failure( riDevice device, riCommandQueue * outQueue )
{
    UNUSED( device );

    // Output handle initialization
    // ----------------------------------------------------------
    if( outQueue != NULL ) *outQueue = NULL;

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.queueCreateFailureCalls;

    return RI_ERROR_BACKEND_INIT;
}

static riStatus
command_queue_create_command_list( riCommandQueue queue, riCommandList * outCommandList )
{
    // Input validation
    // ----------------------------------------------------------
    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outCommandList, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.commandListCreateCalls;

    // Command list handle handoff
    // ----------------------------------------------------------
    // This test double only needs an opaque non-null handle to prove dispatch.
    *outCommandList = (riCommandList)(uintptr_t)2;

    return RI_SUCCESS;
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

static const riDeviceVTable s_command_queue_device_vtable = {
    .init                 = command_queue_backend_init,
    .shutdown             = command_queue_backend_shutdown,
    .create_command_queue = command_queue_create,
};

static const riDeviceVTable s_command_queue_missing_queue_vtable = {
    .init                 = command_queue_backend_init,
    .shutdown             = command_queue_backend_shutdown,
    .create_command_queue = command_queue_create_without_vtable,
};

static const riDeviceVTable s_command_queue_missing_command_list_vtable = {
    .init                 = command_queue_backend_init,
    .shutdown             = command_queue_backend_shutdown,
    .create_command_queue = command_queue_create_without_command_list_create,
};

static const riDeviceVTable s_command_queue_failing_device_vtable = {
    .init                 = command_queue_backend_init,
    .shutdown             = command_queue_backend_shutdown,
    .create_command_queue = command_queue_create_failure,
};

static const riCommandQueueVTable s_command_queue_vtable = {
    .create_command_list   = command_queue_create_command_list,
    .destroy_command_queue = command_queue_destroy,
    .submit_command_list   = command_queue_submit,
};

static const riCommandQueueVTable s_command_queue_no_command_list_vtable = {
    .destroy_command_queue = command_queue_destroy,
    .submit_command_list   = command_queue_submit,
};

//----------------------------------------------------------------------------------
// Command Queue Argument Validation
//----------------------------------------------------------------------------------

// Rejects missing input/output pointers and clears stale output handles
TEST( command_queue, rejects_null_create_args )
{
    riCommandQueue queue = (riCommandQueue)(uintptr_t)1;
    riCommandList  list  = (riCommandList)(uintptr_t)1;

    riSetTraceLogLevel( RI_LOG_NONE );

    EXPECT_EQ( rhioCreateCommandQueue( NULL, NULL ), RI_ERROR_INVALID_PARAM );

    EXPECT_EQ( rhioCreateCommandQueue( NULL, &queue ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( queue, NULL );

    EXPECT_EQ( rhioCreateCommandList( NULL, NULL ), RI_ERROR_INVALID_PARAM );

    EXPECT_EQ( rhioCreateCommandList( NULL, &list ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( list, NULL );
}

// Rejects queue creation if the backend returns a queue without a dispatch table
TEST( command_queue, rejects_missing_queue_vtable )
{
    riDevice       device = NULL;
    riCommandQueue queue  = (riCommandQueue)(uintptr_t)1;
    riDeviceInfo   info   = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable            = &s_command_queue_missing_queue_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );
    EXPECT_EQ( g_commandQueue.deviceInitCalls, 1 );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_ERROR_INVALID_STATE );
    EXPECT_PTR_EQ( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateCalls, 1 );

    rhioDestroyDevice( device );
}

// Rejects queue creation if the queue dispatch table cannot create command lists
TEST( command_queue, rejects_missing_command_list_create_slot )
{
    riDevice       device = NULL;
    riCommandQueue queue  = (riCommandQueue)(uintptr_t)1;
    riDeviceInfo   info   = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable            = &s_command_queue_missing_command_list_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );
    EXPECT_EQ( g_commandQueue.deviceInitCalls, 1 );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_ERROR_INVALID_STATE );
    EXPECT_PTR_EQ( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateCalls, 1 );
    EXPECT_EQ( g_commandQueue.queueDestroyCalls, 1 );

    rhioDestroyDevice( device );
}

//----------------------------------------------------------------------------------
// Command Queue Dispatch
//----------------------------------------------------------------------------------

// Verifies command queue creation dispatches through the device vtable, then uses
// the queue base vtable for later queue operations
TEST( command_queue, dispatches_custom_queue_vtable )
{
    riDevice       device      = NULL;
    riCommandQueue queue       = NULL;
    riCommandList  commandList = NULL;
    riDeviceInfo   info        = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable            = &s_command_queue_device_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );
    EXPECT_EQ( g_commandQueue.deviceInitCalls, 1 );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_SUCCESS );
    EXPECT_PTR_NE( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateCalls, 1 );

    EXPECT_EQ( rhioCreateCommandList( queue, &commandList ), RI_SUCCESS );
    EXPECT_PTR_NE( commandList, NULL );
    EXPECT_EQ( g_commandQueue.commandListCreateCalls, 1 );

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

    info.vtable            = &s_command_queue_failing_device_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );

    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_ERROR_BACKEND_INIT );
    EXPECT_PTR_EQ( queue, NULL );
    EXPECT_EQ( g_commandQueue.queueCreateFailureCalls, 1 );
    EXPECT_EQ( g_commandQueue.queueDestroyCalls, 0 );

    rhioDestroyDevice( device );
    EXPECT_EQ( g_commandQueue.deviceShutdownCalls, 1 );
}
