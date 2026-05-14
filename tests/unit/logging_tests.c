// ============================================================================
// rhio - logging unit tests
// ============================================================================
// Validates the optional trace-log callback path, including log-level filtering
// and formatted message forwarding through the public logging API
// ============================================================================

#include "rhio.h"

#include <rktest/rktest.h>
#include <stdio.h>

//----------------------------------------------------------------------------------
// Custom Log Callback
//----------------------------------------------------------------------------------

// Captured log callback state
typedef struct CustomLogState
{
    int  calls;       // Callback calls
    int  type;        // Log level
    char message[64]; // Formatted text
} CustomLogState;

static CustomLogState g_customLog;

// Resets captured callback state before each logging assertion sequence
static void
reset_custom_log( void )
{
    static const CustomLogState emptyLog = { 0 };

    g_customLog                          = emptyLog;
    g_customLog.type                     = RI_LOG_NONE;
}

// Captures the formatted trace message exactly as a user callback would receive it
static void
custom_log_callback( int logType, const char * text, va_list args )
{
    ++g_customLog.calls;
    g_customLog.type = logType;

    // Callback formatting
    // ----------------------------------------------------------
    vsnprintf( g_customLog.message, sizeof( g_customLog.message ), text, args );
}

//----------------------------------------------------------------------------------
// Logging Tests
//----------------------------------------------------------------------------------

// Verifies filtered logs are skipped and enabled logs reach the callback
TEST( logging, callback_respects_log_level )
{
    reset_custom_log();

    // Filtered log path
    // ----------------------------------------------------------
    // RI_LOG_INFO is below RI_LOG_ERROR and must not reach the callback
    riSetTraceLogCallback( custom_log_callback );
    riSetTraceLogLevel( RI_LOG_ERROR );
    riTraceLog( RI_LOG_INFO, "filtered" );

    EXPECT_EQ( g_customLog.calls, 0 );

    // Enabled log path
    // ----------------------------------------------------------
    // Exercise formatted forwarding, then restore global logging state
    riSetTraceLogLevel( RI_LOG_ALL );
    riTraceLog( RI_LOG_WARNING, "%d", 42 );

    riSetTraceLogCallback( NULL );
    riSetTraceLogLevel( RI_LOG_NONE );

    // Captured callback payload
    // ----------------------------------------------------------
    EXPECT_EQ( g_customLog.calls, 1 );
    EXPECT_EQ( g_customLog.type, RI_LOG_WARNING );
    EXPECT_STREQ( g_customLog.message, "42" );
}
