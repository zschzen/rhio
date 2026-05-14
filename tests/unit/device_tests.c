// ============================================================================
// rhio - device unit tests
// ============================================================================
// Validates device creation/destruction contracts and custom backend lifecycle
// behavior without depending on a real graphics API backend
// ============================================================================

#include "rhio.h"

#include <rktest/rktest.h>
#include <stdint.h>

//----------------------------------------------------------------------------------
// Custom Backend Test Double
//----------------------------------------------------------------------------------

// Backend-private marker values used by custom backend assertions
enum
{
    CUSTOM_BACKEND_INIT_MARKER = 0x5248494F // Sentinel written by init and checked during shutdown
};

// Backend-private state allocated by RHIO for the custom backend test double
typedef struct CustomBackendDeviceState
{
    riU32   initMarker; // Sentinel proving shutdown sees init-populated state
    riFlags flags;      // Device flags cached by the custom backend
} CustomBackendDeviceState;

// Observations captured while RHIO drives the custom backend lifecycle
typedef struct CustomBackendObservations
{
    int initCalls;              // Number of backend init attempts
    int shutdownCalls;          // Number of backend shutdown calls
    int initSawZeroedState;     // Whether init received zeroed backend-private memory

    riFlags initFlags;          // Flags passed through riBackendInitInfo during init
    riFlags shutdownFlags;      // Flags read back from backend-private state during shutdown
    riU32   shutdownInitMarker; // Marker read back from backend-private state during shutdown

    const char * initAppName;   // App name passed through riBackendInitInfo during init
} CustomBackendObservations;

static CustomBackendObservations g_customBackend;

// Clears every observation captured by the custom backend test double
static void
reset_custom_backend_observations( void )
{
    static const CustomBackendObservations emptyObservations = { 0 };

    g_customBackend                                          = emptyObservations;
}

// Accepts a custom backend device, records init inputs, and marks the state live
static riStatus
custom_backend_init_success( void * backendDevice, const riBackendInitInfo * info )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)backendDevice;

    ++g_customBackend.initCalls;

    // Callback contract
    // ----------------------------------------------------------
    // NOTE: The tests expect RHIO to allocate backendDevice for custom backends.
    if( state == NULL || info == NULL ) return RI_ERROR_INVALID_PARAM;

    // Init inputs
    // ----------------------------------------------------------
    // RHIO must pass zeroed backend-private state and normalized init info.
    g_customBackend.initSawZeroedState = ( state->initMarker == 0u ) && ( state->flags == 0u );
    g_customBackend.initFlags          = info->flags;
    g_customBackend.initAppName        = info->base.appName;

    // Backend-private state
    // ----------------------------------------------------------
    // The marker lets shutdown prove it sees the same backend-private storage.
    state->initMarker = CUSTOM_BACKEND_INIT_MARKER;
    state->flags      = info->flags;

    return RI_SUCCESS;
}

// Records the backend state seen during device teardown
static void
custom_backend_shutdown( void * backendDevice )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)backendDevice;

    ++g_customBackend.shutdownCalls;

    // Teardown observation
    // ----------------------------------------------------------
    // NOTE: RHIO also calls shutdown when init fails after being attempted.
    if( state != NULL )
        {
            g_customBackend.shutdownInitMarker = state->initMarker;
            g_customBackend.shutdownFlags      = state->flags;
        }
}

// Simulates a backend init failure after partially touching backend state
static riStatus
custom_backend_init_failure( void * backendDevice, const riBackendInitInfo * info )
{
    CustomBackendDeviceState * state = (CustomBackendDeviceState *)backendDevice;

    (void)info;
    ++g_customBackend.initCalls;

    // Partial initialization
    // ----------------------------------------------------------
    // The touched marker verifies failed init still routes through shutdown
    if( state != NULL )
        {
            state->initMarker = CUSTOM_BACKEND_INIT_MARKER;
        }

    return RI_ERROR_BACKEND_INIT;
}

//----------------------------------------------------------------------------------
// Device Argument Validation
//----------------------------------------------------------------------------------

// Rejects missing input/output pointers and clears a stale output handle
TEST( device, rejects_null_create_args )
{
    riDevice     device = (riDevice)(uintptr_t)1;
    riDeviceInfo info   = RI_ZERO_INIT;

    riSetTraceLogLevel( RI_LOG_NONE );

    // Missing output pointer
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, NULL ), RI_ERROR_INVALID_PARAM );

    // Missing input pointer
    // ----------------------------------------------------------
    // NOTE: The stale handle proves RHIO clears caller output on failure
    EXPECT_EQ( rhioCreateDevice( NULL, &device ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( device, NULL );
}

// Rejects a concrete backend id that rhio.h does not provide yet
TEST( device, rejects_unavailable_backend )
{
    riDevice     device = NULL;
    riDeviceInfo info   = RI_ZERO_INIT;

    riSetTraceLogLevel( RI_LOG_NONE );

    // Unsupported built-in backend
    // ----------------------------------------------------------
    // Use a value outside the public enum instead of depending on build flags
    info.backend = (riBackend)999;

    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_BACKEND_UNAVAIL );
    EXPECT_PTR_EQ( device, NULL );
}

//----------------------------------------------------------------------------------
// Device Flag Validation
//----------------------------------------------------------------------------------

// Rejects unsupported device flags before calling into the backend vtable
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
    info.vtable.init       = custom_backend_init_success;
    info.vtable.shutdown   = custom_backend_shutdown;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Validation result
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( device, NULL );
    EXPECT_EQ( g_customBackend.initCalls, 0 );
    EXPECT_EQ( g_customBackend.shutdownCalls, 0 );
}

//----------------------------------------------------------------------------------
// Custom Backend Validation
//----------------------------------------------------------------------------------

// Rejects a custom backend vtable that cannot both init and shutdown
TEST( device, rejects_incomplete_custom_backend )
{
    riDevice     device = NULL;
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Incomplete vtable
    // ----------------------------------------------------------
    // Custom backends must expose both init and shutdown entry points
    info.vtable.init       = custom_backend_init_success;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Validation result
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_INVALID_PARAM );
    EXPECT_PTR_EQ( device, NULL );
    EXPECT_EQ( g_customBackend.initCalls, 0 );
    EXPECT_EQ( g_customBackend.shutdownCalls, 0 );
}

// Verifies successful custom init receives zeroed state and public create info
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
    info.vtable.init       = custom_backend_init_success;
    info.vtable.shutdown   = custom_backend_shutdown;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Create path
    // ----------------------------------------------------------
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_SUCCESS );
    EXPECT_PTR_NE( device, NULL );

    EXPECT_EQ( g_customBackend.initCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownCalls, 0 );
    EXPECT_TRUE( g_customBackend.initSawZeroedState );

    EXPECT_EQ( g_customBackend.initFlags, RI_DEVICE_FLAG_DEBUG );
    EXPECT_STREQ( g_customBackend.initAppName, "rhio unit" );

    // Destroy path
    // ----------------------------------------------------------
    // Shutdown must receive the backend-private state that init populated
    rhioDestroyDevice( device );

    EXPECT_EQ( g_customBackend.shutdownCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownInitMarker, CUSTOM_BACKEND_INIT_MARKER );
    EXPECT_EQ( g_customBackend.shutdownFlags, RI_DEVICE_FLAG_DEBUG );
}

// Ensures a failed backend init still runs shutdown and nulls the output handle
TEST( device, cleans_up_after_failed_init )
{
    riDevice     device = (riDevice)(uintptr_t)1;
    riDeviceInfo info   = RI_ZERO_INIT;

    reset_custom_backend_observations();
    riSetTraceLogLevel( RI_LOG_NONE );

    // Failing backend setup
    // ----------------------------------------------------------
    // NOTE: The non-null stale handle checks that RHIO nulls output on failure.
    info.vtable.init       = custom_backend_init_failure;
    info.vtable.shutdown   = custom_backend_shutdown;
    info.backendDeviceSize = sizeof( CustomBackendDeviceState );

    // Cleanup path
    // ----------------------------------------------------------
    // Even failed init attempts should run shutdown once for backend cleanup
    EXPECT_EQ( rhioCreateDevice( &info, &device ), RI_ERROR_BACKEND_INIT );
    EXPECT_PTR_EQ( device, NULL );

    EXPECT_EQ( g_customBackend.initCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownCalls, 1 );
    EXPECT_EQ( g_customBackend.shutdownInitMarker, CUSTOM_BACKEND_INIT_MARKER );
}
