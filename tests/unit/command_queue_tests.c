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
    int queueSubmitInfoCalls;
    int queueSubmitListCount;
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

typedef struct CommandQueueTestCommandList
{
    riCommandListBase base;  // Must be first for frontend dispatch
    riCommandQueue    queue; // Owning queue
} CommandQueueTestCommandList;

static CommandQueueObservations g_commandQueue;

static const riDeviceVTable       s_command_queue_device_vtable;
static const riDeviceVTable       s_command_queue_missing_queue_vtable;
static const riDeviceVTable       s_command_queue_missing_command_list_vtable;
static const riDeviceVTable       s_command_queue_failing_device_vtable;
static const riDeviceVTable       s_command_queue_multi_submit_device_vtable;
static const riCommandQueueVTable s_command_queue_vtable;
static const riCommandQueueVTable s_command_queue_no_command_list_vtable;
static const riCommandQueueVTable s_command_queue_multi_submit_vtable;
static const riCommandListVTable  s_command_list_vtable;

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
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );

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
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );

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
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );

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
command_queue_create_with_multi_submit( riDevice device, riCommandQueue * outQueue )
{
    CommandQueueTestQueue * queue = NULL;

    // Input validation
    // ----------------------------------------------------------
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.queueCreateCalls;

    // Queue allocation
    // ----------------------------------------------------------
    queue = (CommandQueueTestQueue *)RI_CALLOC( 1, sizeof( *queue ) );
    if( queue == NULL ) return RI_ERROR_OUT_OF_MEMORY;

    // Queue state initialization
    // ----------------------------------------------------------
    queue->base.vtable = &s_command_queue_multi_submit_vtable;
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
    CommandQueueTestCommandList * commandList = NULL;

    // Input validation
    // ----------------------------------------------------------
    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outCommandList != NULL, RI_ERROR_INVALID_PARAM );

    // Backend observation
    // ----------------------------------------------------------
    ++g_commandQueue.commandListCreateCalls;

    // Command list allocation
    // ----------------------------------------------------------
    commandList = (CommandQueueTestCommandList *)RI_CALLOC( 1, sizeof( *commandList ) );
    if( commandList == NULL ) return RI_ERROR_OUT_OF_MEMORY;

    // Command list state initialization
    // ----------------------------------------------------------
    commandList->base.vtable = &s_command_list_vtable;
    commandList->queue       = queue;

    // Command list handle handoff
    // ----------------------------------------------------------
    *outCommandList = (riCommandList)commandList;

    return RI_SUCCESS;
}

static void
command_list_destroy( riCommandList commandList )
{
    CommandQueueTestCommandList * list = (CommandQueueTestCommandList *)commandList;

    if( list != NULL )
        {
            list->queue = NULL;
        }
}

static riStatus
command_list_begin( riCommandList commandList )
{
    UNUSED( commandList );

    return RI_SUCCESS;
}

static riStatus
command_list_end( riCommandList commandList )
{
    UNUSED( commandList );

    return RI_SUCCESS;
}

static riStatus
command_list_reset( riCommandList commandList )
{
    UNUSED( commandList );

    return RI_SUCCESS;
}

static riStatus
command_list_begin_render_pass( riCommandList commandList, const riRenderPassInfo * info, riRenderPass * outRenderPass )
{
    UNUSED( commandList );
    UNUSED( info );

    if( outRenderPass != NULL ) *outRenderPass = NULL;

    return RI_ERROR_BACKEND_UNAVAIL;
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

static riStatus
command_queue_submit_lists( riCommandQueue queue, const riCommandQueueSubmitInfo * info )
{
    UNUSED( queue );

    if( info == NULL ) return RI_ERROR_INVALID_PARAM;
    if( info->commandLists == NULL ) return RI_ERROR_INVALID_PARAM;
    if( info->commandListCount == 0u ) return RI_ERROR_INVALID_PARAM;

    ++g_commandQueue.queueSubmitInfoCalls;
    g_commandQueue.queueSubmitListCount += (int)info->commandListCount;

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

static const riDeviceVTable s_command_queue_multi_submit_device_vtable = {
    .init                 = command_queue_backend_init,
    .shutdown             = command_queue_backend_shutdown,
    .create_command_queue = command_queue_create_with_multi_submit,
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

static const riCommandQueueVTable s_command_queue_multi_submit_vtable = {
    .create_command_list   = command_queue_create_command_list,
    .destroy_command_queue = command_queue_destroy,
    .submit_command_lists  = command_queue_submit_lists,
};

static const riCommandListVTable s_command_list_vtable = {
    .destroy_command_list = command_list_destroy,
    .begin                = command_list_begin,
    .end                  = command_list_end,
    .reset                = command_list_reset,
    .begin_render_pass    = command_list_begin_render_pass,
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

    rhioDestroyCommandList( commandList );
    rhioDestroyCommandQueue( queue );
    EXPECT_EQ( g_commandQueue.queueDestroyCalls, 1 );

    rhioDestroyDevice( device );
    EXPECT_EQ( g_commandQueue.deviceShutdownCalls, 1 );
}

// Submits multiple command lists through the single-list fallback when a backend
// has not provided a batched submit entry point yet
TEST( command_queue, submit_info_falls_back_to_single_list_dispatch )
{
    riDevice                 device          = NULL;
    riCommandQueue           queue           = NULL;
    riCommandList            commandLists[2] = { NULL, NULL };
    riCommandQueueSubmitInfo submitInfo      = RI_ZERO_INIT;
    riDeviceInfo             info            = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable            = &s_command_queue_device_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandList( queue, &commandLists[0] ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandList( queue, &commandLists[1] ), RI_SUCCESS );

    submitInfo.commandLists     = commandLists;
    submitInfo.commandListCount = 2u;

    EXPECT_EQ( rhioCommandQueueSubmitInfo( queue, &submitInfo ), RI_SUCCESS );
    EXPECT_EQ( g_commandQueue.queueSubmitCalls, 2 );
    EXPECT_EQ( g_commandQueue.queueSubmitInfoCalls, 0 );

    rhioDestroyCommandList( commandLists[0] );
    rhioDestroyCommandList( commandLists[1] );
    rhioDestroyCommandQueue( queue );
    rhioDestroyDevice( device );
}

// Uses the batched submit slot when a backend exposes one
TEST( command_queue, submit_info_uses_batched_dispatch_when_available )
{
    riDevice                 device          = NULL;
    riCommandQueue           queue           = NULL;
    riCommandList            commandLists[2] = { NULL, NULL };
    riCommandQueueSubmitInfo submitInfo      = RI_ZERO_INIT;
    riDeviceInfo             info            = RI_ZERO_INIT;

    reset_command_queue_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    info.vtable            = &s_command_queue_multi_submit_device_vtable;
    info.backendDeviceSize = sizeof( CommandQueueTestDevice );

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandQueue( device, &queue ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandList( queue, &commandLists[0] ), RI_SUCCESS );
    EXPECT_EQ( rhioCreateCommandList( queue, &commandLists[1] ), RI_SUCCESS );

    submitInfo.commandLists     = commandLists;
    submitInfo.commandListCount = 2u;

    EXPECT_EQ( rhioCommandQueueSubmitInfo( queue, &submitInfo ), RI_SUCCESS );
    EXPECT_EQ( g_commandQueue.queueSubmitCalls, 0 );
    EXPECT_EQ( g_commandQueue.queueSubmitInfoCalls, 1 );
    EXPECT_EQ( g_commandQueue.queueSubmitListCount, 2 );

    rhioDestroyCommandList( commandLists[0] );
    rhioDestroyCommandList( commandLists[1] );
    rhioDestroyCommandQueue( queue );
    rhioDestroyDevice( device );
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
