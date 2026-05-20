// ============================================================================
// rhio - device unit tests
// ============================================================================
// Validates the backend-neutral device lifecycle and vtable resolution.
// ============================================================================

#include "rhio.h"

#include <rktest/rktest.h>

//----------------------------------------------------------------------------------
// Custom Backend Test Double
//----------------------------------------------------------------------------------

// Marker used to verify that init correctly touched the state
#define CUSTOM_BACKEND_INIT_MARKER 0xCAFEBABE

enum
{
    RI_BACKEND_CUSTOM = 100 // Test-only backend token
};

// Backend-private state allocated by RHIO for the custom backend test double
typedef struct CustomBackendDeviceState
{
    riDeviceBase base;       // Must be first for frontend dispatch
    riU32        initMarker; // Sentinel proving shutdown sees init-populated state
    riFlags      flags;      // Device flags cached by the custom backend
} CustomBackendDeviceState;

// Observations captured while RHIO drives the custom backend lifecycle
typedef struct CustomBackendObservations
{
    int initCalls;              // Number of backend init attempts
    int shutdownCalls;          // Number of backend shutdown calls
    int initSawZeroedState;     // Whether init received zeroed backend-private fields

    riFlags      initFlags;     // Flags passed through riBackendInitInfo during init
    riFlags      shutdownFlags; // Flags read back from backend-private state during shutdown
    const char * initAppName;   // Application name passed through init info

    int shutdownSawLiveState;   // Whether shutdown saw state modified by init
} CustomBackendObservations;

static CustomBackendObservations g_customBackend;

static void
reset_custom_backend_observations( void )
{
    static const CustomBackendObservations emptyObservations = { 0 };
    g_customBackend                                          = emptyObservations;
}

// Accepts a custom backend device, records init inputs, and marks the state live
static riStatus
custom_backend_init_success( riDevice device, const riBackendInitInfo * info )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)device;

    ++g_customBackend.initCalls;

    // Callback contract
    // ----------------------------------------------------------
    // NOTE: The tests expect RHIO to allocate the full backend device object.
    if( state == NULL || info == NULL ) return RI_ERROR_INVALID_PARAM;

    // Init inputs
    // ----------------------------------------------------------
    // RHIO must set the base vtable, then pass zeroed backend-private fields and normalized init info.
    g_customBackend.initSawZeroedState = ( state->initMarker == 0u ) && ( state->flags == 0u );
    g_customBackend.initFlags          = info->flags;
    g_customBackend.initAppName        = info->base.appName;

    // Side effect to verify shutdown path
    state->initMarker = CUSTOM_BACKEND_INIT_MARKER;
    state->flags      = info->flags;

    return RI_SUCCESS;
}

// Records the backend state seen during device teardown
static void
custom_backend_shutdown( riDevice device )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)device;

    ++g_customBackend.shutdownCalls;

    if( state != NULL )
        {
            g_customBackend.shutdownFlags        = state->flags;
            g_customBackend.shutdownSawLiveState = ( state->initMarker == CUSTOM_BACKEND_INIT_MARKER );
        }
}

static riStatus
custom_backend_create_command_queue( riDevice device, riCommandQueue * outQueue )
{
    // Queue creation contract
    // ----------------------------------------------------------
    // Device tests only require a complete device vtable; queue behavior is covered separately.
    UNUSED( device );
    UNUSED( outQueue );

    return RI_SUCCESS;
}

// Simulates a backend init failure after partially touching backend state
static riStatus
custom_backend_init_failure( riDevice device, const riBackendInitInfo * info )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)device;

    (void)info;
    ++g_customBackend.initCalls;

    // Partial initialization
    // ----------------------------------------------------------
    // The touched marker verifies failed init still routes through shutdown.
    if( state != NULL )
        {
            state->initMarker = CUSTOM_BACKEND_INIT_MARKER;
        }

    return RI_ERROR_BACKEND_INIT;
}

static const riDeviceVTable s_custom_backend_vtable = {
    .init                 = custom_backend_init_success,
    .shutdown             = custom_backend_shutdown,
    .create_command_queue = custom_backend_create_command_queue,
};

static const riDeviceVTable s_custom_backend_failing_vtable = {
    .init                 = custom_backend_init_failure,
    .shutdown             = custom_backend_shutdown,
    .create_command_queue = custom_backend_create_command_queue,
};

//----------------------------------------------------------------------------------
// Device Argument Validation
//----------------------------------------------------------------------------------

// Rejects missing input/output pointers and clears stale output handles
TEST( device, rejects_null_args )
{
    riDevice     device = (riDevice)( (uintptr_t)1 );
    riDeviceInfo info   = RI_ZERO_INIT;

    riSetTraceLogLevel( RI_LOG_NONE );

    EXPECT_EQ( rhioCreateDevice( NULL, NULL ), RI_ERROR_INVALID_PARAM );

    EXPECT_EQ( rhioCreateDevice( &info, NULL ), RI_ERROR_INVALID_PARAM );

    EXPECT_EQ( rhioCreateDevice( NULL, &device ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( device, NULL );
}

// Rejects flags that are not supported by the core or requested backend
TEST( device, rejects_unknown_flags )
{
    riDevice     device = NULL;
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Invalid flag setup
    // ----------------------------------------------------------
    // Unknown bits must fail validation before backend callbacks are reached
    info.flags             = (riFlags)( RI_DEVICE_FLAG_DEBUG << 1 );
    info.vtable            = &s_custom_backend_vtable;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Validation result
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( device, NULL );
    EXPECT_EQ( g_customBackend.initCalls, 0 );
}

//----------------------------------------------------------------------------------
// Custom Backend Validation
//----------------------------------------------------------------------------------

// Rejects a custom backend vtable that cannot init, shutdown, and create queues
TEST( device, rejects_incomplete_custom_backend )
{
    riDevice     device = NULL;
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Incomplete vtable
    // ----------------------------------------------------------
    // Custom backends must expose the required device entry points
    riDeviceVTable incompleteVTable = RI_ZERO_INIT;

    incompleteVTable.init           = custom_backend_init_success;
    info.vtable                     = &incompleteVTable;
    info.backendDeviceSize          = sizeof( CustomBackendDeviceState );

    // Validation result
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_INVALID_STATE );
    EXPECT_PTR_EQ( device, NULL );
    EXPECT_EQ( g_customBackend.initCalls, 0 );
}

// Verifies that custom backends receive a correctly zeroed state block
TEST( device, passes_zeroed_state_to_custom_backend )
{
    riDevice     device = NULL;
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Custom backend setup
    // ----------------------------------------------------------
    info.base.appName      = "rhio unit";
    info.backend           = RI_BACKEND_CUSTOM;
    info.flags             = RI_DEVICE_FLAG_DEBUG;
    info.vtable            = &s_custom_backend_vtable;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Create path
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );

    // Callback contract verification
    // ----------------------------------------------------------
    EXPECT_EQ( g_customBackend.initCalls, 1 );
    EXPECT_TRUE( g_customBackend.initSawZeroedState );
    EXPECT_EQ( g_customBackend.initFlags, RI_DEVICE_FLAG_DEBUG );
    EXPECT_STREQ( g_customBackend.initAppName, "rhio unit" );

    // Shutdown path
    // ----------------------------------------------------------
    rhioDestroyDevice( device );
    EXPECT_EQ( g_customBackend.shutdownCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownFlags, RI_DEVICE_FLAG_DEBUG );
}

// Ensures that if init fails, RHIO still calls shutdown to allow for cleanup
TEST( device, cleans_up_after_failed_init )
{
    riDevice     device = (riDevice)( (uintptr_t)1 );
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Failing backend setup
    // ----------------------------------------------------------
    // NOTE: The non-null stale handle checks that RHIO nulls output on failure.
    info.vtable            = &s_custom_backend_failing_vtable;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Cleanup path
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_BACKEND_INIT );
    EXPECT_PTR_EQ( device, NULL );

    // Cleanup contract verification
    // ----------------------------------------------------------
    EXPECT_EQ( g_customBackend.initCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownCalls, 1 ); // Core must call shutdown if init fails
    EXPECT_TRUE( g_customBackend.shutdownSawLiveState );
}
