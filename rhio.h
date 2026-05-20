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
*          4.1 Device Creation Info ................................ [>>DEVICE_INFO<<]
*       5. Public API Declarations ................................. [>>API<<]
*       6. RHIO Implementation ..................................... [>>RHIO_IMPL<<]
*          6.1 API Impl ............................................ [>>API_IMPL<<]
*              6.1.1 Utilities ..................................... [>>UTILS<<]
*              6.1.2 Lifecycle ..................................... [>>LIFECYCLE<<]
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
*           Generates the implementation. Required in exactly one source file.
*
*       #define RHIO_LOG_SUPPORT
*           Enables internal logging system (stdout/stderr).
*
*       #define RHIO_DEBUG
*           Enables debug-only features (e.g. assertions).
*
*       #define RHIO_BACKEND_OPENGL       // OpenGL 3.3+
*       #define RHIO_BACKEND_OPENGLES     // OpenGLES 2.0 / 3.0
*       #define RHIO_BACKEND_VULKAN       // Vulkan 1.4+
*           Enables the desired rendering backends.
*
*       #define RI_MALLOC(sz)
*       #define RI_CALLOC(n,sz)
*       #define RI_REALLOC(pt,sz)
*       #define RI_FREE(pt)
*           Optional allocator overrides. Defaults to <stdlib.h>
*
*   DEPENDENCIES:
*       - C99 standard library only (<stdarg.h>, <stdint.h>, <stdlib.h>, <stdio.h>).
*       - TODO: Add platform-specific headers are pulled in conditionally inside the implementation.
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

// Stringification helpers
#define _RHIO_STR( x )     #x
#define RHIO_STR( x )      _RHIO_STR( x )

// Version
#define RHIO_VERSION_MAJOR 0
#define RHIO_VERSION_MINOR 0
#define RHIO_VERSION_PATCH 2
#define RHIO_VERSION       ( ( RHIO_VERSION_MAJOR << 16 ) | ( RHIO_VERSION_MINOR << 8 ) | RHIO_VERSION_PATCH )
#define RHIO_VERSION_STRING                                                                                            \
    RHIO_STR( RHIO_VERSION_MAJOR ) "." RHIO_STR( RHIO_VERSION_MINOR ) "." RHIO_STR( RHIO_VERSION_PATCH )

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

// Compile-time assertion
#ifndef RHIO_STATIC_ASSERT
#    if defined( __cplusplus ) && ( ( __cplusplus >= 201103L ) || ( defined( _MSC_VER ) && _MSC_VER >= 1600 ) )
#        define RHIO_STATIC_ASSERT( expr, msg ) static_assert( ( expr ), msg )
#    elif !defined( __cplusplus ) && defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 201112L )
#        define RHIO_STATIC_ASSERT( expr, msg ) _Static_assert( ( expr ), msg )
#    else
#        define RHIO_STATIC_ASSERT( expr, msg )                                                                        \
            extern void rhio_static_assertion_failed( char ( * )[( expr ) ? 1 : -1] )
#    endif
#endif

// Asserting
#ifndef RHIO_ASSERT
#    if defined( NDEBUG ) || !defined( RHIO_DEBUG )
#        define RHIO_ASSERT( x ) ( (void)0 )
#    else
#        define RHIO_ASSERT( x )                                                                                       \
            do                                                                                                         \
                {                                                                                                      \
                    if( RI_UNLIKELY( !( x ) ) )                                                                        \
                        {                                                                                              \
                            TRACELOG( RI_LOG_FATAL, "ASSERTION FAILED: '%s' at %s:%d", #x, __FILE__, __LINE__ );       \
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

// Determine if a `riStatus` return value is succeeded or not
// NOTE: RI_SUCCESS and non-error statuses are non-negative
#define RI_SUCCEEDED( result ) ( ( result ) >= RI_SUCCESS )
#define RI_FAILED( result )    ( !RI_SUCCEEDED( result ) )

// Flags operations
#define RI_FLAG_SET( n, f )    ( ( n ) |= ( f ) )
#define RI_FLAG_CLEAR( n, f )  ( ( n ) &= ~( f ) )
#define RI_FLAG_TOGGLE( n, f ) ( ( n ) ^= ( f ) )
#define RI_FLAG_CHECK( n, f )  ( ( n ) & ( f ) )

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

#pragma endregion // Defines and Macros

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

// Device creation flags
typedef enum
{
    RI_DEVICE_FLAG_NONE  = 0,
    RI_DEVICE_FLAG_DEBUG = 1u << 0, // Request backend debug features (validation layers, debug callbacks, ...)

} riDeviceFlag;

// Return status codes for all public API functions
typedef enum
{
    // Success
    RI_SUCCESS = 0, // Operation completed successfully

    // Generic Errors
    RI_ERROR_UNKNOWN       = -1, // Catch-all failure; check logs for details
    RI_ERROR_INVALID_PARAM = -2, // A required argument was NULL or out of range
    RI_ERROR_OUT_OF_MEMORY = -3, // Heap allocation failed

    // Backend & Compilation Errors
    RI_ERROR_BACKEND_INIT    = -4, // The backend failed to initialize
    RI_ERROR_BACKEND_UNAVAIL = -5, // Requested backend not compiled in or not supported
    RI_ERROR_SHADER_COMPILE  = -6, // Shader source or SPIR-V could not be compiled/linked

    // Runtime & State Errors
    RI_ERROR_INVALID_STATE = -7, // Function called in invalid device state (e.g. no active pass)
    RI_ERROR_NOT_READY     = -8, // Resource not yet ready (async or swapchain)
    RI_ERROR_SURFACE_LOST  = -9, // OS/Windowing surface was lost or resized

} riStatus;

// Native window handle types for surface/swapchain creation
typedef enum
{
    RHIO_SWAPCHAIN_HANDLE_TYPE_NONE = 0,    // No native handle (headless or manual surface creation)
    RHIO_SWAPCHAIN_HANDLE_TYPE_HWND,        // Windows: Win32 HWND + HINSTANCE
    RHIO_SWAPCHAIN_HANDLE_TYPE_METAL_LAYER, // macOS/iOS: CAMetalLayer*
    RHIO_SWAPCHAIN_HANDLE_TYPE_X11,         // X11: Display* + Window
    RHIO_SWAPCHAIN_HANDLE_TYPE_WAYLAND,     // Wayland: wl_display* + wl_surface*
    RHIO_SWAPCHAIN_HANDLE_TYPE_XCB,         // XCB: xcb_connection_t* + xcb_window_t

} riSwapchainHandleType;

// Texture formats for requesting swapchain images or creating textures
typedef enum
{
    RHIO_TEXTURE_FORMAT_UNDEFINED = 0, // Invalid or backend-default swapchain format

    // Unsigned Normalized 8-bit formats
    RHIO_TEXTURE_FORMAT_B8G8R8A8_UNORM, // 8-bit BGRA (unsigned normalized)
    RHIO_TEXTURE_FORMAT_B8G8R8A8_SRGB,  // 8-bit BGRA (sRGB non-linear)
    RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM, // 8-bit RGBA (unsigned normalized)
    RHIO_TEXTURE_FORMAT_R8G8B8A8_SRGB,  // 8-bit RGBA (sRGB non-linear)

    // Floating Point formats
    RHIO_TEXTURE_FORMAT_R16G16B16A16_FLOAT, // 16-bit RGBA (half-precision float)
    RHIO_TEXTURE_FORMAT_R32G32B32A32_FLOAT, // 32-bit RGBA (full-precision float)

    // Depth/Stencil formats
    RHIO_TEXTURE_FORMAT_D24_UNORM_S8_UINT, // 24-bit depth (unorm) + 8-bit stencil (uint)
    RHIO_TEXTURE_FORMAT_D32_FLOAT_S8_UINT, // 32-bit depth (float) + 8-bit stencil (uint)

    // Block Compression (Desktop/Console)
    RHIO_TEXTURE_FORMAT_BC1_UNORM, // DXT1/BC1 compressed RGB (unorm)
    RHIO_TEXTURE_FORMAT_BC3_UNORM, // DXT5/BC3 compressed RGBA (unorm)
    RHIO_TEXTURE_FORMAT_BC7_UNORM, // BC7 compressed RGBA (unorm)
    RHIO_TEXTURE_FORMAT_BC1_SRGB,  // DXT1/BC1 compressed RGB (sRGB)
    RHIO_TEXTURE_FORMAT_BC3_SRGB,  // DXT5/BC3 compressed RGBA (sRGB)
    RHIO_TEXTURE_FORMAT_BC7_SRGB,  // BC7 compressed RGBA (sRGB)

    // ASTC Compression (Mobile/Modern Desktop)
    RHIO_TEXTURE_FORMAT_ASTC_4x4_UNORM, // ASTC compressed 4x4 block (unorm)
    RHIO_TEXTURE_FORMAT_ASTC_6x6_UNORM, // ASTC compressed 6x6 block (unorm)
    RHIO_TEXTURE_FORMAT_ASTC_8x8_UNORM, // ASTC compressed 8x8 block (unorm)
    RHIO_TEXTURE_FORMAT_ASTC_4x4_SRGB,  // ASTC compressed 4x4 block (sRGB)
    RHIO_TEXTURE_FORMAT_ASTC_6x6_SRGB,  // ASTC compressed 6x6 block (sRGB)
    RHIO_TEXTURE_FORMAT_ASTC_8x8_SRGB,  // ASTC compressed 8x8 block (sRGB)

} riTextureFormat;

#pragma endregion // Enumerations

//----------------------------------------------------------------------------------
// Opaque resource handles                                             [>>HANDLES<<]
//----------------------------------------------------------------------------------

#pragma region "Opaque resource handles"

// Opaque type macro
#define RHIO_OPAQUE_TYPE( name ) typedef struct name##_opaque * name

// The main RHI device NOTE: Caller-Owned Instance
RHIO_OPAQUE_TYPE( riDevice );

// Command submission objects
RHIO_OPAQUE_TYPE( riCommandQueue );
RHIO_OPAQUE_TYPE( riCommandList );

#undef RHIO_OPAQUE_TYPE

#pragma endregion

//----------------------------------------------------------------------------------
// Types and Structures Definition                                       [>>TYPES<<]
//----------------------------------------------------------------------------------

#pragma region "Types and Structures Definition"

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

// RGBA color in normalized floating-point channels
typedef struct
{
    riF32 r; // Red channel
    riF32 g; // Green channel
    riF32 b; // Blue channel
    riF32 a; // Alpha channel
} riColor;

#if !defined( RHIO_INIT_INFO_TYPE )
// Initialization information
typedef struct
{
    const char * appName; // Application name

} riInitInfo;
#    define RHIO_INIT_INFO_TYPE
#endif

#if !defined( RHIO_BACKEND_INIT_INFO_TYPE )
// Backend initialization information
typedef struct
{
    riInitInfo base;  // Basic application info
    riFlags    flags; // riDeviceFlag bits requested for this device

} riBackendInitInfo;
#    define RHIO_BACKEND_INIT_INFO_TYPE
#endif

// Dynamic Interface - Device dispatch vtable
// NOTE: Every rendering operation is routed through a slot in this struct.
// Backend authors fill this struct and pass it to `rhioCreateDevice()` via riDeviceInfo.
// Built-in backends (OpenGL+ES, Vulkan) are registered automatically.
typedef struct riDeviceVTable
{
    // Lifecycle Management
    //------------------------------------------------------------------------------
    riStatus ( *init )( riDevice device, const riBackendInitInfo * info ); // Initialize backend graphics context
    void     ( *shutdown )( riDevice device ); // Tear down backend resources and free memory

    // Queue Management
    //------------------------------------------------------------------------------
    riStatus ( *create_command_queue )( riDevice device, riCommandQueue * outQueue ); // Initialize a command queue

} riDeviceVTable;

// Command queue dispatch table
typedef struct riCommandQueueVTable
{
    riStatus ( *create_command_list )( riCommandQueue queue, riCommandList * outCommandList ); // Initialize a queue-owned command list
    void     ( *destroy_command_queue )( riCommandQueue queue ); // Tear down backend-owned queue resources
    riStatus ( *submit_command_list )( riCommandQueue queue, riCommandList commandList ); // Submit work
} riCommandQueueVTable;

//----------------------------------------------------------------------------------
// Base Struct Declaration Helpers
//----------------------------------------------------------------------------------
// Automates the creation of base struct: <Name>Base with a single vtable pointer
#define DECLARE_RI_BASE( Name )                                                                                        \
    typedef struct Name##Base                                                                                          \
    {                                                                                                                  \
        const Name##VTable * vtable;                                                                                   \
    } Name##Base

// Base struct definitions
//----------------------------------------------------------
DECLARE_RI_BASE( riDevice );
DECLARE_RI_BASE( riCommandQueue );

#undef DECLARE_RI_BASE

//----------------------------------------------------------------------------------
// Device creation info                                            [>>DEVICE_INFO<<]
//----------------------------------------------------------------------------------

// Device Initialization Information
// NOTE: Holds configuration for creating a rendering device.
// If `vtable` is set, the built-in backend selection is bypassed;
// set `backend` to `RI_BACKEND_CUSTOM`.
typedef struct riDeviceInfo
{
    riInitInfo             base;              // Basic application info (App name, API version)
    riBackend              backend;           // Built-in backend selection token (RI_BACKEND_OPENGL, RI_BACKEND_VULKAN)
    riFlags                flags;             // riDeviceFlag bits requested for this device
    const riDeviceVTable * vtable;            // Custom backend device dispatch (NULL for built-in)
    riSize                 backendDeviceSize; // Bytes to allocate for custom backend device struct

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

// Lifecycle
RI_API riStatus rhioCreateDevice( const riDeviceInfo * info, riDevice * outDevice );
RI_API void     rhioDestroyDevice( riDevice device );

// Command queues
RI_API riStatus rhioCreateCommandQueue( riDevice device, riCommandQueue * outQueue );
RI_API void     rhioDestroyCommandQueue( riCommandQueue queue );
RI_API riStatus rhioCommandQueueSubmit( riCommandQueue queue, riCommandList commandList );

// Command lists
RI_API riStatus rhioCreateCommandList( riCommandQueue queue, riCommandList * outCommandList );

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
    const char *           name;       // Human-readable backend name for logs
    riBackend              backend;    // Resolved backend token
    const riDeviceVTable * vtable;     // Pointer to static backend device dispatch table
    riSize                 deviceSize; // sizeof(Backend_Device) - base struct must be first member
    riFlags                flags;      // riDeviceFlag bits requested for backend initialization

} riBackendDesc;

//----------------------------------------------------------------------------------
// Internal device struct (hidden from callers — Handle pattern)
//
// The public `riDevice` typedef is `struct riDevice_opaque*`.  Backend structs
// expose dispatch by embedding `riDeviceBase` as their first member.
//----------------------------------------------------------------------------------

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
static riStatus _rhioValidateDeviceVTable( const riDeviceVTable * vtable );
static riStatus _rhioValidateCommandQueueVTable( const riCommandQueueVTable * vtable );

// OpenGL family backend registration (desktop GL, OpenGL ES and WebGL)
//----------------------------------------------------------------------------------
#    if defined( RHIO_BACKEND_OPENGL )                                                                                 \
        || defined( RHIO_BACKEND_OPENGLES )                                                                            \
        || defined( RHIO_BACKEND_OPENGLES2 )                                                                           \
        || defined( RHIO_BACKEND_OPENGLES3 )
#        define RHIO_BACKEND_GL_ENABLED
static riStatus _rhioGL_registerBackend( riBackendDesc * desc );
#    endif

// Vulkan register backend vtable
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
            ST( ERROR_SURFACE_LOST );
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
    riStatus          status          = RI_ERROR_UNKNOWN;
    riBackendDesc     desc            = RI_ZERO_INIT;
    riBackendInitInfo backendInitInfo = RI_ZERO_INIT;
    riDevice          device          = NULL;
    riDeviceBase *    deviceBase      = NULL;
    riSize            deviceSize      = 0;

    TRACELOG( RI_LOG_INFO, "Initializing rhio %s", RHIO_VERSION_STRING );

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD_NULL( outDevice, RI_ERROR_INVALID_PARAM );
    *outDevice = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    // Resolve Backend Info
    //----------------------------------------------------------
    status = _rhioResolveBackendDesc( info, &desc );
    if( RI_FAILED( status ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Failed to resolve backend descriptor: %d", status );
            goto fail;
        }

    // Backend Vtable Validation
    //----------------------------------------------------------
    // Catch incomplete custom backends before allocating backend-private state
    status = _rhioValidateDeviceVTable( desc.vtable );
    if( RI_FAILED( status ) ) goto fail;

    // Device Allocation
    //----------------------------------------------------------
    // Allocate the backend-specific struct, which MUST have riDeviceBase as its first member
    deviceSize = desc.deviceSize > 0 ? desc.deviceSize : (riSize)sizeof( riDeviceBase );
    if( RI_UNLIKELY( deviceSize < (riSize)sizeof( riDeviceBase ) ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Backend device size is smaller than riDeviceBase" );
            status = RI_ERROR_INVALID_PARAM;
            goto fail;
        }

    if( RI_UNLIKELY( deviceSize > (riSize)( (size_t)-1 ) ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Backend device allocation size is too large" );
            status = RI_ERROR_OUT_OF_MEMORY;
            goto fail;
        }

    deviceBase = (riDeviceBase *)RI_CALLOC( 1, (size_t)deviceSize );
    if( RI_UNLIKELY( deviceBase == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Out of memory in %s", __func__ );
            status = RI_ERROR_OUT_OF_MEMORY;
            goto fail;
        }

    // Device State Initialization
    //----------------------------------------------------------
    deviceBase->vtable = desc.vtable;
    device             = (riDevice)deviceBase;

    // Backend Initialization
    //----------------------------------------------------------
    // Backend receives its allocated private state and init info.
    // NOTE: shutdown is called on init failure so backends can centralize cleanup.
    backendInitInfo.base  = info->base;
    backendInitInfo.flags = desc.flags;

    // Call vtable init
    status = deviceBase->vtable->init( device, &backendInitInfo );
    if( RI_FAILED( status ) )
        {
            TRACELOG(
                RI_LOG_ERROR, "DEVICE: Backend initialization failed: %s (%d)", riStatusToString( status ), status );
            goto fail;
        }

    // Output Handle Handoff
    //----------------------------------------------------------
    *outDevice = device;

    TRACELOG( RI_LOG_INFO,
              "DEVICE: Created successfully using %s backend",
              STR_NONEMPTY( desc.name ) ? desc.name : "unknown" );

    return RI_SUCCESS;

fail:
    // Failure Cleanup
    //----------------------------------------------------------
    if( NULL != device )
        {
            deviceBase = (riDeviceBase *)device;
            if( deviceBase->vtable != NULL && deviceBase->vtable->shutdown != NULL )
                {
                    deviceBase->vtable->shutdown( device );
                }

            RI_FREE( device );
        }

    return status;
}

// Tear down a graphics device and all associated resources
RI_API void
rhioDestroyDevice( riDevice device )
{
    riDeviceBase * deviceBase = NULL;

    RI_GUARD_NULL_VOID( device );

    deviceBase = (riDeviceBase *)device;

    // Backend Shutdown
    //----------------------------------------------------------
    // NOTE: shutdown receives the base struct handle
    if( deviceBase->vtable != NULL && deviceBase->vtable->shutdown != NULL )
        {
            deviceBase->vtable->shutdown( device );
        }

    // Device Memory Cleanup
    //----------------------------------------------------------
    // device points to the base of the backend struct, allocated in a single block.
    RI_FREE( device );

    TRACELOG( RI_LOG_INFO, "DEVICE: Destroyed successfully" );
}

#    pragma endregion // Lifecycle

/* ---------------------------------------------------------------------------------
 *
 * COMMAND QUEUES                                                 [>>COMMAND_QUEUE<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Command Queues"

// Create a command queue wrapper bound to a device backend
RI_API riStatus
rhioCreateCommandQueue( riDevice device, riCommandQueue * outQueue )
{
    riDeviceBase *       deviceBase = NULL;
    riCommandQueueBase * queueBase  = NULL;
    riStatus             status     = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD_NULL( outQueue, RI_ERROR_INVALID_PARAM );
    *outQueue = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( deviceBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( deviceBase->vtable->create_command_queue, RI_ERROR_INVALID_STATE );

    // Backend Queue Creation
    //----------------------------------------------------------
    // Backend allocates the full structure, sets the base fields, and returns it
    status = deviceBase->vtable->create_command_queue( device, outQueue );
    if( RI_FAILED( status ) )
        {
            TRACELOG(
                RI_LOG_ERROR, "COMMAND_QUEUE: Backend creation failed: %s (%d)", riStatusToString( status ), status );
            *outQueue = NULL;
            return status;
        }

    RI_GUARD_NULL( *outQueue, RI_ERROR_INVALID_STATE );

    // Command Queue Dispatch Validation
    //----------------------------------------------------------
    queueBase = (riCommandQueueBase *)*outQueue;
    status    = _rhioValidateCommandQueueVTable( queueBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( queueBase->vtable != NULL && queueBase->vtable->destroy_command_queue != NULL )
                {
                    queueBase->vtable->destroy_command_queue( *outQueue );
                }

            RI_FREE( *outQueue );
            *outQueue = NULL;
            return status;
        }

    TRACELOG( RI_LOG_INFO, "COMMAND_QUEUE: Created successfully" );

    return RI_SUCCESS;
}

// Destroy a command queue wrapper and release queue resources
RI_API void
rhioDestroyCommandQueue( riCommandQueue queue )
{
    riCommandQueueBase * queueBase = NULL;

    if( NULL == queue ) return;

    queueBase = (riCommandQueueBase *)queue;

    if( NULL != queueBase->vtable && NULL != queueBase->vtable->destroy_command_queue )
        {
            queueBase->vtable->destroy_command_queue( queue );
        }

    RI_FREE( queue );

    TRACELOG( RI_LOG_INFO, "COMMAND_QUEUE: Destroyed successfully" );
}

// Create a command list through the queue backend that will submit it
RI_API riStatus
rhioCreateCommandList( riCommandQueue queue, riCommandList * outCommandList )
{
    riCommandQueueBase * queueBase = NULL;
    riStatus             status    = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD_NULL( outCommandList, RI_ERROR_INVALID_PARAM );
    *outCommandList = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    queueBase = (riCommandQueueBase *)queue;

    // Queue Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( queueBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( queueBase->vtable->create_command_list, RI_ERROR_INVALID_STATE );

    // Backend Command List Creation
    //----------------------------------------------------------
    // Command lists are created from queues so backends can bind queue-family state.
    status = queueBase->vtable->create_command_list( queue, outCommandList );
    if( RI_FAILED( status ) )
        {
            TRACELOG(
                RI_LOG_ERROR, "COMMAND_LIST: Backend creation failed: %s (%d)", riStatusToString( status ), status );
            *outCommandList = NULL;
            return status;
        }

    RI_GUARD_NULL( *outCommandList, RI_ERROR_INVALID_STATE );

    TRACELOG( RI_LOG_INFO, "COMMAND_LIST: Created successfully" );

    return RI_SUCCESS;
}

// Submit one command list through a command queue
RI_API riStatus
rhioCommandQueueSubmit( riCommandQueue queue, riCommandList commandList )
{
    riCommandQueueBase * queueBase = NULL;

    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

    queueBase = (riCommandQueueBase *)queue;

    RI_GUARD_NULL( queueBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( queueBase->vtable->submit_command_list, RI_ERROR_INVALID_STATE );

    return queueBase->vtable->submit_command_list( queue, commandList );
}

#    pragma endregion // Command Queues

#    pragma endregion // API Impl

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

#    else
    UNUSED( logType );
    UNUSED( text );
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

#    if defined( RHIO_BACKEND_GL_ENABLED )

//----------------------------------------------------------------------------------
// Platform GL Headers and Entry Points
//----------------------------------------------------------------------------------
// NOTE: rhio does not own an OpenGL context. The application/windowing owns it.
// This block provides only the minimal types and headers.

// Emscripten / WebGL
//------------------------------------------------------------------------------
#        if defined( __EMSCRIPTEN__ )

// NOTE: Emscripten/WebGL exposes GLES symbols directly, so optional entry points
// can be referenced without a runtime loader

#            if !defined( GL_GLEXT_PROTOTYPES )
#                define GL_GLEXT_PROTOTYPES 1
#            endif

#            if defined( RHIO_BACKEND_OPENGLES2 )
#                include <GLES2/gl2.h>
#            else
#                include <GLES3/gl3.h>
#            endif
#            include <GLES2/gl2ext.h> // ES extension prototypes used by both GLES2 and GLES3

#            include <emscripten/emscripten.h>

typedef void * riGL_DeviceContext;
typedef void * riGL_RenderContext;

#            define _RHIO_GL_LOAD( type, name ) ( (type *)name )

// Win32 / WGL
//------------------------------------------------------------------------------
#        elif defined( _WIN32 )

// NOTE: Win32 uses WGL for context handles and extension lookup. windows.h must
// be included before GL/gl.h so HDC/HGLRC are available to the OpenGL header

#            if !defined( WIN32_LEAN_AND_MEAN )
#                define WIN32_LEAN_AND_MEAN
#            endif
#            if !defined( NOMINMAX )
#                define NOMINMAX
#            endif

#            include <GL/gl.h>
#            include <windows.h>

typedef HDC   riGL_DeviceContext;
typedef HGLRC riGL_RenderContext;

#            define _RHIO_GL_LOAD( type, name ) ( (type *)wglGetProcAddress( #name ) )

// Apple
//------------------------------------------------------------------------------
#        elif defined( __APPLE__ )

// NOTE: Apple platforms link GL/GLES entry points directly. Context creation is
// still owned by the host windowing layer

#            define GL_SILENCE_DEPRECATION

#            if defined( RHIO_BACKEND_OPENGLES ) || defined( RHIO_BACKEND_OPENGLES2 )
#                include <OpenGLES/ES2/gl.h>
#            elif defined( RHIO_BACKEND_OPENGLES3 )
#                include <OpenGLES/ES3/gl.h>
#            else
#                include <OpenGL/gl3.h>
#            endif

typedef void * riGL_DeviceContext;
typedef void * riGL_RenderContext;

#            define _RHIO_GL_LOAD( type, name ) ( (type *)name )

// Unix-like (Linux / *BSD) / GLX + EGL
//------------------------------------------------------------------------------
#        elif defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ )

// NOTE: Unix-like desktop GL uses GLX for extension lookup. GLES builds generally
// expose the symbols directly through EGL or the platform loader used by app

#            if defined( RHIO_BACKEND_OPENGLES )                                                                       \
                || defined( RHIO_BACKEND_OPENGLES2 )                                                                   \
                || defined( RHIO_BACKEND_OPENGLES3 )

#                if !defined( GL_GLEXT_PROTOTYPES )
#                    define GL_GLEXT_PROTOTYPES 1
#                endif
#                include <EGL/egl.h>
#                if defined( RHIO_BACKEND_OPENGLES3 )
#                    include <GLES3/gl3.h> // OpenGL ES 3.0 core API
#                else
#                    include <GLES2/gl2.h> // OpenGL ES 2.0 core API
#                endif

#                include <GLES2/gl2ext.h>  // ES extension prototypes used by both GLES2 and GLES3

typedef void * riGL_DeviceContext;
typedef void * riGL_RenderContext;

#                define _RHIO_GL_LOAD( type, name ) ( (type *)name )

#            else /* RHIO_BACKEND_OPENGLES */

#                include <GL/gl.h>
#                include <GL/glext.h>
#                include <GL/glx.h>
#                include <X11/Xlib.h>

typedef Display *  riGL_DeviceContext;
typedef GLXContext riGL_RenderContext;

#                define _RHIO_GL_LOAD( type, name ) ( (type *)glXGetProcAddress( (const GLubyte *)#name ) )

#            endif /* RHIO_BACKEND_OPENGLES || RHIO_BACKEND_OPENGLES2 || RHIO_BACKEND_OPENGLES3 */

// Unsupported
//------------------------------------------------------------------------------
#        else
#            error "RHIO GL backend: unsupported platform"
#        endif     /* GL platform selection */

//----------------------------------------------------------------------------------
// Compatibility Defines
//----------------------------------------------------------------------------------

#        if !defined( GL_SHADING_LANGUAGE_VERSION )
#            define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#        endif

#        if !defined( GL_CLAMP ) && defined( GL_CLAMP_TO_EDGE )
#            define GL_CLAMP GL_CLAMP_TO_EDGE
#        endif

#        if defined( RHIO_BACKEND_OPENGLES2 )
#            define glClearDepth glClearDepthf
#        endif

//----------------------------------------------------------------------------------
// Types and Structures Definition                                    [>>GL_TYPES<<]
//----------------------------------------------------------------------------------

// Internal OpenGL device state
typedef struct riGL_Device
{
    riDeviceBase base; // Frontend handle dispatch

} riGL_Device;

typedef struct riGL_CommandList riGL_CommandList;

typedef struct riGL_CommandQueue
{
    riCommandQueueBase base;         // Frontend handle dispatch
    riGL_CommandList * commandLists; // Queue-owned command lists

} riGL_CommandQueue;

struct riGL_CommandList
{
    riGL_CommandList * next;  // Next queue-owned command list
    riCommandQueue     queue; // Owning queue for future GL command-buffer emulation

};

// Device
static riStatus _rhioGL_init( riDevice device, const riBackendInitInfo * info );
static void     _rhioGL_shutdown( riDevice device );

// Command queue
static riStatus _rhioGL_create_command_queue( riDevice device, riCommandQueue * outQueue );
static riStatus _rhioGL_create_command_list( riCommandQueue queue, riCommandList * outCommandList );
static void     _rhioGL_destroy_command_queue( riCommandQueue queue );
static riStatus _rhioGL_submit_command_list( riCommandQueue queue, riCommandList commandList );

//----------------------------------------------------------------------------------
// OpenGL Backend Vtable
//----------------------------------------------------------------------------------
#        define BIND_FUNC( name ) .name = _rhioGL_##name

static const riDeviceVTable s_gl_vtable = {
    BIND_FUNC( init ),
    BIND_FUNC( shutdown ),
    BIND_FUNC( create_command_queue ),
};

static const riCommandQueueVTable s_gl_command_queue_vtable = {
    BIND_FUNC( create_command_list ),
    BIND_FUNC( destroy_command_queue ),
    BIND_FUNC( submit_command_list ),
};

#        undef BIND_FUNC

//----------------------------------------------------------------------------------
// OpenGL Backend Implementation                                       [>>GL_IMPL<<]
//----------------------------------------------------------------------------------

// Initialize OpenGL state and allocate default resources
static riStatus
_rhioGL_init( riDevice device, const riBackendInitInfo * info )
{
    const GLubyte * vendor      = NULL;
    const GLubyte * renderer    = NULL;
    const GLubyte * version     = NULL;
    const GLubyte * glslVersion = NULL;

    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    UNUSED( info );

    // Check OpenGL
    //------------------------------------------------------------------------------
    // Validate the core precondition
    version = glGetString( GL_VERSION );
    if( RI_UNLIKELY( NULL == version ) )
        {
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: No current OpenGL context" );
            return RI_ERROR_BACKEND_INIT;
        }

    vendor      = glGetString( GL_VENDOR );
    renderer    = glGetString( GL_RENDERER );
    glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

    TRACELOG( RI_LOG_INFO, "BACKEND GL: Initialized context" );
    TRACELOG( RI_LOG_INFO, "    > Version:  %s", version );
    TRACELOG( RI_LOG_INFO, "    > Renderer: %s", renderer );
    TRACELOG( RI_LOG_INFO, "    > Vendor:   %s", vendor );
    TRACELOG( RI_LOG_INFO, "    > GLSL:     %s", glslVersion );
    //------------------------------------------------------------------------------

    return RI_SUCCESS;
}

// Free OpenGL resources and tear down device state
static void
_rhioGL_shutdown( riDevice device )
{
    riGL_Device   emptyDevice = RI_ZERO_INIT;
    riGL_Device * glDevice    = (riGL_Device *)device;

    if( NULL != glDevice )
        {
            emptyDevice.base = glDevice->base; // preserve base struct!
            *glDevice        = emptyDevice;
        }

    TRACELOG( RI_LOG_INFO, "BACKEND GL: Shutdown complete" );
}

// Initialize an OpenGL command queue wrapper
static riStatus
_rhioGL_create_command_queue( riDevice device, riCommandQueue * outQueue )
{
    riGL_CommandQueue * glQueue = NULL;

    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outQueue, RI_ERROR_INVALID_PARAM );

    // Queue Allocation
    //----------------------------------------------------------
    glQueue = (riGL_CommandQueue *)RI_CALLOC( 1, sizeof( *glQueue ) );
    if( RI_UNLIKELY( NULL == glQueue ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    // Queue Dispatch Initialization
    //----------------------------------------------------------
    glQueue->base.vtable = &s_gl_command_queue_vtable;

    // Queue Handle Handoff
    //----------------------------------------------------------
    *outQueue = (riCommandQueue)glQueue;

    return RI_SUCCESS;
}

// Initialize an OpenGL command list wrapper
static riStatus
_rhioGL_create_command_list( riCommandQueue queue, riCommandList * outCommandList )
{
    riGL_CommandQueue * glQueue       = NULL;
    riGL_CommandList *  glCommandList = NULL;

    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outCommandList, RI_ERROR_INVALID_PARAM );

    glQueue = (riGL_CommandQueue *)queue;

    // Command List Allocation
    //----------------------------------------------------------
    glCommandList = (riGL_CommandList *)RI_CALLOC( 1, sizeof( *glCommandList ) );
    if( RI_UNLIKELY( NULL == glCommandList ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    // Command List State Initialization
    //----------------------------------------------------------
    glCommandList->queue  = queue;
    glCommandList->next   = glQueue->commandLists;
    glQueue->commandLists = glCommandList;

    // Command List Handle Handoff
    //----------------------------------------------------------
    *outCommandList = (riCommandList)glCommandList;

    return RI_SUCCESS;
}

// Tear down OpenGL command queue state
static void
_rhioGL_destroy_command_queue( riCommandQueue queue )
{
    riGL_CommandQueue * glQueue     = (riGL_CommandQueue *)queue;
    riGL_CommandList *  commandList = NULL;

    RI_GUARD_NULL_VOID( glQueue );

    // Command List Cleanup
    //----------------------------------------------------------
    while( NULL != glQueue->commandLists )
        {
            commandList           = glQueue->commandLists;
            glQueue->commandLists = commandList->next;
            RI_FREE( commandList );
        }
}

// Submit one command list to the OpenGL backend
static riStatus
_rhioGL_submit_command_list( riCommandQueue queue, riCommandList commandList )
{
    UNUSED( commandList );
    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );

    return RI_SUCCESS;
}

// Populate the backend descriptor with OpenGL implementations
static riStatus
_rhioGL_registerBackend( riBackendDesc * desc )
{
    riBackend backend = RI_BACKEND_OPENGL;
    riFlags   flags   = 0;

    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    flags = desc->flags;

    // Preserve the requested GL flavor before clearing the descriptor
    if( desc->backend == RI_BACKEND_OPENGLES )
        {
            backend = RI_BACKEND_OPENGLES;
        }

    _rhioBackendDescInit( desc );

    // Backend Descriptor Setup
    //----------------------------------------------------------
    desc->name       = ( backend == RI_BACKEND_OPENGLES ) ? "OpenGL ES" : "OpenGL";
    desc->backend    = backend;
    desc->deviceSize = (riSize)sizeof( riGL_Device );
    desc->flags      = flags;

    // VTable
    desc->vtable = &s_gl_vtable;

    return RI_SUCCESS;
}

#    endif            /* RHIO_BACKEND_GL_ENABLED */

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

//----------------------------------------------------------------------------------
// Vulkan Backend Vtable
//----------------------------------------------------------------------------------
#        define BIND_FUNC( name ) .name = _rhioVK_##name

static const riDeviceVTable s_vk_vtable = RI_ZERO_INIT;

#        undef BIND_FUNC

// Fill Vulkan descriptor metadata
static riStatus
_rhioVK_registerBackend( riBackendDesc * desc )
{
    riFlags flags = 0;

    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    flags = desc->flags;

    _rhioBackendDescInit( desc );

    // Backend Descriptor Setup
    //----------------------------------------------------------
    desc->name    = "Vulkan";
    desc->backend = RI_BACKEND_VULKAN;
    desc->flags   = flags;

    // VTable
    desc->vtable = &s_vk_vtable;

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
_rhioValidateDeviceVTable( const riDeviceVTable * vtable )
{
    RI_GUARD_NULL( vtable, RI_ERROR_INVALID_PARAM );

    // Required backend callbacks
    //----------------------------------------------------------
    // NOTE: Public API calls dispatch through these slots after device creation.
#    define CHECK_VTABLE_SLOT( name )                                                                                  \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( vtable->name == NULL ) )                                                              \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "BACKEND: Missing required vtable slot '%s'", #name );                 \
                        return RI_ERROR_INVALID_PARAM;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_VTABLE_SLOT( init );
    CHECK_VTABLE_SLOT( shutdown );
    CHECK_VTABLE_SLOT( create_command_queue );

#    undef CHECK_VTABLE_SLOT

    // TODO: Validate required resource dispatch slots here when buffers,
    // textures, samplers, shaders, render passes, and command lists are added.

    return RI_SUCCESS;
}

// Ensure a command queue exposes callbacks required by the public queue API
static riStatus
_rhioValidateCommandQueueVTable( const riCommandQueueVTable * vtable )
{
    if( RI_UNLIKELY( vtable == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "COMMAND_QUEUE: Missing dispatch table" );
            return RI_ERROR_INVALID_STATE;
        }

#    define CHECK_QUEUE_VTABLE_SLOT( name )                                                                            \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( vtable->name == NULL ) )                                                              \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "COMMAND_QUEUE: Missing required vtable slot '%s'", #name );           \
                        return RI_ERROR_INVALID_STATE;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_QUEUE_VTABLE_SLOT( create_command_list );
    CHECK_QUEUE_VTABLE_SLOT( destroy_command_queue );
    CHECK_QUEUE_VTABLE_SLOT( submit_command_list );

#    undef CHECK_QUEUE_VTABLE_SLOT

    return RI_SUCCESS;
}

// Convert device creation info into a concrete backend descriptor
static riStatus
_rhioResolveBackendDesc( const riDeviceInfo * info, riBackendDesc * desc )
{
    const riFlags validFlags = (riFlags)RI_DEVICE_FLAG_DEBUG;

    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

    _rhioBackendDescInit( desc );

    if( RI_UNLIKELY( RI_FLAG_CHECK( info->flags, ~validFlags ) != 0 ) )
        {
            TRACELOG( RI_LOG_ERROR, "DEVICE: Invalid device flags: 0x%08x", (unsigned int)info->flags );
            return RI_ERROR_INVALID_PARAM;
        }

    // Custom backend
    //----------------------------------------------------------
    // NOTE: A custom vtable takes precedence over info->backend. Missing required
    // slots are reported by `_rhioValidateDeviceVTable()`.
    if( info->vtable != NULL )
        {
            desc->name       = "Custom";
            desc->backend    = RI_BACKEND_CUSTOM;
            desc->vtable     = info->vtable;
            desc->deviceSize = info->backendDeviceSize;
            desc->flags      = info->flags;
            return RI_SUCCESS;
        }

    // Built-in backend
    //----------------------------------------------------------
    // NOTE: Only backends compiled into this translation unit can register here.
    desc->flags = info->flags;

    switch( info->backend )
        {
#    if defined( RHIO_BACKEND_GL_ENABLED )

            // Built-in: OpenGL / OpenGL ES
            //----------------------------------------------------------
#        if defined( RHIO_BACKEND_OPENGL )
        case RI_BACKEND_OPENGL: desc->backend = info->backend; return _rhioGL_registerBackend( desc );
#        endif

#        if defined( RHIO_BACKEND_OPENGLES ) || defined( RHIO_BACKEND_OPENGLES2 ) || defined( RHIO_BACKEND_OPENGLES3 )
        case RI_BACKEND_OPENGLES: desc->backend = info->backend; return _rhioGL_registerBackend( desc );
#        endif
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
