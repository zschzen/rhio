/**********************************************************************************************
*
*     ▌ ▘   v0.0.1
*   ▛▘▛▌▌▛▌ Single-file header-only C99 RHI
*   ▌ ▌▌▌▙▌ zlib/libpng licensed
*
*   DESCRIPTION:
*       A tiny, single-header rendering hardware interface for C that routes
*       OpenGL 3.3, ES 2.0/3.0, and Vulkan 1.4 through one explicit device API.
*
*   TABLE OF CONTENTS:
*       1. Defines and Macros ...................................... [>>DEFS<<]
*       2. Enumerations ............................................ [>>ENUMS<<]
*       3. Opaque Resource Handles ................................. [>>HANDLES<<]
*       4. Types and Structures Definition ......................... [>>TYPES<<]
*          4.1 Context Creation Info ............................... [>>DEVICE_INFO<<]
*       5. Public API Declarations ................................. [>>API<<]
*       6. RHIO Implementation ..................................... [>>RHIO_IMPL<<]
*          6.1 API Impl ............................................ [>>API_IMPL<<]
*              6.1.1 Utilities ..................................... [>>UTILS<<]
*              6.1.2 Lifecycle ...................................... [>>LIFECYCLE<<]
*              6.1.3 Frame Control ................................. [>>FRAME<<]
*              6.1.4 Logging ....................................... [>>LOG<<]
*          6.2 GL Backend .......................................... [>>GL<<]
*              6.2.1 GL Types and Structures ....................... [>>GL_TYPES<<]
*              6.2.2 OpenGL Backend Impl ........................... [>>GL_IMPL<<]
*          6.3 VK Backend .......................................... [>>VK<<]
*
*   LIMITATIONS:
*       - ...
*
*   POSSIBLE IMPROVEMENTS:
*       - ...
*
*   ADDITIONAL NOTES:
*       - ..
*
*   CONFIGURATION:
*       #define RHIO_IMPLEMENTATION
*           Define this macro to compile the library’s implementation in the current file.
*           If left undefined, the library works header‑only and can be safely included anywhere.
*           Make sure only a single source file defines it.
*
*       #define RHIO_LOG_SUPPORT
*           Enables rhio logging system.
*
*   DEPENDENCIES:
*       - ...
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2026 SOHNE, Leandro Peres (@zschzen) and contributors
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef RHIO_H
#define RHIO_H

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------

#pragma region "Includes"

#include <stdarg.h> /* va_list, va_start, va_end */
#include <stdint.h> /* uint8_t, uint64_t, int32_t */
#include <stdlib.h> /* malloc, calloc, realloc, free */

#pragma endregion

//----------------------------------------------------------------------------------
// Defines and Macros                                                     [>>DEFS<<]
//----------------------------------------------------------------------------------

#pragma region "Defines and Macros"

// Shared library
#ifndef RI_EXPORT
#    if defined( rhio_EXPORTS ) && defined( _WIN32 )
#        define RI_EXPORT __declspec( dllexport )                      // Building Win32 .dll
#    elif defined( RHIO_DLL ) && defined( _WIN32 )
#        define RI_EXPORT __declspec( dllimport )                      // Using Win32 .dll
#    elif defined( rhio_EXPORTS ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
#        define RI_EXPORT __attribute__( ( visibility( "default" ) ) ) // Building Unix .so/.dylib
#    else
#        define RI_EXPORT                                              // static library
#    endif
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
#    define RI_NORETURN __attribute__( ( noreturn ) )
#    define RI_PACKED   __attribute__( ( packed ) )
#    define RI_PACKED_BEGIN
#    define RI_PACKED_END
#    define RI_FORCE_INLINE  inline __attribute__( ( always_inline ) )
#    define RI_ALIGNED( x )  __attribute__( ( aligned( x ) ) )
#    define RI_DEPRECATED    __attribute__( ( deprecated ) )
#    define RI_LIKELY( x )   __builtin_expect( !!( x ), 1 )
#    define RI_UNLIKELY( x ) __builtin_expect( !!( x ), 0 )

#elif defined( _MSC_VER )
#    define RI_NORETURN __declspec( noreturn )
#    define RI_PACKED
#    define RI_PACKED_BEGIN  __pragma( pack( push, 1 ) )
#    define RI_PACKED_END    __pragma( pack( pop ) )
#    define RI_FORCE_INLINE  __forceinline
#    define RI_ALIGNED( x )  __declspec( align( x ) )
#    define RI_DEPRECATED    __declspec( deprecated )
#    define RI_LIKELY( x )   ( x )
#    define RI_UNLIKELY( x ) ( x )

#else
#    define RI_NORETURN
#    define RI_PACKED
#    define RI_PACKED_BEGIN
#    define RI_PACKED_END
#    define RI_FORCE_INLINE inline
#    define RI_ALIGNED( x )
#    define RI_DEPRECATED
#    define RI_LIKELY( x )   ( x )
#    define RI_UNLIKELY( x ) ( x )
#endif

// C++ macros
#ifdef __cplusplus
#    define RI_API          extern "C" RI_EXPORT
#    define RI_INLINE       inline
#    define RI_LITERAL( T ) T
#    define RI_ZERO_INIT                                                                                               \
        {                                                                                                              \
        }

#else
#    define RI_API          RI_EXPORT
#    define RI_INLINE       static inline
#    define RI_LITERAL( T ) ( T )
#    define RI_ZERO_INIT    { 0 }
#endif

// Asserting
#ifndef RHIO_ASSERT
#    if defined( NDEBUG )
#        define RHIO_ASSERT( x, ... ) ( (void)0 )
#    else
#        include <assert.h>
#        define RHIO_ASSERT( x, ... )                                                                                  \
            do                                                                                                         \
                {                                                                                                      \
                    if( RI_UNLIKELY( !( x ) ) )                                                                        \
                        {                                                                                              \
                            TRACELOG( RI_LOG_FATAL, "ASSERTION FAILED: " __VA_ARGS__ );                                \
                            assert( x );                                                                               \
                        }                                                                                              \
                }                                                                                                      \
            while( 0 )
#    endif
#endif

// Custom memory allocators
#ifndef RI_MALLOC
#    define RI_MALLOC( sz ) malloc( sz )
#endif
#ifndef RI_CALLOC
#    define RI_CALLOC( n, sz ) calloc( n, sz )
#endif
#ifndef RI_REALLOC
#    define RI_REALLOC( pt, sz ) realloc( pt, sz )
#endif
#ifndef RI_FREE
#    define RI_FREE( pt ) free( pt )
#endif

//----------------------------------------------------------------------------------
// Misc utility macros
//----------------------------------------------------------------------------------

// Silence compiler for non used
#ifndef UNUSED
#    define UNUSED( x ) (void)( x )
#endif

// Determine if a given str is null or empty
#ifndef STR_NONEMPTY
#    define STR_NONEMPTY( s ) ( ( s ) != NULL && ( s )[0] != '\0' )
#endif

// Base trace log macro
#ifndef TRACELOG
#    define TRACELOG( l, ... ) riTraceLog( l, __VA_ARGS__ )
#endif

// API limits
#ifndef RHIO_MAX_COLOR_ATTACHMENTS
#    define RHIO_MAX_COLOR_ATTACHMENTS 8
#endif
#ifndef RHIO_MAX_VERTEX_ATTRIBUTES
#    define RHIO_MAX_VERTEX_ATTRIBUTES 16
#endif
#ifndef RHIO_MAX_VERTEX_BUFFERS
#    define RHIO_MAX_VERTEX_BUFFERS 8
#endif

// Internal helper macro. Determines if a cond isnt true
#define _RI_GUARD_CORE( cond, log_msg, ... )                                                                           \
    do                                                                                                                 \
        {                                                                                                              \
            if( RI_UNLIKELY( cond ) )                                                                                  \
                {                                                                                                      \
                    TRACELOG( RI_LOG_ERROR, "%s: " log_msg, __func__ );                                                \
                    __VA_ARGS__;                                                                                       \
                }                                                                                                      \
        }                                                                                                              \
    while( 0 )

#define RI_GUARD_NULL( ptr, retval ) _RI_GUARD_CORE( ( ptr ) == NULL, "unexpected NULL argument", return ( retval ) )
#define RI_GUARD( cond, retval )     _RI_GUARD_CORE( !( cond ), "guard failed: " #cond, return ( retval ) )
#define RI_GUARD_NULL_VOID( ptr )    _RI_GUARD_CORE( ( ptr ) == NULL, "unexpected NULL argument", return )
#define RI_GUARD_VOID( cond )        _RI_GUARD_CORE( !( cond ), "guard failed: " #cond, return )

// Type aliases
typedef uint8_t  riU8;
typedef uint16_t riU16;
typedef uint32_t riU32;
typedef uint64_t riU64;
typedef int8_t   riI8;
typedef int16_t  riI16;
typedef int32_t  riI32;
typedef int64_t  riI64;
typedef float    riF32;
typedef double   riF64;
typedef riU32    riFlags; /* bitfield type used for usage/feature flags */
typedef riU64    riSize;

#pragma endregion         // Defines and Macros

//----------------------------------------------------------------------------------
// Enumerations                                                          [>>ENUMS<<]
//----------------------------------------------------------------------------------

#pragma region "Enumerations"

// Logging Levels
typedef enum
{
    RI_LOG_ALL = 0, // Emit all logs
    RI_LOG_TRACE,   // Internal use
    RI_LOG_DEBUG,   // Internal use. Disabled on release builds
    RI_LOG_INFO,    // Program execution
    RI_LOG_WARNING, // Recoverable failures
    RI_LOG_ERROR,   // Unrecoverable failures
    RI_LOG_FATAL,   // Abort execution with EXIT_FAILURE
    RI_LOG_NONE     // Disable logging system
} riTraceLogLevel;

// Selects the rendering backend at context-creation time
typedef enum
{
    RI_BACKEND_OPENGL   = 0, // Desktop OpenGL 3.3 core profile
    RI_BACKEND_OPENGLES = 1, // OpenGL ES 2.0 / 3.0
    RI_BACKEND_VULKAN   = 2, // Vulkan 1.4
    RI_BACKEND_CUSTOM   = 3, // Caller supplies the full vtable

} riBackend;

// Return Status Codes
typedef enum
{
    RI_SUCCESS = 0,           // Operation completed successfully
    RI_ERROR_INVALID_PARAM,   // A required argument was NULL or out of range
    RI_ERROR_OUT_OF_MEMORY,   // Heap allocation failed
    RI_ERROR_BACKEND_INIT,    // The backend failed to initialise
    RI_ERROR_BACKEND_UNAVAIL, // Requested backend not compiled in / not supported
    RI_ERROR_SHADER_COMPILE,  // Shader source / SPIR-V could not be compiled/linked
    RI_ERROR_INVALID_STATE,   // Function called in wrong context (e.g. no active pass)
    RI_ERROR_NOT_READY,       // Resource not yet ready (async / swapchain)
    RI_ERROR_UNKNOWN,         // Catch-all. check logs for details

} riStatus;

#pragma endregion // Enumerations

//----------------------------------------------------------------------------------
// Opaque resource handles                                             [>>HANDLES<<]
//----------------------------------------------------------------------------------

#pragma region "Opaque resource handles"

// The main RHI context NOTE: Caller-Owned Instance
typedef struct RI_DEVICE_STRUCT * riDevice;

#pragma endregion

//----------------------------------------------------------------------------------
// Types and Structures Definition                                       [>>TYPES<<]
//----------------------------------------------------------------------------------

#pragma region "Types and Structures Definition"

// Boolean
#if !defined( __cplusplus ) && !defined( bool )
#    if ( defined( __has_include ) && __has_include( <stdbool.h> ) )   \
     || ( defined( __GNUC__ ) && __GNUC__ >= 3 )                       \
     || ( defined( _MSC_VER ) && _MSC_VER >= 1800 )                    \
     || ( defined( __clang__ ) )
#        include <stdbool.h>
#    elif !defined( bool )
#        ifndef RHIO_BOOL_TYPE
/* clang-format off */
             typedef enum bool { false = 0, true = !false } bool;
/* clang-format on */
#            define RHIO_BOOL_TYPE
#        endif
#    endif
#endif

#if !defined( RHIO_INIT_INFO_TYPE )
// Initialization information
typedef struct
{
    const char * appName; // Application name

} riInitInfo;
#    define RHIO_INIT_INFO_TYPE
#endif

// Dynamic Interface - Backend function pointers vtable
// NOTE: Every rendering operation is routed through a slot in this struct, allowing to:
//   - Swap backends at runtime without touching the public API
//   - Unit-test rendering code with a mock backend
//   - Add new backends without modifying existing backend code
// Backend authors fill this struct and pass it to `rhioCreateDevice()` via riDeviceInfo.
// Built-in backends (OpenGL, Vulkan) are registered automatically.
typedef struct riBackendFuncs
{
    // Lifecycle Management
    //------------------------------------------------------------------------------
    riStatus ( *init )( void * backendCtx, const riInitInfo * info ); // Initialize the backend graphics context
    void     ( *shutdown )( void * backendCtx );                      // Tear down backend resources and free memory

    // Frame Control Operations
    //------------------------------------------------------------------------------
    void ( *beginFrame )( void * backendCtx ); // Begin recording commands for the current frame
    void ( *endFrame )( void * backendCtx );   // End recording commands and submit to GPU
    void ( *present )( void * backendCtx );    // Present the final frame buffer to the screen

    //TODO: Add buffers, textures, shaders, pipelines, passes, draw, ...

} riBackendFuncs;

//----------------------------------------------------------------------------------
// Context creation info                                              [>>DEVICE_INFO<<]
//----------------------------------------------------------------------------------

// Context Initialization Information
// NOTE: Holds configuration for creating a rendering context.
// If `funcs.init` is set, the built-in backend selection
// is bypassed and the custom vtable is used directly;
// set `backend` to `RI_BACKEND_CUSTOM`.
typedef struct riDeviceInfo
{
    riInitInfo     base;    // Basic application info (App name, API version)
    riBackend      backend; // Built-in backend selection token (RI_BACKEND_OPENGL, RI_BACKEND_VULKAN)
    riBackendFuncs funcs;   // Dynamic interface hook for custom backend vtable
    riSize         backendCtxSize; // Bytes of backend-private state RHIO should allocate for custom backends

} riDeviceInfo;

#pragma endregion // Types and Structures Definition

//----------------------------------------------------------------------------------
// Callbacks
//----------------------------------------------------------------------------------

#pragma region "Callbacks"

// Logging Callback Signature
typedef void ( *TraceLogCallback )( int logType, const char * text, va_list args );

#pragma endregion

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

#pragma region "Global Variables Definition"

// ...

#pragma endregion

//----------------------------------------------------------------------------------
// Internal Functions Declaration
//----------------------------------------------------------------------------------

#pragma region "Internal Functions Declaration"

// ...

#pragma endregion

//----------------------------------------------------------------------------------
// Public API declarations                                                 [>>API<<]
//----------------------------------------------------------------------------------

#pragma region "Public API declarations"

// Status helpers
RI_API const char * riStatusToString( int status );

RI_API riStatus rhioCreateDevice( const riDeviceInfo * info, riDevice * outDevice );
RI_API void     rhioDestroyDevice( riDevice device );

// Frame control
RI_API void rhioBeginFrame( riDevice device );
RI_API void rhioEndFrame( riDevice device );
RI_API void rhioPresent( riDevice device );

// Logging system
RI_API void riSetTraceLogLevel( int logType );                  // Set the minimum log level
RI_API void riTraceLog( int logType, const char * text, ... );  // Emit trace log message
RI_API void riSetTraceLogCallback( TraceLogCallback callback ); // Set custom trace log

#pragma endregion

/***********************************************************************************
*
*   RHIO IMPLEMENTATION                                              [>>RHIO_IMPL<<]
*
************************************************************************************/

#pragma region "RHIO Implementation"

#ifdef RHIO_IMPLEMENTATION

#    include <stdio.h>

#    if defined( PLATFORM_ANDROID )
#        include <android/log.h>
#    endif

// Resolved backend metadata used only during device creation
// NOTE: Registration functions fill this struct so built-in and custom backends
// follow the same creation path.
typedef struct riBackendDesc
{
    const char *   name;    // Human-readable backend name for logs
    riBackend      backend; // Resolved backend token
    riBackendFuncs funcs;   // Backend dispatch table
    riSize         ctxSize; // Bytes of backend-private state RHIO should allocate
    riFlags        flags;   // Reserved for backend capabilities/options

} riBackendDesc;

//----------------------------------------------------------------------------------
// Internal device struct (hidden from callers — Handle pattern)
//
// The public `riDevice` typedef is `struct RI_DEVICE_STRUCT*`.  The
// definition only exists inside RHIO_IMPLEMENTATION, so callers can never
// peek at the members.
//----------------------------------------------------------------------------------

struct RI_DEVICE_STRUCT
{
    riBackend      backend;   // Resolved backend token
    riBackendFuncs funcs;     // Dispatch table used by public API entry points

    void * backendCtx;        // Backend-private state passed to every vtable call

    const char * backendName; // Stable backend name used for diagnostics
};

//----------------------------------------------------------------------------------
// Module Internal State
//----------------------------------------------------------------------------------

static int              rhio_logTypeLevel = RI_LOG_INFO;
static TraceLogCallback rhio_traceLog     = NULL;

//----------------------------------------------------------------------------------
// Backend registration helpers
//----------------------------------------------------------------------------------

static void     _rhioBackendDescInit( riBackendDesc * desc );
static riStatus _rhioResolveBackendDesc( const riDeviceInfo * info, riBackendDesc * desc );
static riStatus _rhioValidateBackendFuncs( const riBackendFuncs * funcs );

// OpenGL+ES register backend funcs
//---------------------------------------------------------------------------------
#    if defined( RHIO_BACKEND_OPENGL )                                                                                 \
        || defined( RHIO_BACKEND_OPENGLES )                                                                            \
        || defined( RHIO_BACKEND_OPENGLES2 )                                                                           \
        || defined( RHIO_BACKEND_OPENGLES3 )
static riStatus _rhioGL_registerBackend( riBackendDesc * desc );
#    endif

// Vulkan register backend funcs
//---------------------------------------------------------------------------------
#    if defined( RHIO_BACKEND_VULKAN )
static riStatus _rhioVK_registerBackend( riBackendDesc * desc );
#    endif

// =================================================================================
//
//    ▄▖▖▖▄ ▖ ▄▖▄▖  ▄▖▄▖▄▖
//    ▙▌▌▌▙▘▌ ▐ ▌   ▌▌▙▌▐                                             [>>API_IMPL<<]
//    ▌ ▙▌▙▘▙▖▟▖▙▖  ▛▌▌ ▟▖
//
// =================================================================================

#    pragma region "API Impl"

/* ---------------------------------------------------------------------------------
 *
 * UTILITIES                                                             [>>UTILS<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Utilities"

RI_API const char *
riStatusToString( int status )
{
    const char * str = "UNKNOWN";

    switch( status )
        {
        default: break;
#    define ST( s )                                                                                                    \
    case RI_##s:                                                                                                       \
        {                                                                                                              \
            str = #s;                                                                                                  \
        }                                                                                                              \
        break
            ST( SUCCESS );
            ST( ERROR_INVALID_PARAM );
            ST( ERROR_OUT_OF_MEMORY );
            ST( ERROR_BACKEND_INIT );
            ST( ERROR_BACKEND_UNAVAIL );
            ST( ERROR_SHADER_COMPILE );
            ST( ERROR_INVALID_STATE );
            ST( ERROR_NOT_READY );
            ST( ERROR_UNKNOWN );
#    undef ST
        }

    return str;
}

#    pragma endregion // Utilities

/* ---------------------------------------------------------------------------------
 *
 * LIFECYCLE                                                         [>>LIFECYCLE<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Lifecycle"

// Create and initialize a new device instance (Caller-owned)
RI_API riStatus
rhioCreateDevice( const riDeviceInfo * info, riDevice * outDevice )
{
    riStatus                  status               = RI_ERROR_UNKNOWN;
    riBackendDesc             desc                 = RI_ZERO_INIT;
    struct RI_DEVICE_STRUCT * device               = NULL;
    bool                      backendInitAttempted = false;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD_NULL( outDevice, RI_ERROR_INVALID_PARAM );
    *outDevice = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    // Backend Descriptor Resolution
    //----------------------------------------------------------
    // Convert caller config into one normalized descriptor for all backend types
    status = _rhioResolveBackendDesc( info, &desc );
    if( status != RI_SUCCESS ) goto fail;

    // Backend Vtable Validation
    //----------------------------------------------------------
    // Catch incomplete custom backends before allocating backend-private state
    status = _rhioValidateBackendFuncs( &desc.funcs );
    if( status != RI_SUCCESS ) goto fail;

    // Device Allocation
    //----------------------------------------------------------
    device = (struct RI_DEVICE_STRUCT *)RI_CALLOC( 1, sizeof( *device ) );
    if( RI_UNLIKELY( device == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Out of memory in %s", __func__ );
            status = RI_ERROR_OUT_OF_MEMORY;
            goto fail;
        }

    // Device State Initialization
    //----------------------------------------------------------
    device->backend     = desc.backend;
    device->funcs       = desc.funcs;
    device->backendName = desc.name;

    // Backend Context Allocation
    //----------------------------------------------------------
    // NOTE: ctxSize == 0 is valid; the backend receives NULL and manages state elsewhere.
    if( desc.ctxSize > 0 )
        {
            if( RI_UNLIKELY( desc.ctxSize > (riSize)( (size_t)-1 ) ) )
                {
                    TRACELOG( RI_LOG_ERROR, "DEVICE: Backend state allocation size is too large" );
                    status = RI_ERROR_OUT_OF_MEMORY;
                    goto fail;
                }

            device->backendCtx = RI_CALLOC( 1, (size_t)desc.ctxSize );
            if( RI_UNLIKELY( device->backendCtx == NULL ) )
                {
                    TRACELOG( RI_LOG_ERROR, "DEVICE: Backend state allocation failed. OOM" );
                    status = RI_ERROR_OUT_OF_MEMORY;
                    goto fail;
                }
        }

    // Backend Initialization
    //----------------------------------------------------------
    // Backend receives its private state plus the public application init info
    // NOTE: shutdown is called on init failure so backends can centralize cleanup.
    backendInitAttempted = true;
    status               = device->funcs.init( device->backendCtx, &info->base );
    if( status != RI_SUCCESS )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Backend initialization failed: %s (%d)", riStatusToString( status ),
                      status );
            goto fail;
        }

    // Device Handle Handoff
    //----------------------------------------------------------
    *outDevice = device;

    TRACELOG( RI_LOG_INFO, "DEVICE: Created successfully using %s backend",
              STR_NONEMPTY( device->backendName ) ? device->backendName : "unknown" );

    return RI_SUCCESS;

fail:
    // Failure Cleanup
    //----------------------------------------------------------
    if( device != NULL )
        {
            if( backendInitAttempted && device->funcs.shutdown != NULL )
                {
                    device->funcs.shutdown( device->backendCtx );
                }

            if( device->backendCtx != NULL )
                {
                    RI_FREE( device->backendCtx );
                }

            RI_FREE( device );
        }

    return status;
}

// Destroy graphics device, shutdown backend, and free associated memory
RI_API void
rhioDestroyDevice( riDevice device )
{
    if( device == NULL ) return;

    // Backend Shutdown
    //----------------------------------------------------------
    // NOTE: shutdown receives the same private state pointer passed to init().
    if( device->funcs.shutdown != NULL )
        {
            device->funcs.shutdown( device->backendCtx );
        }

    // Device Memory Cleanup
    //----------------------------------------------------------
    if( device->backendCtx != NULL )
        {
            RI_FREE( device->backendCtx );
        }

    RI_FREE( device );

    TRACELOG( RI_LOG_INFO, "DEVICE: Destroyed successfully" );
}

#    pragma endregion // Lifecycle

#    pragma endregion // API Impl

/* ---------------------------------------------------------------------------------
 *
 * FRAME CONTROL                                                         [>>FRAME<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Frame Control"

// Prepare the context for a new frame's rendering commands
RI_API void
rhioBeginFrame( riDevice ctx )
{
    RI_GUARD_NULL_VOID( ctx );
    RHIO_ASSERT( ctx->funcs.beginFrame != NULL, "backend beginFrame is NULL" );
    ctx->funcs.beginFrame( ctx->backendCtx );
}

// Finalize rendering commands for the current frame
RI_API void
rhioEndFrame( riDevice ctx )
{
    RI_GUARD_NULL_VOID( ctx );
    RHIO_ASSERT( ctx->funcs.endFrame != NULL, "backend endFrame is NULL" );
    ctx->funcs.endFrame( ctx->backendCtx );
}

// Present the rendered frame to the screen
RI_API void
rhioPresent( riDevice ctx )
{
    RI_GUARD_NULL_VOID( ctx );
    RHIO_ASSERT( ctx->funcs.present != NULL, "backend present is NULL" );
    ctx->funcs.present( ctx->backendCtx );
}

#    pragma endregion

/* ---------------------------------------------------------------------------------
 *
 * LOGGING                                                                 [>>LOG<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Logging"

// Set the minimum log level
RI_API void
riSetTraceLogLevel( int logType )
{
    rhio_logTypeLevel = logType;
}

// Emit trace log messages
RI_API void
riTraceLog( int logType, const char * text, ... )
{
#    if defined( RHIO_LOG_SUPPORT )
    // Threshold message level
    if( ( rhio_logTypeLevel > logType ) || !STR_NONEMPTY( text ) ) return;

    va_list args;
    va_start( args, text );

    // Custom trace log
    if( rhio_traceLog )
        {
            rhio_traceLog( logType, text, args );
            va_end( args );
            return;
        }

#        if defined( PLATFORM_ANDROID )
    int androidLevel = ANDROID_LOG_DEFAULT;
    switch( logType )
        {
        case RI_LOG_TRACE:   androidLevel = ANDROID_LOG_VERBOSE; break;
        case RI_LOG_DEBUG:   androidLevel = ANDROID_LOG_DEBUG; break;
        case RI_LOG_INFO:    androidLevel = ANDROID_LOG_INFO; break;
        case RI_LOG_WARNING: androidLevel = ANDROID_LOG_WARN; break;
        case RI_LOG_ERROR:   androidLevel = ANDROID_LOG_ERROR; break;
        case RI_LOG_FATAL:   androidLevel = ANDROID_LOG_FATAL; break;
        }
    __android_log_vprint( androidLevel, "rhio", text, args );

#        else
    // Get log string name
    const char * prefix = "";
#            define LOG_CASE( level )                                                                                  \
            case RI_LOG_##level: prefix = #level ": "; break;

    switch( logType )
        {
            LOG_CASE( TRACE )
            LOG_CASE( DEBUG )
            LOG_CASE( INFO )
            LOG_CASE( WARNING )
            LOG_CASE( ERROR )
            LOG_CASE( FATAL )
        }
#            undef LOG_CASE

    FILE * stream = ( RI_LOG_WARNING <= logType ) ? stderr : stdout;
    fprintf( stream, "[rhio] %s", prefix );
    vfprintf( stream, text, args );
    fprintf( stream, "\n" );
    fflush( stream );
#        endif

    va_end( args );

    if( RI_LOG_FATAL == logType ) exit( EXIT_FAILURE );
#    endif /* RHIO_LOG_SUPPORT */
}

// Set a custom trace log
RI_API void
riSetTraceLogCallback( TraceLogCallback callback )
{
    rhio_traceLog = callback;
}

#    pragma endregion // Logging

// =================================================================================
//
//    ▄▖▄▖▄▖▖ ▖▄▖▖   ▄▖▄▖
//    ▌▌▙▌▙▖▛▖▌▌ ▌ ▟▖▙▖▚                                                    [>>GL<<]
//    ▙▌▌ ▙▖▌▝▌▙▌▙▖▝ ▙▖▄▌
//
// =================================================================================

#    pragma region "GL Backend"

#    if defined( RHIO_BACKEND_OPENGL ) || defined( RHIO_BACKEND_OPENGLES )

//----------------------------------------------------------------------------------
// Types and Structures Definition                                    [>>GL_TYPES<<]
//----------------------------------------------------------------------------------

// Internal OpenGL context state
typedef struct riGL_Context
{
    int dummyPlaceholder; // TODO: Replace with actual OpenGL resource handles

} riGL_Context;

//----------------------------------------------------------------------------------
// OpenGL Backend Implementation (Stubs)                               [>>GL_IMPL<<]
//----------------------------------------------------------------------------------

// Initialize OpenGL state and allocate default resources
static riStatus
_rhioGL_init( void * backendCtx, const riInitInfo * info )
{
    UNUSED( backendCtx );
    UNUSED( info );

    // TODO: Initialize GL state, load extensions, create default VAO, etc.

    TRACELOG( RI_LOG_INFO, "BACKEND GL: Initialized successfully (stub)" );

    return RI_SUCCESS;
}

// Free OpenGL resources and tear down context state
static void
_rhioGL_shutdown( void * backendCtx )
{
    UNUSED( backendCtx );

    // TODO: Delete allocated GL resources (buffers, textures, shaders)

    TRACELOG( RI_LOG_INFO, "BACKEND GL: Shutdown complete (stub)" );
}

// Prepare OpenGL state for a new frame's rendering commands
static void
_rhioGL_beginFrame( void * backendCtx )
{
    UNUSED( backendCtx );

    // TODO: Reset per-frame scratch allocators and bind default states
}

// Finalize OpenGL rendering commands for the current frame
static void
_rhioGL_endFrame( void * backendCtx )
{
    UNUSED( backendCtx );

    // TODO: Flush command queues if batching is implemented
}

// Display the rendered frame to the screen
static void
_rhioGL_present( void * backendCtx )
{
    UNUSED( backendCtx );

    // TODO: Invoke platform-specific swap-buffers (e.g., glfwSwapBuffers)
}

// Populate the backend descriptor with OpenGL implementations
static riStatus
_rhioGL_registerBackend( riBackendDesc * desc )
{
    riBackend backend = RI_BACKEND_OPENGL;

    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    // Preserve requested GL flavor before clearing the descriptor
    if( desc->backend == RI_BACKEND_OPENGLES )
        {
            backend = RI_BACKEND_OPENGLES;
        }

    _rhioBackendDescInit( desc );

    // Backend Descriptor Setup
    //----------------------------------------------------------
    desc->name    = ( backend == RI_BACKEND_OPENGLES ) ? "OpenGL ES" : "OpenGL";
    desc->backend = backend;
    desc->ctxSize = (riSize)sizeof( riGL_Context );
    desc->flags   = 0;

    // Bind OpenGL-specific functions to the dynamic interface
    //----------------------------------------------------------
#        define BIND_GL_FUNC( name ) desc->funcs.name = _rhioGL_##name

    BIND_GL_FUNC( init );
    BIND_GL_FUNC( shutdown );
    BIND_GL_FUNC( beginFrame );
    BIND_GL_FUNC( endFrame );
    BIND_GL_FUNC( present );

#        undef BIND_GL_FUNC

    return RI_SUCCESS;
}

#    endif            /* RHIO_BACKEND_OPENGL || RHIO_BACKEND_OPENGLES */

#    pragma endregion // GL Backend

// =================================================================================
//
//    ▖▖▖▖▖ ▖▖▄▖▖ ▖
//    ▌▌▌▌▌ ▙▘▌▌▛▖▌                                                         [>>VK<<]
//    ▚▘▙▌▙▖▌▌▛▌▌▝▌
//
// =================================================================================

#    pragma region "VK Backend"

#    if defined( RHIO_BACKEND_VULKAN )
// Fill Vulkan descriptor metadata and report it unavailable for now
static riStatus
_rhioVK_registerBackend( riBackendDesc * desc )
{
    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    _rhioBackendDescInit( desc );

    // Backend Descriptor Setup
    //----------------------------------------------------------
    // NOTE: Descriptor fields are useful for diagnostics, but creation still fails
    // until the Vulkan vtable is implemented.
    desc->name    = "Vulkan";
    desc->backend = RI_BACKEND_VULKAN;
    desc->flags   = 0;

    TRACELOG( RI_LOG_ERROR, "BACKEND VK: Vulkan backend registration is not implemented yet" );

    return RI_ERROR_BACKEND_UNAVAIL;
}
#    endif

#    pragma endregion

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Reset backend descriptor fields before registration
static void
_rhioBackendDescInit( riBackendDesc * desc )
{
    if( desc != NULL )
        {
            // Clear all fields before a backend writes its supported capabilities
            riBackendDesc empty = RI_ZERO_INIT;
            *desc               = empty;
        }
}

// Ensure backend exposes callbacks required by the device API
static riStatus
_rhioValidateBackendFuncs( const riBackendFuncs * funcs )
{
    RI_GUARD_NULL( funcs, RI_ERROR_INVALID_PARAM );

    // Required backend callbacks
    //----------------------------------------------------------
    // NOTE: Public API calls dispatch through these slots after device creation.
#    define CHECK_BACKEND_FUNC( name )                                                                                 \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( funcs->name == NULL ) )                                                               \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "BACKEND: Missing required vtable slot '%s'", #name );                 \
                        return RI_ERROR_INVALID_PARAM;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_BACKEND_FUNC( init );
    CHECK_BACKEND_FUNC( shutdown );
    CHECK_BACKEND_FUNC( beginFrame );
    CHECK_BACKEND_FUNC( endFrame );
    CHECK_BACKEND_FUNC( present );

#    undef CHECK_BACKEND_FUNC

    return RI_SUCCESS;
}

// Convert device creation info into a concrete backend descriptor
static riStatus
_rhioResolveBackendDesc( const riDeviceInfo * info, riBackendDesc * desc )
{
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    _rhioBackendDescInit( desc );

    // Custom backend
    //----------------------------------------------------------
    // NOTE: A custom vtable takes precedence over info->backend. Missing required
    // slots are reported by `_rhioValidateBackendFuncs()`.
    if( info->funcs.init != NULL )
        {
            desc->name    = "Custom";
            desc->backend = RI_BACKEND_CUSTOM;
            desc->funcs   = info->funcs;
            desc->ctxSize = info->backendCtxSize;
            return RI_SUCCESS;
        }

    // Built-in backend
    //----------------------------------------------------------
    // NOTE: Only backends compiled into this translation unit can register here.
    switch( info->backend )
        {
#    if defined( RHIO_BACKEND_OPENGL )                                                                                 \
        || defined( RHIO_BACKEND_OPENGLES )                                                                            \
        || defined( RHIO_BACKEND_OPENGLES2 )                                                                           \
        || defined( RHIO_BACKEND_OPENGLES3 )

            // Built-in: OpenGL / OpenGL ES
            //----------------------------------------------------------
        case RI_BACKEND_OPENGL:
        case RI_BACKEND_OPENGLES: desc->backend = info->backend; return _rhioGL_registerBackend( desc );
#    endif

#    if defined( RHIO_BACKEND_VULKAN )
            // Built-in: Vulkan
            //----------------------------------------------------------
        case RI_BACKEND_VULKAN: desc->backend = info->backend; return _rhioVK_registerBackend( desc );
#    endif

            // Unsupported or uncompiled backend fallback
            //----------------------------------------------------------
        default:
            TRACELOG( RI_LOG_ERROR, "DEVICE: Backend '%d' is unavailable or not compiled in", info->backend );
            return RI_ERROR_BACKEND_UNAVAIL;
        }
}

#endif            // RHIO_IMPLEMENTATION

#pragma endregion // RHIO Implementation

#endif            // RHIO_H
