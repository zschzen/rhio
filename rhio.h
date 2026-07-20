/**********************************************************************************************
*
*     ▌ ▘   v0.1.0
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
*       - C99 standard library (<stdarg.h>, <stdint.h>, <stdlib.h>, <stdio.h>).
*       - Desktop OpenGL builds require GLEW; the CMake target wires glew-cmake automatically.
*       - Platform GL/GLES/Vulkan SDK headers are pulled in conditionally inside the implementation.
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
#define RHIO_VERSION_MINOR 1
#define RHIO_VERSION_PATCH 0
#define RHIO_VERSION       ( ( RHIO_VERSION_MAJOR << 16 ) | ( RHIO_VERSION_MINOR << 8 ) | RHIO_VERSION_PATCH )
#define RHIO_VERSION_STRING                                                                                            \
    RHIO_STR( RHIO_VERSION_MAJOR ) "." RHIO_STR( RHIO_VERSION_MINOR ) "." RHIO_STR( RHIO_VERSION_PATCH )

// Compile-time resource limits
#ifndef RHIO_MAX_COLOR_ATTACHMENTS
#    define RHIO_MAX_COLOR_ATTACHMENTS 8
#endif

#ifndef RHIO_MAX_SHADER_BUFFER_BINDINGS
#    define RHIO_MAX_SHADER_BUFFER_BINDINGS 8
#endif

#ifndef RHIO_TEXTURE_VIEW_ALL_MIPS
#    define RHIO_TEXTURE_VIEW_ALL_MIPS 0xFFFFFFFFu
#endif

#ifndef RHIO_TEXTURE_VIEW_ALL_ARRAY_LAYERS
#    define RHIO_TEXTURE_VIEW_ALL_ARRAY_LAYERS 0xFFFFFFFFu
#endif

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
#define RI_SUCCEEDED( result )      ( ( result ) >= RI_SUCCESS )
#define RI_FAILED( result )         ( !RI_SUCCEEDED( result ) )

// Determine if a `riStatus` return value failed on an expected-success path
// NOTE: Branch prediction hint applied
#define RI_SHOULD_SUCCEED( result ) RI_UNLIKELY( RI_FAILED( result ) )

// Flags operations
#define RI_FLAG_SET( n, f )         ( ( n ) |= ( f ) )
#define RI_FLAG_CLEAR( n, f )       ( ( n ) &= ~( f ) )
#define RI_FLAG_TOGGLE( n, f )      ( ( n ) ^= ( f ) )
#define RI_FLAG_CHECK( n, f )       ( ( n ) & ( f ) )

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
#define RI_GUARD( cond, retval )                                                                                       \
    do                                                                                                                 \
        {                                                                                                              \
            if( !( cond ) )                                                                                            \
                {                                                                                                      \
                    return retval;                                                                                     \
                }                                                                                                      \
        }                                                                                                              \
    while( 0 )

#define RI_GUARD_VOID( cond ) RI_GUARD( cond, )

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

// Swapchain presentation behavior flags
typedef enum
{
    RI_SWAPCHAIN_PRESENT_FLAG_NONE        = 0,
    RI_SWAPCHAIN_PRESENT_FLAG_FORCE_FLUSH = 1u << 0, // Request an explicit backend flush before native present

} riSwapchainPresentFlag;

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

// Texture usage flags
typedef enum
{
    RI_TEXTURE_USAGE_NONE          = 0,
    RI_TEXTURE_USAGE_SAMPLED       = 1u << 0, // Texture can be read by shaders
    RI_TEXTURE_USAGE_STORAGE       = 1u << 1, // Texture can be written by shaders
    RI_TEXTURE_USAGE_RENDER_TARGET = 1u << 2, // Texture can be used as a color attachment
    RI_TEXTURE_USAGE_DEPTH_STENCIL = 1u << 3, // Texture can be used as depth/stencil attachment

} riTextureUsage;

// Buffer usage flags
typedef enum
{
    RI_BUFFER_USAGE_NONE        = 0,
    RI_BUFFER_USAGE_SHADER_READ = 1u << 0, // Buffer can be read by shaders

} riBufferUsage;

// Texture dimensionality for concrete textures and views
typedef enum
{
    RI_TEXTURE_DIMENSIONS_UNDEFINED = 0,
    RI_TEXTURE_DIMENSIONS_1D,       // Texture1D
    RI_TEXTURE_DIMENSIONS_2D,       //Texture2D
    RI_TEXTURE_DIMENSIONS_2D_ARRAY, //Texture2DArray
    RI_TEXTURE_DIMENSIONS_3D,       //Texture3D
    RI_TEXTURE_DIMENSIONS_CUBE,     // TextureCube

} riTextureDimensions;

// Attachment load operation at render-pass begin
typedef enum
{
    RI_ATTACHMENT_LOAD_ACTION_LOAD = 0, // Preserve existing attachment contents
    RI_ATTACHMENT_LOAD_ACTION_CLEAR,    // Clear attachment before rendering
    RI_ATTACHMENT_LOAD_ACTION_DONT_CARE // Existing contents are undefined

} riAttachmentLoadAction;

// Attachment store operation at render-pass end
typedef enum
{
    RI_ATTACHMENT_STORE_ACTION_STORE = 0, // Keep rendered attachment contents
    RI_ATTACHMENT_STORE_ACTION_DONT_CARE  // Final contents are undefined

} riAttachmentStoreAction;

// Primitive assembly topology
typedef enum riPipelineTopology
{
    RI_PIPELINE_TOPOLOGY_TRIANGLE_LIST = 0, // Independent triangles
    RI_PIPELINE_TOPOLOGY_LINE_LIST,         // Independent line segments
    RI_PIPELINE_TOPOLOGY_POINT_LIST,        // Independent points

} riPipelineTopology;

// Rasterization fill mode
typedef enum riPipelineFillMode
{
    RI_PIPELINE_FILL_MODE_SOLID = 0,
    RI_PIPELINE_FILL_MODE_WIREFRAME,

} riPipelineFillMode;

// Rasterization culling mode
typedef enum riPipelineCullMode
{
    RI_PIPELINE_CULL_MODE_NONE = 0,
    RI_PIPELINE_CULL_MODE_FRONT,
    RI_PIPELINE_CULL_MODE_BACK,

} riPipelineCullMode;

// Rasterization front-face winding
typedef enum riPipelineFrontFace
{
    RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    RI_PIPELINE_FRONT_FACE_CLOCKWISE,

} riPipelineFrontFace;

// Depth/stencil comparison operation
typedef enum riCompareOperation
{
    RI_COMPARE_OPERATION_NEVER = 0,
    RI_COMPARE_OPERATION_LESS,
    RI_COMPARE_OPERATION_EQUAL,
    RI_COMPARE_OPERATION_LESS_EQUAL,
    RI_COMPARE_OPERATION_GREATER,
    RI_COMPARE_OPERATION_NOT_EQUAL,
    RI_COMPARE_OPERATION_GREATER_EQUAL,
    RI_COMPARE_OPERATION_ALWAYS,

} riCompareOperation;

// Color blending factor
typedef enum riBlendFactor
{
    RI_BLEND_FACTOR_ZERO = 0,
    RI_BLEND_FACTOR_ONE,
    RI_BLEND_FACTOR_SRC_COLOR,
    RI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    RI_BLEND_FACTOR_DST_COLOR,
    RI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    RI_BLEND_FACTOR_SRC_ALPHA,
    RI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    RI_BLEND_FACTOR_DST_ALPHA,
    RI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    RI_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    RI_BLEND_FACTOR_BLEND_COLOR,

} riBlendFactor;

// Color blending operation
typedef enum riBlendOperation
{
    RI_BLEND_OPERATION_ADD = 0,
    RI_BLEND_OPERATION_SUBTRACT,
    RI_BLEND_OPERATION_REVERSE_SUBTRACT,
    RI_BLEND_OPERATION_MIN,
    RI_BLEND_OPERATION_MAX,

} riBlendOperation;

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

// GPU resources
RHIO_OPAQUE_TYPE( riBuffer );
RHIO_OPAQUE_TYPE( riShaderModule );
RHIO_OPAQUE_TYPE( riRenderPipeline );

// Display and render-target objects
RHIO_OPAQUE_TYPE( riSwapchain );
RHIO_OPAQUE_TYPE( riTexture );
RHIO_OPAQUE_TYPE( riTextureView );
RHIO_OPAQUE_TYPE( riRenderPass );

#undef RHIO_OPAQUE_TYPE

#pragma endregion

//----------------------------------------------------------------------------------
// Types and Structures Definition                                       [>>TYPES<<]
//----------------------------------------------------------------------------------

#pragma region "Types and Structures Definition"

// Type aliases
typedef uint8_t   riU8;
typedef uint16_t  riU16;
typedef uint32_t  riU32;
typedef uint64_t  riU64;
typedef int8_t    riI8;
typedef int16_t   riI16;
typedef int32_t   riI32;
typedef int64_t   riI64;
typedef float     riF32;
typedef double    riF64;
typedef riU32     riFlags; /* bitfield type used for usage/feature flags */
typedef riU64     riSize;
typedef uintptr_t riUPtr;

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

// Optional callback used when RHIO does not own native presentation
// OpenGL uses this hook to let the application swap the host window buffers
typedef riStatus ( *riSwapchainPresentCallback )( riSwapchain swapchain, void * userData );

// Native swapchain/display target creation parameters
typedef struct riSwapchainInfo
{
    riSwapchainHandleType handleType; // Native handle kind
    union
    {
        struct
        {
            void * instance; // HINSTANCE
            void * window;   // HWND
        } hwnd;

        void * metalLayer;   // CAMetalLayer*

        struct
        {
            void * display; // Display*
            riUPtr window;  // Window
        } x11;

        struct
        {
            void * display; // wl_display*
            void * surface; // wl_surface*
        } wayland;

        struct
        {
            void * connection; // xcb_connection_t*
            riU32  window;     // xcb_window_t
        } xcb;
    } handle;

    riU32           width;                      // Current framebuffer width in pixels
    riU32           height;                     // Current framebuffer height in pixels
    riTextureFormat format;                     // Preferred display format, or UNDEFINED for backend default
    riU8            maxFramesInFlight;          // Backend-specific frame depth hint
    riFlags         presentFlags;               // riSwapchainPresentFlag bits

    riSwapchainPresentCallback presentCallback; // Optional native present hook
    void *                     presentUserData; // Passed to presentCallback

} riSwapchainInfo;

// Texture metadata used by swapchain images and future user-created textures
typedef struct riTextureInfo
{
    riU32               width;       // Width in pixels
    riU32               height;      // Height in pixels
    riU32               depth;       // Depth for 3D textures, otherwise 1
    riU32               mipLevels;   // Number of mip levels
    riU32               arrayLayers; // Array/cube layer count
    riTextureFormat     format;      // Texture storage format
    riFlags             usage;       // riTextureUsage bits
    riTextureDimensions dimensions;  // Texture dimensionality
    const char *        name;        // Optional debug name, not retained by core

} riTextureInfo;

// Texture view creation parameters
typedef struct riTextureViewInfo
{
    riTexture texture;              // Source texture

    riU32 baseMipLevel;             // First mip level
    riU32 mipLevelCount;            // Number of mip levels, 0 or ALL_MIPS for all available
    riU32 baseArrayLayer;           // First array/cube layer
    riU32 arrayLayerCount;          // Number of layers, 0 or ALL_ARRAY_LAYERS for all available

    riTextureFormat     format;     // UNDEFINED inherits the texture format
    riFlags             usage;      // 0 inherits the texture usage
    riTextureDimensions dimensions; // UNDEFINED inherits the texture dimensions

} riTextureViewInfo;

// Buffer creation parameters
typedef struct riBufferInfo
{
    riSize       size;  // Buffer size in bytes
    riFlags      usage; // riBufferUsage bits
    const void * data;  // Optional initial contents; not retained by RHIO
    const char * name;  // Optional debug name, not retained by core

} riBufferInfo;

// Render pipeline creation parameters
typedef struct riRenderPipelineInfo
{
    // General info
    riU8 supportsIndirectCommands;

    // Rasterizer
    riPipelineFillMode  fillMode;
    riPipelineCullMode  cullMode;
    riPipelineFrontFace frontFace;
    riPipelineTopology  topology;

    // Depth-stencil
    riU8               depthTestEnable;
    riU8               depthWriteEnable;
    riU8               depthClampEnable;
    riCompareOperation depthCompareOp;
    riU8               stencilTestEnable;
    riU8               stencilWriteEnable;
    riCompareOperation stencilCompareOp;
    riTextureFormat    depthStencilFormat;

    // Blending and color output
    riU8             blendEnable[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendFactor    blendSrcRgbFactor[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendFactor    blendDstRgbFactor[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendOperation blendRgbOp[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendFactor    blendSrcAlphaFactor[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendFactor    blendDstAlphaFactor[RHIO_MAX_COLOR_ATTACHMENTS];
    riBlendOperation blendAlphaOp[RHIO_MAX_COLOR_ATTACHMENTS];
    riTextureFormat  renderTargetFormats[RHIO_MAX_COLOR_ATTACHMENTS];
    riU32            renderTargetCount;

    riShaderModule vertexShader;
    riShaderModule fragmentShader;
    riShaderModule meshShader;
    riShaderModule taskShader;

    const char * name; // Optional debug name

} riRenderPipelineInfo;

// Attachment parameters consumed when beginning a render pass
typedef struct riRenderPassAttachmentInfo
{
    riTextureView           textureView; // Render target/depth view
    riAttachmentLoadAction  loadAction;  // Attachment load behavior
    riAttachmentStoreAction storeAction; // Attachment store behavior

    riF32 clearColor[4];                 // RGBA, used for color attachments
    riF32 clearDepth;                    // Used for depth attachments
    riU8  clearStencil;                  // Used for stencil attachments

} riRenderPassAttachmentInfo;

// Render pass begin parameters
typedef struct riRenderPassInfo
{
    riRenderPassAttachmentInfo colorAttachments[RHIO_MAX_COLOR_ATTACHMENTS];
    riU32                      colorAttachmentCount;

    riRenderPassAttachmentInfo depthStencilAttachment;
    bool                       hasDepthStencilAttachment;

    riU32 renderAreaX;
    riU32 renderAreaY;
    riU32 renderWidth;
    riU32 renderHeight;

} riRenderPassInfo;

// Command queue submission parameters
typedef struct riCommandQueueSubmitInfo
{
    const riCommandList * commandLists;     // Command lists submitted in array order
    riU32                 commandListCount; // Number of command lists in commandLists

} riCommandQueueSubmitInfo;

// Texture dispatch table
typedef struct riTextureVTable
{
    void ( *destroy_texture )( riTexture texture );
    void ( *get_texture_info )( riTexture texture, riTextureInfo * outInfo );

} riTextureVTable;

// Buffer dispatch table
typedef struct riBufferVTable
{
    void ( *destroy_buffer )( riBuffer buffer );
    void ( *get_buffer_info )( riBuffer buffer, riBufferInfo * outInfo );

} riBufferVTable;

// Texture view dispatch table
typedef struct riTextureViewVTable
{
    void ( *destroy_texture_view )( riTextureView textureView );
    void ( *get_texture_view_info )( riTextureView textureView, riTextureViewInfo * outInfo );

} riTextureViewVTable;

// Render pipeline dispatch table
typedef struct riRenderPipelineVTable
{
    void ( *destroy_render_pipeline )( riRenderPipeline pipeline );
    void ( *get_render_pipeline_info )( riRenderPipeline pipeline, riRenderPipelineInfo * outInfo );

} riRenderPipelineVTable;

// Swapchain dispatch table
typedef struct riSwapchainVTable
{
    void     ( *destroy_swapchain )( riSwapchain swapchain );
    riStatus ( *get_current_texture )( riSwapchain swapchain, riTexture * outTexture );
    riStatus ( *present )( riSwapchain swapchain );

} riSwapchainVTable;

// Command list dispatch table
typedef struct riCommandListVTable
{
    void     ( *destroy_command_list )( riCommandList commandList );
    riStatus ( *begin )( riCommandList commandList );
    riStatus ( *end )( riCommandList commandList );
    riStatus ( *reset )( riCommandList commandList );
    riStatus ( *begin_render_pass )( riCommandList commandList, const riRenderPassInfo * info,
                                     riRenderPass * outRenderPass );

} riCommandListVTable;

// Render pass dispatch table
typedef struct riRenderPassVTable
{
    riStatus ( *end )( riRenderPass renderPass );
    riStatus ( *set_pipeline )( riRenderPass renderPass, riRenderPipeline pipeline );
    riStatus ( *set_shader_buffer )( riRenderPass renderPass, riU32 slot, riBuffer buffer, riU32 offset );
    riStatus ( *set_viewport )( riRenderPass renderPass, riU32 x, riU32 y, riU32 width, riU32 height );
    riStatus ( *draw )( riRenderPass renderPass, riU32 vertexCount, riU32 instanceCount, riU32 firstVertex,
                        riU32 firstInstance );

} riRenderPassVTable;

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

    // Display / Render Target Management
    //------------------------------------------------------------------------------
    riStatus ( *create_swapchain )( riDevice device, riCommandQueue queue, const riSwapchainInfo * info,
                                    riSwapchain * outSwapchain );
    riStatus ( *create_texture_view )( riDevice device, const riTextureViewInfo * info,
                                       riTextureView * outTextureView );
    riStatus ( *create_buffer )( riDevice device, const riBufferInfo * info, riBuffer * outBuffer );
    riStatus ( *create_render_pipeline )( riDevice device, const riRenderPipelineInfo * info,
                                          riRenderPipeline * outPipeline );

} riDeviceVTable;

// Command queue dispatch table
typedef struct riCommandQueueVTable
{
    riStatus ( *create_command_list )( riCommandQueue queue, riCommandList * outCommandList );
    void     ( *destroy_command_queue )( riCommandQueue queue );
    riStatus ( *submit_command_list )( riCommandQueue queue, riCommandList commandList );

    riStatus ( *submit_command_lists )( riCommandQueue queue, const riCommandQueueSubmitInfo * info );

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
DECLARE_RI_BASE( riCommandList );

DECLARE_RI_BASE( riBuffer );
DECLARE_RI_BASE( riRenderPipeline );

DECLARE_RI_BASE( riSwapchain );

DECLARE_RI_BASE( riTexture );
DECLARE_RI_BASE( riTextureView );

DECLARE_RI_BASE( riRenderPass );

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
RI_API riStatus rhioCommandQueueSubmitInfo( riCommandQueue queue, const riCommandQueueSubmitInfo * info );

// Command lists
RI_API riStatus rhioCreateCommandList( riCommandQueue queue, riCommandList * outCommandList );
RI_API void     rhioDestroyCommandList( riCommandList commandList );
RI_API riStatus rhioCommandListBegin( riCommandList commandList );
RI_API riStatus rhioCommandListEnd( riCommandList commandList );
RI_API riStatus rhioCommandListReset( riCommandList commandList );
RI_API riStatus rhioCommandListBeginRenderPass( riCommandList commandList, const riRenderPassInfo * info,
                                                riRenderPass * outRenderPass );

// Buffers
RI_API riStatus rhioCreateBuffer( riDevice device, const riBufferInfo * info, riBuffer * outBuffer );
RI_API void     rhioDestroyBuffer( riBuffer buffer );
RI_API void     rhioGetBufferInfo( riBuffer buffer, riBufferInfo * outInfo );

// Render pipelines
RI_API riStatus rhioCreateRenderPipeline( riDevice device, const riRenderPipelineInfo * info,
                                          riRenderPipeline * outPipeline );
RI_API void     rhioDestroyRenderPipeline( riRenderPipeline pipeline );
RI_API void     rhioGetRenderPipelineInfo( riRenderPipeline pipeline, riRenderPipelineInfo * outInfo );

// Swapchains
RI_API riStatus rhioCreateSwapchain( riDevice device, riCommandQueue queue, const riSwapchainInfo * info,
                                     riSwapchain * outSwapchain );
RI_API void     rhioDestroySwapchain( riSwapchain swapchain );
RI_API riStatus rhioSwapchainGetCurrentTexture( riSwapchain swapchain, riTexture * outTexture );
RI_API riStatus rhioSwapchainPresent( riSwapchain swapchain );

// Texture views
RI_API riStatus rhioCreateTextureView( riDevice device, const riTextureViewInfo * info,
                                       riTextureView * outTextureView );
RI_API void     rhioDestroyTextureView( riTextureView textureView );

// Render passes
RI_API riStatus rhioRenderPassEnd( riRenderPass renderPass );
RI_API riStatus rhioRenderPassSetPipeline( riRenderPass renderPass, riRenderPipeline pipeline );
RI_API riStatus rhioRenderPassSetShaderBuffer( riRenderPass renderPass, riU32 slot, riBuffer buffer, riU32 offset );
RI_API riStatus rhioRenderPassSetViewport( riRenderPass renderPass, riU32 x, riU32 y, riU32 width, riU32 height );
RI_API riStatus rhioRenderPassDraw( riRenderPass renderPass, riU32 vertexCount, riU32 instanceCount, riU32 firstVertex,
                                    riU32 firstInstance );

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

//----------------------------------------------------------------------------------
// Internal Macros
//----------------------------------------------------------------------------------

// Calls func(in_val, out_ptr), stores result in `riStatus status`, returns on failure
#    define TRY_CONVERT( func, in_val, out_ptr )                                                                       \
        do                                                                                                             \
            {                                                                                                          \
                status = func( ( in_val ), ( out_ptr ) );                                                              \
                if( RI_FAILED( status ) ) return status;                                                               \
            }                                                                                                          \
        while( 0 )

//----------------------------------------------------------------------------------
// Internal Structs
//----------------------------------------------------------------------------------

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
static riStatus _rhioValidateCommandListVTable( const riCommandListVTable * vtable );
static riStatus _rhioValidateSwapchainVTable( const riSwapchainVTable * vtable );
static riStatus _rhioValidateBufferVTable( const riBufferVTable * vtable );
static riStatus _rhioValidateTextureViewVTable( const riTextureViewVTable * vtable );
static riStatus _rhioValidateRenderPipelineVTable( const riRenderPipelineVTable * vtable );
static riStatus _rhioValidateRenderPassVTable( const riRenderPassVTable * vtable );

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
    RI_GUARD( outDevice != NULL, RI_ERROR_INVALID_PARAM );
    *outDevice = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

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

    RI_GUARD_VOID( device != NULL );

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
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );
    *outQueue = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD( deviceBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( deviceBase->vtable->create_command_queue != NULL, RI_ERROR_INVALID_STATE );

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

    RI_GUARD( *outQueue != NULL, RI_ERROR_INVALID_STATE );

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
    riCommandListBase *  listBase  = NULL;
    riStatus             status    = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD( outCommandList != NULL, RI_ERROR_INVALID_PARAM );
    *outCommandList = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    queueBase = (riCommandQueueBase *)queue;

    // Queue Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD( queueBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( queueBase->vtable->create_command_list != NULL, RI_ERROR_INVALID_STATE );

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

    RI_GUARD( *outCommandList != NULL, RI_ERROR_INVALID_STATE );

    // Command List Dispatch Validation
    //----------------------------------------------------------
    listBase = (riCommandListBase *)*outCommandList;
    status   = _rhioValidateCommandListVTable( listBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( listBase->vtable != NULL && listBase->vtable->destroy_command_list != NULL )
                {
                    listBase->vtable->destroy_command_list( *outCommandList );
                }

            RI_FREE( *outCommandList );
            *outCommandList = NULL;
            return status;
        }

    TRACELOG( RI_LOG_INFO, "COMMAND_LIST: Created successfully" );

    return RI_SUCCESS;
}

// Destroy a command list wrapper and release recorded commands/resources
RI_API void
rhioDestroyCommandList( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    if( NULL == commandList ) return;

    listBase = (riCommandListBase *)commandList;

    if( NULL != listBase->vtable && NULL != listBase->vtable->destroy_command_list )
        {
            listBase->vtable->destroy_command_list( commandList );
        }

    RI_FREE( commandList );

    TRACELOG( RI_LOG_INFO, "COMMAND_LIST: Destroyed successfully" );
}

// Begin recording commands into a command list
RI_API riStatus
rhioCommandListBegin( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD( listBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( listBase->vtable->begin != NULL, RI_ERROR_INVALID_STATE );

    return listBase->vtable->begin( commandList );
}

// Finish recording commands into a command list
RI_API riStatus
rhioCommandListEnd( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD( listBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( listBase->vtable->end != NULL, RI_ERROR_INVALID_STATE );

    return listBase->vtable->end( commandList );
}

// Reset a command list so it can record a new frame
RI_API riStatus
rhioCommandListReset( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD( listBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( listBase->vtable->reset != NULL, RI_ERROR_INVALID_STATE );

    return listBase->vtable->reset( commandList );
}

// Begin a render pass and record its load operations into the command list
RI_API riStatus
rhioCommandListBeginRenderPass( riCommandList commandList, const riRenderPassInfo * info, riRenderPass * outRenderPass )
{
    riCommandListBase * listBase = NULL;
    riRenderPassBase *  passBase = NULL;
    riStatus            status   = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD( outRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    *outRenderPass = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD( listBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( listBase->vtable->begin_render_pass != NULL, RI_ERROR_INVALID_STATE );

    // Backend Render Pass Begin
    //----------------------------------------------------------
    status = listBase->vtable->begin_render_pass( commandList, info, outRenderPass );
    if( RI_FAILED( status ) )
        {
            *outRenderPass = NULL;
            return status;
        }

    RI_GUARD( *outRenderPass != NULL, RI_ERROR_INVALID_STATE );

    // Render Pass Dispatch Validation
    //----------------------------------------------------------
    passBase = (riRenderPassBase *)*outRenderPass;
    status   = _rhioValidateRenderPassVTable( passBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( passBase->vtable != NULL && passBase->vtable->end != NULL )
                {
                    passBase->vtable->end( *outRenderPass );
                }

            RI_FREE( *outRenderPass );
            *outRenderPass = NULL;
            return status;
        }

    return RI_SUCCESS;
}

// Submit one command list through a command queue
RI_API riStatus
rhioCommandQueueSubmit( riCommandQueue queue, riCommandList commandList )
{
    riCommandQueueSubmitInfo submitInfo = RI_ZERO_INIT;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    submitInfo.commandLists     = &commandList;
    submitInfo.commandListCount = 1u;

    return rhioCommandQueueSubmitInfo( queue, &submitInfo );
}

// Submit one or more command lists through a command queue
RI_API riStatus
rhioCommandQueueSubmitInfo( riCommandQueue queue, const riCommandQueueSubmitInfo * info )
{
    riCommandQueueBase * queueBase = NULL;
    riU32                i         = 0u;
    riStatus             status    = RI_ERROR_UNKNOWN;

    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandLists != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandListCount > 0u, RI_ERROR_INVALID_PARAM );

    queueBase = (riCommandQueueBase *)queue;

    RI_GUARD( queueBase->vtable != NULL, RI_ERROR_INVALID_STATE );

    // Submit Batch Validation
    //----------------------------------------------------------
    // Validate every handle before dispatch so the fallback path cannot submit a
    // partial batch before discovering a bad command list
    for( i = 0u; i < info->commandListCount; ++i )
        {
            RI_GUARD( info->commandLists[i] != NULL, RI_ERROR_INVALID_PARAM );
        }

    if( queueBase->vtable->submit_command_lists != NULL )
        {
            return queueBase->vtable->submit_command_lists( queue, info );
        }

    RI_GUARD( queueBase->vtable->submit_command_list != NULL, RI_ERROR_INVALID_STATE );

    for( i = 0u; i < info->commandListCount; ++i )
        {
            status = queueBase->vtable->submit_command_list( queue, info->commandLists[i] );
            if( RI_FAILED( status ) ) return status;
        }

    return RI_SUCCESS;
}

#    pragma endregion // Command Queues

/* ---------------------------------------------------------------------------------
 *
 * BUFFERS / PIPELINES                                               [>>RESOURCES<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Resources"

// Create a backend buffer
RI_API riStatus
rhioCreateBuffer( riDevice device, const riBufferInfo * info, riBuffer * outBuffer )
{
    riDeviceBase * deviceBase = NULL;
    riBufferBase * bufferBase = NULL;
    riStatus       status     = RI_ERROR_UNKNOWN;

    RI_GUARD( outBuffer != NULL, RI_ERROR_INVALID_PARAM );
    *outBuffer = NULL;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    RI_GUARD( deviceBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( deviceBase->vtable->create_buffer != NULL, RI_ERROR_BACKEND_UNAVAIL );

    status = deviceBase->vtable->create_buffer( device, info, outBuffer );
    if( RI_FAILED( status ) )
        {
            *outBuffer = NULL;
            return status;
        }

    RI_GUARD( *outBuffer != NULL, RI_ERROR_INVALID_STATE );

    bufferBase = (riBufferBase *)*outBuffer;
    status     = _rhioValidateBufferVTable( bufferBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( bufferBase->vtable != NULL && bufferBase->vtable->destroy_buffer != NULL )
                {
                    bufferBase->vtable->destroy_buffer( *outBuffer );
                }

            RI_FREE( *outBuffer );
            *outBuffer = NULL;
            return status;
        }

    return RI_SUCCESS;
}

// Destroy a backend buffer
RI_API void
rhioDestroyBuffer( riBuffer buffer )
{
    riBufferBase * bufferBase = NULL;

    if( buffer == NULL ) return;

    bufferBase = (riBufferBase *)buffer;

    if( bufferBase->vtable != NULL && bufferBase->vtable->destroy_buffer != NULL )
        {
            bufferBase->vtable->destroy_buffer( buffer );
        }

    RI_FREE( buffer );
}

// Return buffer metadata
RI_API void
rhioGetBufferInfo( riBuffer buffer, riBufferInfo * outInfo )
{
    riBufferBase * bufferBase = NULL;

    if( buffer == NULL || outInfo == NULL ) return;

    bufferBase = (riBufferBase *)buffer;

    if( bufferBase->vtable != NULL && bufferBase->vtable->get_buffer_info != NULL )
        {
            bufferBase->vtable->get_buffer_info( buffer, outInfo );
        }
}

// Create a backend render pipeline
RI_API riStatus
rhioCreateRenderPipeline( riDevice device, const riRenderPipelineInfo * info, riRenderPipeline * outPipeline )
{
    riDeviceBase *         deviceBase   = NULL;
    riRenderPipelineBase * pipelineBase = NULL;
    riStatus               status       = RI_ERROR_UNKNOWN;

    RI_GUARD( outPipeline != NULL, RI_ERROR_INVALID_PARAM );
    *outPipeline = NULL;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    RI_GUARD( deviceBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( deviceBase->vtable->create_render_pipeline != NULL, RI_ERROR_BACKEND_UNAVAIL );

    status = deviceBase->vtable->create_render_pipeline( device, info, outPipeline );
    if( RI_FAILED( status ) )
        {
            *outPipeline = NULL;
            return status;
        }

    RI_GUARD( *outPipeline != NULL, RI_ERROR_INVALID_STATE );

    pipelineBase = (riRenderPipelineBase *)*outPipeline;
    status       = _rhioValidateRenderPipelineVTable( pipelineBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( pipelineBase->vtable != NULL && pipelineBase->vtable->destroy_render_pipeline != NULL )
                {
                    pipelineBase->vtable->destroy_render_pipeline( *outPipeline );
                }

            RI_FREE( *outPipeline );
            *outPipeline = NULL;
            return status;
        }

    return RI_SUCCESS;
}

// Destroy a render pipeline
RI_API void
rhioDestroyRenderPipeline( riRenderPipeline pipeline )
{
    riRenderPipelineBase * pipelineBase = NULL;

    if( pipeline == NULL ) return;

    pipelineBase = (riRenderPipelineBase *)pipeline;

    if( pipelineBase->vtable != NULL && pipelineBase->vtable->destroy_render_pipeline != NULL )
        {
            pipelineBase->vtable->destroy_render_pipeline( pipeline );
        }

    RI_FREE( pipeline );
}

// Return render pipeline metadata
RI_API void
rhioGetRenderPipelineInfo( riRenderPipeline pipeline, riRenderPipelineInfo * outInfo )
{
    riRenderPipelineBase * pipelineBase = NULL;

    if( pipeline == NULL || outInfo == NULL ) return;

    pipelineBase = (riRenderPipelineBase *)pipeline;

    if( pipelineBase->vtable != NULL && pipelineBase->vtable->get_render_pipeline_info != NULL )
        {
            pipelineBase->vtable->get_render_pipeline_info( pipeline, outInfo );
        }
}

#    pragma endregion // Resources

/* ---------------------------------------------------------------------------------
 *
 * SWAPCHAINS / TEXTURE VIEWS / RENDER PASSES                         [>>FRAME<<]
 *
 * ---------------------------------------------------------------------------------*/

#    pragma region "Frame Control"

// Create a backend display swapchain
RI_API riStatus
rhioCreateSwapchain( riDevice device, riCommandQueue queue, const riSwapchainInfo * info, riSwapchain * outSwapchain )
{
    riDeviceBase *    deviceBase    = NULL;
    riSwapchainBase * swapchainBase = NULL;
    riStatus          status        = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD( outSwapchain != NULL, RI_ERROR_INVALID_PARAM );
    *outSwapchain = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD( deviceBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( deviceBase->vtable->create_swapchain != NULL, RI_ERROR_INVALID_STATE );

    // Backend Swapchain Creation
    //----------------------------------------------------------
    status = deviceBase->vtable->create_swapchain( device, queue, info, outSwapchain );
    if( RI_FAILED( status ) )
        {
            *outSwapchain = NULL;
            return status;
        }

    RI_GUARD( *outSwapchain != NULL, RI_ERROR_INVALID_STATE );

    // Swapchain Dispatch Validation
    //----------------------------------------------------------
    swapchainBase = (riSwapchainBase *)*outSwapchain;
    status        = _rhioValidateSwapchainVTable( swapchainBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( swapchainBase->vtable != NULL && swapchainBase->vtable->destroy_swapchain != NULL )
                {
                    swapchainBase->vtable->destroy_swapchain( *outSwapchain );
                }

            RI_FREE( *outSwapchain );
            *outSwapchain = NULL;
            return status;
        }

    TRACELOG( RI_LOG_INFO, "SWAPCHAIN: Created successfully" );

    return RI_SUCCESS;
}

// Destroy a swapchain wrapper
RI_API void
rhioDestroySwapchain( riSwapchain swapchain )
{
    riSwapchainBase * swapchainBase = NULL;

    if( NULL == swapchain ) return;

    swapchainBase = (riSwapchainBase *)swapchain;

    if( NULL != swapchainBase->vtable && NULL != swapchainBase->vtable->destroy_swapchain )
        {
            swapchainBase->vtable->destroy_swapchain( swapchain );
        }

    RI_FREE( swapchain );

    TRACELOG( RI_LOG_INFO, "SWAPCHAIN: Destroyed successfully" );
}

// Acquire the current display texture; returned texture is borrowed from the swapchain
RI_API riStatus
rhioSwapchainGetCurrentTexture( riSwapchain swapchain, riTexture * outTexture )
{
    riSwapchainBase * swapchainBase = NULL;

    RI_GUARD( outTexture != NULL, RI_ERROR_INVALID_PARAM );
    *outTexture = NULL;

    RI_GUARD( swapchain != NULL, RI_ERROR_INVALID_PARAM );

    swapchainBase = (riSwapchainBase *)swapchain;

    RI_GUARD( swapchainBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( swapchainBase->vtable->get_current_texture != NULL, RI_ERROR_INVALID_STATE );

    return swapchainBase->vtable->get_current_texture( swapchain, outTexture );
}

// Present the current swapchain image
RI_API riStatus
rhioSwapchainPresent( riSwapchain swapchain )
{
    riSwapchainBase * swapchainBase = NULL;

    RI_GUARD( swapchain != NULL, RI_ERROR_INVALID_PARAM );

    swapchainBase = (riSwapchainBase *)swapchain;

    RI_GUARD( swapchainBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( swapchainBase->vtable->present != NULL, RI_ERROR_INVALID_STATE );

    return swapchainBase->vtable->present( swapchain );
}

// Create a texture view through the device backend
RI_API riStatus
rhioCreateTextureView( riDevice device, const riTextureViewInfo * info, riTextureView * outTextureView )
{
    riDeviceBase *      deviceBase      = NULL;
    riTextureViewBase * textureViewBase = NULL;
    riStatus            status          = RI_ERROR_UNKNOWN;

    // Output Handle Initialization
    //----------------------------------------------------------
    RI_GUARD( outTextureView != NULL, RI_ERROR_INVALID_PARAM );
    *outTextureView = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD( deviceBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( deviceBase->vtable->create_texture_view != NULL, RI_ERROR_INVALID_STATE );

    // Backend Texture View Creation
    //----------------------------------------------------------
    status = deviceBase->vtable->create_texture_view( device, info, outTextureView );
    if( RI_FAILED( status ) )
        {
            *outTextureView = NULL;
            return status;
        }

    RI_GUARD( *outTextureView != NULL, RI_ERROR_INVALID_STATE );

    // Texture View Dispatch Validation
    //----------------------------------------------------------
    textureViewBase = (riTextureViewBase *)*outTextureView;
    status          = _rhioValidateTextureViewVTable( textureViewBase->vtable );
    if( RI_FAILED( status ) )
        {
            if( textureViewBase->vtable != NULL && textureViewBase->vtable->destroy_texture_view != NULL )
                {
                    textureViewBase->vtable->destroy_texture_view( *outTextureView );
                }

            RI_FREE( *outTextureView );
            *outTextureView = NULL;
            return status;
        }

    return RI_SUCCESS;
}

// Destroy a texture view wrapper
RI_API void
rhioDestroyTextureView( riTextureView textureView )
{
    riTextureViewBase * textureViewBase = NULL;

    if( NULL == textureView ) return;

    textureViewBase = (riTextureViewBase *)textureView;

    if( NULL != textureViewBase->vtable && NULL != textureViewBase->vtable->destroy_texture_view )
        {
            textureViewBase->vtable->destroy_texture_view( textureView );
        }

    RI_FREE( textureView );
}

// End an active render pass
RI_API riStatus
rhioRenderPassEnd( riRenderPass renderPass )
{
    riRenderPassBase * passBase = NULL;

    RI_GUARD( renderPass != NULL, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD( passBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( passBase->vtable->end != NULL, RI_ERROR_INVALID_STATE );

    return passBase->vtable->end( renderPass );
}

// Record render pipeline bind into an active render pass
RI_API riStatus
rhioRenderPassSetPipeline( riRenderPass renderPass, riRenderPipeline pipeline )
{
    riRenderPassBase * passBase = NULL;

    RI_GUARD( renderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( pipeline != NULL, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD( passBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( passBase->vtable->set_pipeline != NULL, RI_ERROR_BACKEND_UNAVAIL );

    return passBase->vtable->set_pipeline( renderPass, pipeline );
}

// Record shader-buffer bind into an active render pass
RI_API riStatus
rhioRenderPassSetShaderBuffer( riRenderPass renderPass, riU32 slot, riBuffer buffer, riU32 offset )
{
    riRenderPassBase * passBase = NULL;

    RI_GUARD( renderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( buffer != NULL, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD( passBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( passBase->vtable->set_shader_buffer != NULL, RI_ERROR_BACKEND_UNAVAIL );

    return passBase->vtable->set_shader_buffer( renderPass, slot, buffer, offset );
}

// Record viewport state into an active render pass
RI_API riStatus
rhioRenderPassSetViewport( riRenderPass renderPass, riU32 x, riU32 y, riU32 width, riU32 height )
{
    riRenderPassBase * passBase = NULL;

    RI_GUARD( renderPass != NULL, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD( passBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( passBase->vtable->set_viewport != NULL, RI_ERROR_BACKEND_UNAVAIL );

    return passBase->vtable->set_viewport( renderPass, x, y, width, height );
}

// Record a non-indexed draw into an active render pass
RI_API riStatus
rhioRenderPassDraw( riRenderPass renderPass, riU32 vertexCount, riU32 instanceCount, riU32 firstVertex,
                    riU32 firstInstance )
{
    riRenderPassBase * passBase = NULL;

    RI_GUARD( renderPass != NULL, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD( passBase->vtable != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( passBase->vtable->draw != NULL, RI_ERROR_BACKEND_UNAVAIL );

    return passBase->vtable->draw( renderPass, vertexCount, instanceCount, firstVertex, firstInstance );
}

#    pragma endregion // Frame Control

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
// OpenGL Backend Configuration
//----------------------------------------------------------------------------------

// Default number of fixed-size OpenGL commands per memory block
#        ifndef RHIO_GL_COMMANDS_PER_BLOCK
#            define RHIO_GL_COMMANDS_PER_BLOCK 256u
#        endif

// Default number of command blocks preallocated per command list
#        ifndef RHIO_GL_COMMAND_BLOCK_POOL_SIZE
#            define RHIO_GL_COMMAND_BLOCK_POOL_SIZE 8u
#        endif

// Built-in triangle shader pulls three vertices from shader buffer binding 0
#        ifndef RHIO_GL_TRIANGLE_VERTEX_COUNT
#            define RHIO_GL_TRIANGLE_VERTEX_COUNT 3u
#        endif

#        ifndef RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING
#            define RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING 0u
#        endif

//----------------------------------------------------------------------------------
// Platform GL Headers and Entry Points
//----------------------------------------------------------------------------------
// NOTE: rhio does not own an OpenGL context. The application/windowing owns it
// This block provides only the minimal types and headers

#        if defined( RHIO_BACKEND_OPENGL )                                                                             \
            && !defined( RHIO_BACKEND_OPENGLES )                                                                       \
            && !defined( RHIO_BACKEND_OPENGLES2 )                                                                      \
            && !defined( RHIO_BACKEND_OPENGLES3 )
#            define RHIO_GL_BACKEND_DESKTOP
#        endif

#        if defined( RHIO_GL_BACKEND_DESKTOP ) && !defined( __EMSCRIPTEN__ ) && !defined( __APPLE__ )
#            define RHIO_GL_USE_GLEW
#        endif

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

// Win32 / WGL
//------------------------------------------------------------------------------
#        elif defined( _WIN32 )

// NOTE: Win32 uses WGL context handles. GLEW owns desktop GL entry-point loading.

#            if !defined( WIN32_LEAN_AND_MEAN )
#                define WIN32_LEAN_AND_MEAN
#            endif
#            if !defined( NOMINMAX )
#                define NOMINMAX
#            endif

#            include <GL/glew.h>
#            include <windows.h>

typedef HDC   riGL_DeviceContext;
typedef HGLRC riGL_RenderContext;

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

// Unix-like (Linux / *BSD) / GLX + EGL
//------------------------------------------------------------------------------
#        elif defined( __linux__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ )

// NOTE: Unix-like GLES builds expose symbols directly through EGL. Desktop GL uses
// GLEW for entry-point loading.

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

#            else                          /* RHIO_BACKEND_OPENGLES */

#                include <GL/glew.h>
#                include <X11/Xlib.h>

typedef Display * riGL_DeviceContext;
typedef void *    riGL_RenderContext;

#            endif /* RHIO_BACKEND_OPENGLES || RHIO_BACKEND_OPENGLES2 || RHIO_BACKEND_OPENGLES3 */

// Unsupported
//------------------------------------------------------------------------------
#        else
#            error "RHIO GL backend: unsupported platform"
#        endif     /* GL platform selection */

//----------------------------------------------------------------------------------
// Compatibility Defines
//----------------------------------------------------------------------------------

#        if defined( RHIO_BACKEND_OPENGLES ) || defined( RHIO_BACKEND_OPENGLES2 ) || defined( RHIO_BACKEND_OPENGLES3 )
#            define glClearDepth( depth ) glClearDepthf( (GLfloat)( depth ) )
#        else
#            define glClearDepth( depth ) glClearDepth( (GLdouble)( depth ) )
#        endif

#        if defined( RHIO_GL_BACKEND_DESKTOP ) || defined( RHIO_BACKEND_OPENGLES3 )
#            define RHIO_GL_HAS_VERTEX_ARRAY_OBJECT
#        endif

#        if defined( RHIO_GL_BACKEND_DESKTOP ) || defined( RHIO_BACKEND_OPENGLES3 )
#            define RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING
#        endif

//----------------------------------------------------------------------------------
// Types and Structures Definition                                    [>>GL_TYPES<<]
//----------------------------------------------------------------------------------

// Internal OpenGL device state
typedef struct riGL_Device
{
    riDeviceBase base;                  // Frontend handle dispatch
#        if defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    GLint uniformBufferOffsetAlignment; // Alignment required for glBindBufferRange offsets
#        endif
#        if defined( RHIO_GL_HAS_VERTEX_ARRAY_OBJECT )
    GLuint defaultVertexArray;          // Shared VAO required by desktop core profile
#        endif

} riGL_Device;

// Internal OpenGL buffer state
typedef struct riGL_Buffer
{
    riBufferBase base;   // Frontend handle dispatch
    riBufferInfo info;   // Normalized metadata; data pointer is not retained
    GLuint       buffer; // GL buffer object name

} riGL_Buffer;

// Internal OpenGL render pipeline state
typedef struct riGL_RenderPipeline
{
    riRenderPipelineBase base;      // Frontend handle dispatch
    riRenderPipelineInfo info;      // Normalized metadata
    GLuint               program;   // Linked passthrough shader program
    GLenum               primitive; // GL primitive topology

} riGL_RenderPipeline;

// Internal OpenGL texture state
typedef struct riGL_Texture
{
    riTextureBase base;                 // Frontend handle dispatch
    riTextureInfo info;                 // Normalized metadata
    bool          isDefaultFramebuffer; // Texture represents the backbuffer

} riGL_Texture;

// Internal OpenGL texture view state
typedef struct riGL_TextureView
{
    riTextureViewBase base;                 // Frontend handle dispatch
    riTextureViewInfo info;                 // Normalized view metadata
    riGL_Texture *    texture;              // Borrowed texture backing this view
    bool              isDefaultFramebuffer; // View targets framebuffer 0

} riGL_TextureView;

// Internal OpenGL swapchain state lifecycle
typedef enum riGL_SwapchainState
{
    RI_GL_SWAPCHAIN_STATE_IDLE = 0, // Swapchain is ready for use
    RI_GL_SWAPCHAIN_STATE_ACQUIRED, // Backbuffer has been acquired
    RI_GL_SWAPCHAIN_STATE_LOST,     // Swapchain needs recreation

} riGL_SwapchainState;

// Internal OpenGL swapchain implementation
typedef struct riGL_Swapchain
{
    riSwapchainBase     base;           // Frontend handle dispatch
    riSwapchainInfo     info;           // Creation info copy
    riGL_Texture        currentTexture; // Borrowed default-framebuffer facade
    riGL_SwapchainState state;          // Acquire / present lifetime state

} riGL_Swapchain;

// Internal OpenGL command list recording and submission state
typedef enum riGL_CommandListState
{
    RI_GL_COMMAND_LIST_STATE_INITIAL = 0, // Command list is ready for recording
    RI_GL_COMMAND_LIST_STATE_RECORDING,   // Command list is currently recording commands
    RI_GL_COMMAND_LIST_STATE_EXECUTABLE,  // Command list has finished recording and is ready for submission
    RI_GL_COMMAND_LIST_STATE_PENDING,     // Command list has been submitted and is awaiting execution
    RI_GL_COMMAND_LIST_STATE_SUBMITTED,   // Command list has been successfully executed

} riGL_CommandListState;

// Internal OpenGL command identifiers
typedef enum riGL_CommandType
{
    RI_GL_COMMAND_BIND_DEFAULT_FRAMEBUFFER = 0,
    RI_GL_COMMAND_SET_VIEWPORT,
    RI_GL_COMMAND_SET_SCISSOR,
    RI_GL_COMMAND_SET_SCISSOR_ENABLED,
    RI_GL_COMMAND_SET_CLEAR_COLOR,
    RI_GL_COMMAND_SET_CLEAR_DEPTH_STENCIL,
    RI_GL_COMMAND_CLEAR,
    RI_GL_COMMAND_BIND_PIPELINE,
    RI_GL_COMMAND_BIND_SHADER_BUFFER,
    RI_GL_COMMAND_DRAW,

} riGL_CommandType;

typedef struct riGL_CmdSetRect
{
    riU32 x;
    riU32 y;
    riU32 width;
    riU32 height;

} riGL_CmdSetRect;

typedef struct riGL_CmdSetClearColor
{
    riF32 r;
    riF32 g;
    riF32 b;
    riF32 a;

} riGL_CmdSetClearColor;

typedef struct riGL_CmdSetClearDepthStencil
{
    riF32 depth;
    riU8  stencil;
    riU8  clearDepth;
    riU8  clearStencil;
    riU8  _pad;

} riGL_CmdSetClearDepthStencil;

typedef struct riGL_CmdSetEnabled
{
    riU32 enabled;

} riGL_CmdSetEnabled;

typedef struct riGL_CmdClear
{
    riU32 mask;

} riGL_CmdClear;

typedef struct riGL_CmdBindPipeline
{
    riRenderPipeline pipeline;

} riGL_CmdBindPipeline;

typedef struct riGL_CmdBindShaderBuffer
{
    riBuffer buffer;
    riU32    offset;
    riU32    slot;

} riGL_CmdBindShaderBuffer;

typedef struct riGL_CmdDraw
{
    riU32 vertexCount;
    riU32 instanceCount;
    riU32 firstVertex;
    riU32 firstInstance;

} riGL_CmdDraw;

typedef union riGL_CommandData
{
    riGL_CmdSetRect              setRect;
    riGL_CmdSetClearColor        setClearColor;
    riGL_CmdSetClearDepthStencil setClearDepthStencil;
    riGL_CmdSetEnabled           setEnabled;
    riGL_CmdClear                clear;
    riGL_CmdBindPipeline         bindPipeline;
    riGL_CmdBindShaderBuffer     bindShaderBuffer;
    riGL_CmdDraw                 draw;

} riGL_CommandData;

// Fixed-size OpenGL command record. Keep this small to preserve command-stream locality.
typedef struct riGL_Command
{
    riU32            type; // riGL_CommandType identifier
    riGL_CommandData data;

} riGL_Command;

// 24 = sizeof(riU32 type) + 4 pad + sizeof(riGL_CommandData union) where largest members are 16 bytes
RHIO_STATIC_ASSERT( sizeof( riGL_Command ) <= 24u, "OpenGL emitted command records must stay <= 24 bytes" );

// A fixed-size block of command records
typedef struct riGL_CommandBlock
{
    struct riGL_CommandBlock * next;        // Next active or recycled block
    riU32                      used;        // Commands consumed in data
    riU32                      capacity;    // Usable command count in data
    riGL_Command               commands[1]; // Flexible command storage

} riGL_CommandBlock;

// Memory manager for OpenGL command recording
typedef struct riGL_CommandAllocator
{
    riGL_CommandBlock * firstBlock;   // First active block in execution order
    riGL_CommandBlock * lastBlock;    // Last active block for O(1) appends
    riGL_CommandBlock * currentBlock; // Current write target
    riGL_CommandBlock * freeBlocks;   // Recycled blocks kept for the next reset
    riU32               commandsPerBlock;
#        if defined( RHIO_DEBUG )
    riU32 activeBlockCount;           // Number of blocks currently in use
    riU32 freeBlockCount;             // Number of blocks in the free list
    riU32 commandCount;               // Total number of commands recorded
    riU32 peakCommandCount;           // Maximum command count recorded
#        endif

} riGL_CommandAllocator;

typedef struct riGL_CommandList riGL_CommandList;

// Command queue implementation
typedef struct riGL_CommandQueue
{
    riCommandQueueBase base;         // Frontend handle dispatch
    riGL_Device *      device;       // Owning device state
    riGL_CommandList * commandLists; // Queue-owned command lists

} riGL_CommandQueue;

// Command list implementation
struct riGL_CommandList
{
    riCommandListBase     base;             // Frontend handle dispatch
    riGL_CommandList *    next;             // Next queue-owned command list
    riCommandQueue        queue;            // Owning queue handle
    riGL_CommandListState state;            // Recording/submission lifecycle
    riGL_CommandAllocator commandAllocator; // Linear command memory
    riRenderPass          activeRenderPass; // Currently open pass, if any
};

// Render pass state
typedef struct riGL_RenderPass
{
    riRenderPassBase   base;        // Frontend handle dispatch
    riGL_CommandList * commandList; // Owning command list while pass is open
    bool               ended;       // Prevents double-end on the opaque handle

} riGL_RenderPass;

// Device
static riStatus _rhioGL_init( riDevice device, const riBackendInitInfo * info );
static void     _rhioGL_shutdown( riDevice device );
static riStatus _rhioGL_create_swapchain( riDevice device, riCommandQueue queue, const riSwapchainInfo * info,
                                          riSwapchain * outSwapchain );
static riStatus _rhioGL_create_texture_view( riDevice device, const riTextureViewInfo * info,
                                             riTextureView * outTextureView );
static riStatus _rhioGL_create_buffer( riDevice device, const riBufferInfo * info, riBuffer * outBuffer );
static riStatus _rhioGL_create_render_pipeline( riDevice device, const riRenderPipelineInfo * info,
                                                riRenderPipeline * outPipeline );

// Command queue
static riStatus _rhioGL_create_command_queue( riDevice device, riCommandQueue * outQueue );
static riStatus _rhioGL_create_command_list( riCommandQueue queue, riCommandList * outCommandList );
static void     _rhioGL_destroy_command_queue( riCommandQueue queue );
static riStatus _rhioGL_submit_command_list( riCommandQueue queue, riCommandList commandList );
static riStatus _rhioGL_submit_command_lists( riCommandQueue queue, const riCommandQueueSubmitInfo * info );

// Command list
static void     _rhioGL_destroy_command_list( riCommandList commandList );
static riStatus _rhioGL_command_list_begin( riCommandList commandList );
static riStatus _rhioGL_command_list_end( riCommandList commandList );
static riStatus _rhioGL_command_list_reset( riCommandList commandList );
static riStatus _rhioGL_begin_render_pass( riCommandList commandList, const riRenderPassInfo * info,
                                           riRenderPass * outRenderPass );

// Swapchain
static void     _rhioGL_destroy_swapchain( riSwapchain swapchain );
static riStatus _rhioGL_get_current_texture( riSwapchain swapchain, riTexture * outTexture );
static riStatus _rhioGL_present( riSwapchain swapchain );

// Texture / texture views
static void _rhioGL_destroy_buffer( riBuffer buffer );
static void _rhioGL_get_buffer_info( riBuffer buffer, riBufferInfo * outInfo );
static void _rhioGL_destroy_texture_noop( riTexture texture );
static void _rhioGL_get_texture_info( riTexture texture, riTextureInfo * outInfo );
static void _rhioGL_destroy_texture_view( riTextureView textureView );
static void _rhioGL_get_texture_view_info( riTextureView textureView, riTextureViewInfo * outInfo );
static void _rhioGL_destroy_render_pipeline( riRenderPipeline pipeline );
static void _rhioGL_get_render_pipeline_info( riRenderPipeline pipeline, riRenderPipelineInfo * outInfo );

// Render pass
static riStatus _rhioGL_render_pass_end( riRenderPass renderPass );
static riStatus _rhioGL_render_pass_set_pipeline( riRenderPass renderPass, riRenderPipeline pipeline );
static riStatus _rhioGL_render_pass_set_shader_buffer( riRenderPass renderPass, riU32 slot, riBuffer buffer,
                                                       riU32 offset );
static riStatus _rhioGL_render_pass_set_viewport( riRenderPass renderPass, riU32 x, riU32 y, riU32 width,
                                                  riU32 height );
static riStatus _rhioGL_render_pass_draw( riRenderPass renderPass, riU32 vertexCount, riU32 instanceCount,
                                          riU32 firstVertex, riU32 firstInstance );

// Command memory
static riStatus _rhioGL_command_allocator_init( riGL_CommandAllocator * allocator );
static void     _rhioGL_command_allocator_destroy( riGL_CommandAllocator * allocator );
static void     _rhioGL_command_allocator_reset( riGL_CommandAllocator * allocator );
static riStatus _rhioGL_command_allocator_write( riGL_CommandAllocator * allocator, riGL_CommandType type,
                                                 riGL_Command ** outCommand );
static void     _rhioGL_command_list_clear_commands( riGL_CommandList * commandList );
static riStatus _rhioGL_append_bind_default_framebuffer_command( riGL_CommandList * commandList );
static riStatus _rhioGL_append_set_viewport_command( riGL_CommandList * commandList, riU32 x, riU32 y, riU32 width,
                                                     riU32 height );
static riStatus _rhioGL_append_set_scissor_command( riGL_CommandList * commandList, riU32 x, riU32 y, riU32 width,
                                                    riU32 height );
static riStatus _rhioGL_append_set_scissor_enabled_command( riGL_CommandList * commandList, bool enabled );
static riStatus _rhioGL_append_set_clear_color_command( riGL_CommandList * commandList, const riF32 color[4] );
static riStatus _rhioGL_append_set_clear_depth_stencil_command( riGL_CommandList * commandList, riF32 depth,
                                                                riU8 stencil, bool clearDepth, bool clearStencil );
static riStatus _rhioGL_append_clear_command( riGL_CommandList * commandList, riU32 mask );
static riStatus _rhioGL_append_bind_pipeline_command( riGL_CommandList * commandList, riRenderPipeline pipeline );
static riStatus _rhioGL_append_bind_shader_buffer_command( riGL_CommandList * commandList, riU32 slot, riBuffer buffer,
                                                           riU32 offset );
static riStatus _rhioGL_append_draw_command( riGL_CommandList * commandList, riU32 vertexCount, riU32 instanceCount,
                                             riU32 firstVertex, riU32 firstInstance );

// Command execution
static riStatus _rhioGL_execute_command_list( riGL_CommandList * commandList );
static riStatus _rhioGL_execute_draw_command( const riGL_Command * command, riGL_RenderPipeline * pipeline,
                                              riGL_Buffer * shaderBuffers[RHIO_MAX_SHADER_BUFFER_BINDINGS],
                                              const riU32   shaderBufferOffsets[RHIO_MAX_SHADER_BUFFER_BINDINGS] );
static void     _rhioGL_bind_default_framebuffer( void );
static riStatus _rhioGL_load_entrypoints( void );
static riStatus _rhioGL_compile_shader( GLenum shaderType, const char * source, GLuint * outShader );
static riStatus _rhioGL_create_builtin_program( GLuint * outProgram );
static riStatus _rhioGL_validate_render_pipeline_info( const riRenderPipelineInfo * info );
static riStatus _rhioGL_apply_pipeline_state( const riGL_RenderPipeline * pipeline );
static riStatus _rhioGL_topology_to_gl( riPipelineTopology topology, GLenum * outTopology );
static riStatus _rhioGL_compare_operation_to_gl( riCompareOperation operation, GLenum * outOperation );
static riStatus _rhioGL_blend_factor_to_gl( riBlendFactor factor, GLenum * outFactor );
static riStatus _rhioGL_blend_operation_to_gl( riBlendOperation operation, GLenum * outOperation );

//----------------------------------------------------------------------------------
// OpenGL Backend Vtable
//----------------------------------------------------------------------------------
#        define BIND_FUNC( name ) .name = _rhioGL_##name

static const riDeviceVTable s_gl_vtable = {
    BIND_FUNC( init ),
    BIND_FUNC( shutdown ),
    BIND_FUNC( create_command_queue ),
    BIND_FUNC( create_swapchain ),
    BIND_FUNC( create_texture_view ),
    BIND_FUNC( create_buffer ),
    BIND_FUNC( create_render_pipeline ),
};

static const riCommandQueueVTable s_gl_command_queue_vtable = {
    BIND_FUNC( create_command_list ),
    BIND_FUNC( destroy_command_queue ),
    BIND_FUNC( submit_command_list ),
    BIND_FUNC( submit_command_lists ),
};

static const riCommandListVTable s_gl_command_list_vtable = {
    BIND_FUNC( destroy_command_list ),
    .begin             = _rhioGL_command_list_begin,
    .end               = _rhioGL_command_list_end,
    .reset             = _rhioGL_command_list_reset,
    .begin_render_pass = _rhioGL_begin_render_pass,
};

static const riSwapchainVTable s_gl_swapchain_vtable = {
    BIND_FUNC( destroy_swapchain ),
    BIND_FUNC( get_current_texture ),
    BIND_FUNC( present ),
};

static const riTextureVTable s_gl_texture_vtable = {
    .destroy_texture  = _rhioGL_destroy_texture_noop,
    .get_texture_info = _rhioGL_get_texture_info,
};

static const riBufferVTable s_gl_buffer_vtable = {
    BIND_FUNC( destroy_buffer ),
    BIND_FUNC( get_buffer_info ),
};

static const riTextureViewVTable s_gl_texture_view_vtable = {
    BIND_FUNC( destroy_texture_view ),
    BIND_FUNC( get_texture_view_info ),
};

static const riRenderPipelineVTable s_gl_render_pipeline_vtable = {
    BIND_FUNC( destroy_render_pipeline ),
    BIND_FUNC( get_render_pipeline_info ),
};

static const riRenderPassVTable s_gl_render_pass_vtable = {
    .end               = _rhioGL_render_pass_end,
    .set_pipeline      = _rhioGL_render_pass_set_pipeline,
    .set_shader_buffer = _rhioGL_render_pass_set_shader_buffer,
    .set_viewport      = _rhioGL_render_pass_set_viewport,
    .draw              = _rhioGL_render_pass_draw,
};

#        undef BIND_FUNC

//----------------------------------------------------------------------------------
// OpenGL Backend Implementation                                       [>>GL_IMPL<<]
//----------------------------------------------------------------------------------

// Initialize OpenGL state and allocate default resources
static riStatus
_rhioGL_init( riDevice device, const riBackendInitInfo * info )
{
    riGL_Device *   glDevice    = (riGL_Device *)device;
    const GLubyte * vendor      = NULL;
    const GLubyte * renderer    = NULL;
    const GLubyte * version     = NULL;
    const GLubyte * glslVersion = NULL;
    riStatus        status      = RI_ERROR_UNKNOWN;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );

    UNUSED( info );

    // Context Validation
    //------------------------------------------------------------------------------
    // rhio records and submits into the context current on this thread. A NULL
    // GL_VERSION is the portable signal that no usable context is current
    version = glGetString( GL_VERSION );
    if( RI_UNLIKELY( NULL == version ) )
        {
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: No current OpenGL context" );
            return RI_ERROR_BACKEND_INIT;
        }

    // Entry Point Loading
    //------------------------------------------------------------------------------
    status = _rhioGL_load_entrypoints();
    if( RI_FAILED( status ) ) return status;

    // Context Diagnostics
    //------------------------------------------------------------------------------
    vendor      = glGetString( GL_VENDOR );
    renderer    = glGetString( GL_RENDERER );
    glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

#        if defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &glDevice->uniformBufferOffsetAlignment );
    if( glDevice->uniformBufferOffsetAlignment <= 0 )
        {
            glDevice->uniformBufferOffsetAlignment = 1;
        }
#        endif

#        if defined( RHIO_GL_HAS_VERTEX_ARRAY_OBJECT )
    // Default Vertex Array
    //------------------------------------------------------------------------------
    // Desktop core OpenGL requires a VAO to issue draws. ES2 builds skip this
    // path because VAOs are not core there.
    glGenVertexArrays( 1, &glDevice->defaultVertexArray );
    if( RI_UNLIKELY( glDevice->defaultVertexArray == 0u ) )
        {
            return RI_ERROR_BACKEND_INIT;
        }

    glBindVertexArray( glDevice->defaultVertexArray );
#        endif

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

    // Device State Reset
    //----------------------------------------------------------
    if( NULL != glDevice )
        {
#        if defined( RHIO_GL_HAS_VERTEX_ARRAY_OBJECT )
            if( glDevice->defaultVertexArray != 0u )
                {
                    glDeleteVertexArrays( 1, &glDevice->defaultVertexArray );
                }
#        endif

            emptyDevice.base = glDevice->base; // Preserve frontend dispatch state
            *glDevice        = emptyDevice;
        }

    TRACELOG( RI_LOG_INFO, "BACKEND GL: Shutdown complete" );
}

// Create an OpenGL display swapchain wrapper over the current default framebuffer
static riStatus
_rhioGL_create_swapchain( riDevice device, riCommandQueue queue, const riSwapchainInfo * info,
                          riSwapchain * outSwapchain )
{
    riGL_Swapchain * glSwapchain       = NULL;
    riFlags          validPresentFlags = RI_SWAPCHAIN_PRESENT_FLAG_FORCE_FLUSH;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outSwapchain != NULL, RI_ERROR_INVALID_PARAM );

    RI_GUARD( info->width > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->height > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( RI_FLAG_CHECK( info->presentFlags, ~validPresentFlags ) == 0u, RI_ERROR_INVALID_PARAM );

    // Swapchain Allocation
    //----------------------------------------------------------
    glSwapchain = (riGL_Swapchain *)RI_CALLOC( 1, sizeof( *glSwapchain ) );
    if( RI_UNLIKELY( NULL == glSwapchain ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    // Swapchain State Initialization
    //----------------------------------------------------------
    glSwapchain->base.vtable = &s_gl_swapchain_vtable;
    glSwapchain->info        = *info;
    glSwapchain->state       = RI_GL_SWAPCHAIN_STATE_IDLE;

    if( glSwapchain->info.format == RHIO_TEXTURE_FORMAT_UNDEFINED )
        {
            glSwapchain->info.format = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        }

    // Current Display Texture
    //----------------------------------------------------------
    // OpenGL exposes the drawable framebuffer through framebuffer 0. The texture
    // handle is a borrowed RHI facade so render-pass code can stay target-based
    glSwapchain->currentTexture.base.vtable          = &s_gl_texture_vtable;
    glSwapchain->currentTexture.isDefaultFramebuffer = true;
    glSwapchain->currentTexture.info.width           = glSwapchain->info.width;
    glSwapchain->currentTexture.info.height          = glSwapchain->info.height;
    glSwapchain->currentTexture.info.depth           = 1u;
    glSwapchain->currentTexture.info.mipLevels       = 1u;
    glSwapchain->currentTexture.info.arrayLayers     = 1u;
    glSwapchain->currentTexture.info.format          = glSwapchain->info.format;
    glSwapchain->currentTexture.info.usage           = RI_TEXTURE_USAGE_RENDER_TARGET;
    glSwapchain->currentTexture.info.dimensions      = RI_TEXTURE_DIMENSIONS_2D;
    glSwapchain->currentTexture.info.name            = "GL default framebuffer";

    *outSwapchain                                    = (riSwapchain)glSwapchain;

    return RI_SUCCESS;
}

// Create a GL texture view over a backend texture
static riStatus
_rhioGL_create_texture_view( riDevice device, const riTextureViewInfo * info, riTextureView * outTextureView )
{
    riGL_Texture *     glTexture     = NULL;
    riGL_TextureView * glTextureView = NULL;
    riTextureViewInfo  normalized    = RI_ZERO_INIT;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outTextureView != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->texture != NULL, RI_ERROR_INVALID_PARAM );

    // View Normalization
    //----------------------------------------------------------
    // Public zero/UNDEFINED fields inherit from the source texture so callers can
    // request a whole-texture render-target view without restating metadata
    glTexture  = (riGL_Texture *)info->texture;
    normalized = *info;

    if( normalized.format == RHIO_TEXTURE_FORMAT_UNDEFINED )
        {
            normalized.format = glTexture->info.format;
        }

    if( normalized.usage == 0u )
        {
            normalized.usage = glTexture->info.usage;
        }

    if( normalized.dimensions == RI_TEXTURE_DIMENSIONS_UNDEFINED )
        {
            normalized.dimensions = glTexture->info.dimensions;
        }

    // Subresource Range Validation
    //----------------------------------------------------------
    RI_GUARD( normalized.baseMipLevel < glTexture->info.mipLevels, RI_ERROR_INVALID_PARAM );
    RI_GUARD( normalized.baseArrayLayer < glTexture->info.arrayLayers, RI_ERROR_INVALID_PARAM );

    if( normalized.mipLevelCount == 0u || normalized.mipLevelCount == RHIO_TEXTURE_VIEW_ALL_MIPS )
        {
            normalized.mipLevelCount = glTexture->info.mipLevels - normalized.baseMipLevel;
        }

    if( normalized.arrayLayerCount == 0u || normalized.arrayLayerCount == RHIO_TEXTURE_VIEW_ALL_ARRAY_LAYERS )
        {
            normalized.arrayLayerCount = glTexture->info.arrayLayers - normalized.baseArrayLayer;
        }

    RI_GUARD( normalized.mipLevelCount > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( normalized.arrayLayerCount > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( normalized.mipLevelCount <= glTexture->info.mipLevels - normalized.baseMipLevel, RI_ERROR_INVALID_PARAM );
    RI_GUARD( normalized.arrayLayerCount <= glTexture->info.arrayLayers - normalized.baseArrayLayer,
              RI_ERROR_INVALID_PARAM );

    // Usage Validation
    //----------------------------------------------------------
    // A view may narrow usage, but it cannot claim capabilities that the backing
    // texture did not expose at creation time
    RI_GUARD( RI_FLAG_CHECK( normalized.usage,
                             ~( RI_TEXTURE_USAGE_SAMPLED
                                | RI_TEXTURE_USAGE_STORAGE
                                | RI_TEXTURE_USAGE_RENDER_TARGET
                                | RI_TEXTURE_USAGE_DEPTH_STENCIL ) )
                  == 0u,
              RI_ERROR_INVALID_PARAM );
    RI_GUARD( RI_FLAG_CHECK( normalized.usage, ~glTexture->info.usage ) == 0u, RI_ERROR_INVALID_PARAM );

    // Texture View Allocation
    //----------------------------------------------------------
    glTextureView = (riGL_TextureView *)RI_CALLOC( 1, sizeof( *glTextureView ) );
    if( RI_UNLIKELY( NULL == glTextureView ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    // Texture View State Initialization
    //----------------------------------------------------------
    glTextureView->base.vtable          = &s_gl_texture_view_vtable;
    glTextureView->info                 = normalized;
    glTextureView->texture              = glTexture;
    glTextureView->isDefaultFramebuffer = glTexture->isDefaultFramebuffer;

    // Texture View Handle Handoff
    //----------------------------------------------------------
    *outTextureView = (riTextureView)glTextureView;

    UNUSED( device );

    return RI_SUCCESS;
}

// Create a GL buffer and optionally upload initial contents
static riStatus
_rhioGL_create_buffer( riDevice device, const riBufferInfo * info, riBuffer * outBuffer )
{
    riGL_Buffer * glBuffer   = NULL;
    riBufferInfo  normalized = RI_ZERO_INIT;
    const riFlags validUsage = RI_BUFFER_USAGE_SHADER_READ;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outBuffer != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->size > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( RI_FLAG_CHECK( info->usage, ~validUsage ) == 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( RI_FLAG_CHECK( info->usage, RI_BUFFER_USAGE_SHADER_READ ) != 0u, RI_ERROR_INVALID_PARAM );

#        if !defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    UNUSED( device );
    UNUSED( outBuffer );
    return RI_ERROR_BACKEND_UNAVAIL;
#        endif

    glBuffer = (riGL_Buffer *)RI_CALLOC( 1, sizeof( *glBuffer ) );
    if( RI_UNLIKELY( glBuffer == NULL ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    normalized            = *info;
    normalized.data       = NULL;

    glBuffer->base.vtable = &s_gl_buffer_vtable;
    glBuffer->info        = normalized;

    glGenBuffers( 1, &glBuffer->buffer );
    if( RI_UNLIKELY( glBuffer->buffer == 0u ) )
        {
            RI_FREE( glBuffer );
            return RI_ERROR_BACKEND_INIT;
        }

    glBindBuffer( GL_UNIFORM_BUFFER, glBuffer->buffer );
    glBufferData( GL_UNIFORM_BUFFER, (GLsizeiptr)info->size, info->data, GL_STATIC_DRAW );
    glBindBuffer( GL_UNIFORM_BUFFER, 0u );

    *outBuffer = (riBuffer)glBuffer;

    return RI_SUCCESS;
}

// Create a render pipeline using the backend's hardcoded passthrough shader
static riStatus
_rhioGL_create_render_pipeline( riDevice device, const riRenderPipelineInfo * info, riRenderPipeline * outPipeline )
{
    riGL_RenderPipeline * glPipeline = NULL;
    riRenderPipelineInfo  normalized = RI_ZERO_INIT;
    riStatus              status     = RI_ERROR_UNKNOWN;
    GLuint                program    = 0u;
    GLenum                primitive  = GL_TRIANGLES;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outPipeline != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_validate_render_pipeline_info( info );
    if( RI_FAILED( status ) ) return status;

#        if !defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    UNUSED( device );
    UNUSED( outPipeline );
    return RI_ERROR_BACKEND_UNAVAIL;
#        endif

    TRY_CONVERT( _rhioGL_topology_to_gl, info->topology, &primitive );

    status = _rhioGL_create_builtin_program( &program );
    if( RI_FAILED( status ) ) return status;

    glPipeline = (riGL_RenderPipeline *)RI_CALLOC( 1, sizeof( *glPipeline ) );
    if( RI_UNLIKELY( glPipeline == NULL ) )
        {
            glDeleteProgram( program );
            return RI_ERROR_OUT_OF_MEMORY;
        }

    normalized = *info;
    if( normalized.renderTargetCount == 0u )
        {
            normalized.renderTargetCount      = 1u;
            normalized.renderTargetFormats[0] = RHIO_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        }

    glPipeline->base.vtable = &s_gl_render_pipeline_vtable;
    glPipeline->info        = normalized;
    glPipeline->program     = program;
    glPipeline->primitive   = primitive;

    *outPipeline            = (riRenderPipeline)glPipeline;

    return RI_SUCCESS;
}

// Initialize an OpenGL command queue wrapper
static riStatus
_rhioGL_create_command_queue( riDevice device, riCommandQueue * outQueue )
{
    riGL_CommandQueue * glQueue = NULL;

    RI_GUARD( device != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outQueue != NULL, RI_ERROR_INVALID_PARAM );

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
    glQueue->device      = (riGL_Device *)device;

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

    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outCommandList != NULL, RI_ERROR_INVALID_PARAM );

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
    glCommandList->base.vtable = &s_gl_command_list_vtable;
    glCommandList->queue       = queue;

    // Queue Ownership Link
    //----------------------------------------------------------
    // The queue tracks live lists so queue destruction can clean up command memory
    // even if the frontend destroys the queue before individual lists
    glCommandList->next   = glQueue->commandLists;
    glQueue->commandLists = glCommandList;

    {
        riStatus status = _rhioGL_command_allocator_init( &glCommandList->commandAllocator );
        if( RI_FAILED( status ) )
            {
                glQueue->commandLists = glCommandList->next;
                RI_FREE( glCommandList );
                return status;
            }
    }

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

    RI_GUARD_VOID( glQueue != NULL );

    // Command List Cleanup
    //----------------------------------------------------------
    while( NULL != glQueue->commandLists )
        {
            commandList           = glQueue->commandLists;
            glQueue->commandLists = commandList->next;
            if( commandList->activeRenderPass != NULL )
                {
                    (void)_rhioGL_render_pass_end( commandList->activeRenderPass );
                }
            _rhioGL_command_allocator_destroy( &commandList->commandAllocator );
            RI_FREE( commandList );
        }
}

// Submit one command list to the OpenGL backend
static riStatus
_rhioGL_submit_command_list( riCommandQueue queue, riCommandList commandList )
{
    riCommandQueueSubmitInfo submitInfo = RI_ZERO_INIT;

    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    submitInfo.commandLists     = &commandList;
    submitInfo.commandListCount = 1u;

    return _rhioGL_submit_command_lists( queue, &submitInfo );
}

// Submit command lists to the OpenGL owner context in caller-provided order
static riStatus
_rhioGL_submit_command_lists( riCommandQueue queue, const riCommandQueueSubmitInfo * info )
{
    riGL_CommandList * glCommandList = NULL;
    riU32              i             = 0u;
    riStatus           status        = RI_ERROR_UNKNOWN;

    RI_GUARD( queue != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandLists != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandListCount > 0u, RI_ERROR_INVALID_PARAM );

    // Submit Validation
    //----------------------------------------------------------
    // Validate the full batch before executing any command list. This keeps the
    // fallback and batched paths from partially submitting malformed batches
    for( i = 0u; i < info->commandListCount; ++i )
        {
            RI_GUARD( info->commandLists[i] != NULL, RI_ERROR_INVALID_PARAM );

            glCommandList = (riGL_CommandList *)info->commandLists[i];
            RI_GUARD( glCommandList->queue == queue, RI_ERROR_INVALID_PARAM );
            RI_GUARD( glCommandList->state == RI_GL_COMMAND_LIST_STATE_EXECUTABLE, RI_ERROR_INVALID_STATE );
        }

    // Command Stream Execution
    //----------------------------------------------------------
    // GL commands execute immediately on the caller's current context. rhio keeps
    // the state transition explicit so later fence/sync support has a clean slot
    for( i = 0u; i < info->commandListCount; ++i )
        {
            glCommandList        = (riGL_CommandList *)info->commandLists[i];
            glCommandList->state = RI_GL_COMMAND_LIST_STATE_PENDING;

            status               = _rhioGL_execute_command_list( glCommandList );
            if( RI_FAILED( status ) )
                {
                    _rhioGL_command_list_clear_commands( glCommandList );
                    glCommandList->state = RI_GL_COMMAND_LIST_STATE_SUBMITTED;
                    return status;
                }

            _rhioGL_command_list_clear_commands( glCommandList );
            glCommandList->state = RI_GL_COMMAND_LIST_STATE_SUBMITTED;
        }

    return RI_SUCCESS;
}

// Destroy one command list and detach it from its owning queue
static void
_rhioGL_destroy_command_list( riCommandList commandList )
{
    riGL_CommandList *  glCommandList = (riGL_CommandList *)commandList;
    riGL_CommandQueue * glQueue       = NULL;
    riGL_CommandList ** cursor        = NULL;

    RI_GUARD_VOID( glCommandList != NULL );

    // Active Render Pass Cleanup
    //----------------------------------------------------------
    if( glCommandList->activeRenderPass != NULL )
        {
            (void)_rhioGL_render_pass_end( glCommandList->activeRenderPass );
        }

    // Command Memory Cleanup
    //----------------------------------------------------------
    _rhioGL_command_allocator_destroy( &glCommandList->commandAllocator );

    // Queue Ownership Unlink
    //----------------------------------------------------------
    // The frontend frees the wrapper after this callback returns, so the backend
    // only detaches queue-owned links and releases backend-private allocations
    glQueue = (riGL_CommandQueue *)glCommandList->queue;
    if( glQueue != NULL )
        {
            cursor = &glQueue->commandLists;
            while( *cursor != NULL )
                {
                    if( *cursor == glCommandList )
                        {
                            *cursor = glCommandList->next;
                            break;
                        }

                    cursor = &( *cursor )->next;
                }
        }
}

// Begin command recording
static riStatus
_rhioGL_command_list_begin( riCommandList commandList )
{
    riGL_CommandList * glCommandList = (riGL_CommandList *)commandList;

    RI_GUARD( glCommandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( glCommandList->state == RI_GL_COMMAND_LIST_STATE_INITIAL, RI_ERROR_INVALID_STATE );

    glCommandList->state = RI_GL_COMMAND_LIST_STATE_RECORDING;

    return RI_SUCCESS;
}

// Finish command recording
static riStatus
_rhioGL_command_list_end( riCommandList commandList )
{
    riGL_CommandList * glCommandList = (riGL_CommandList *)commandList;

    RI_GUARD( glCommandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( glCommandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( glCommandList->activeRenderPass == NULL, RI_ERROR_INVALID_STATE );

    glCommandList->state = RI_GL_COMMAND_LIST_STATE_EXECUTABLE;

    return RI_SUCCESS;
}

// Reset command recording state and discard recorded commands
static riStatus
_rhioGL_command_list_reset( riCommandList commandList )
{
    riGL_CommandList * glCommandList = (riGL_CommandList *)commandList;

    RI_GUARD( glCommandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( glCommandList->activeRenderPass == NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glCommandList->state != RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( glCommandList->state != RI_GL_COMMAND_LIST_STATE_PENDING, RI_ERROR_INVALID_STATE );

    _rhioGL_command_list_clear_commands( glCommandList );
    glCommandList->state = RI_GL_COMMAND_LIST_STATE_INITIAL;

    return RI_SUCCESS;
}

// Begin a render pass and record attachment load operations
static riStatus
_rhioGL_begin_render_pass( riCommandList commandList, const riRenderPassInfo * info, riRenderPass * outRenderPass )
{
    riGL_CommandList *                 glCommandList   = (riGL_CommandList *)commandList;
    riGL_RenderPass *                  glRenderPass    = NULL;
    const riRenderPassAttachmentInfo * attachment      = NULL;
    riGL_TextureView *                 glTextureView   = NULL;
    riF32                              clearColor[4]   = { 0.0f, 0.0f, 0.0f, 0.0f };
    riF32                              clearDepth      = 1.0f;
    riU8                               clearStencil    = 0u;
    riU32                              clearMask       = 0u;
    riU32                              targetWidth     = 0u;
    riU32                              targetHeight    = 0u;
    bool                               hasClearColor   = false;
    bool                               hasClearDepth   = false;
    bool                               hasClearStencil = false;
    bool                               clearScissored  = false;
    riU32                              i               = 0;
    riStatus                           status          = RI_ERROR_UNKNOWN;

    RI_GUARD( glCommandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outRenderPass != NULL, RI_ERROR_INVALID_PARAM );

    RI_GUARD( glCommandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( glCommandList->activeRenderPass == NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( info->colorAttachmentCount <= RHIO_MAX_COLOR_ATTACHMENTS, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderWidth > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderHeight > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->colorAttachmentCount > 0u || info->hasDepthStencilAttachment, RI_ERROR_INVALID_PARAM );

    // Attachment Capability Validation
    //----------------------------------------------------------
    // OpenGL display path currently targets framebuffer 0. Offscreen FBO texture
    // attachments can share this render-pass API once GL textures are introduced
    if( info->colorAttachmentCount > 1u )
        {
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: Multiple color attachments require GL texture/FBO support" );
            return RI_ERROR_BACKEND_UNAVAIL;
        }

    // Color Attachment Load Operations
    //----------------------------------------------------------
    // LOAD and DONT_CARE need no GL command for the default framebuffer path
    // CLEAR is encoded into the command list so submission owns all GL calls
    for( i = 0u; i < info->colorAttachmentCount; ++i )
        {
            attachment = &info->colorAttachments[i];
            RI_GUARD( attachment->textureView != NULL, RI_ERROR_INVALID_PARAM );

            glTextureView = (riGL_TextureView *)attachment->textureView;
            RI_GUARD( glTextureView->isDefaultFramebuffer, RI_ERROR_BACKEND_UNAVAIL );
            RI_GUARD( glTextureView->texture != NULL, RI_ERROR_INVALID_STATE );
            RI_GUARD( RI_FLAG_CHECK( glTextureView->info.usage, RI_TEXTURE_USAGE_RENDER_TARGET ) != 0u,
                      RI_ERROR_INVALID_PARAM );

            if( i == 0u )
                {
                    targetWidth  = glTextureView->texture->info.width;
                    targetHeight = glTextureView->texture->info.height;
                }

            if( attachment->loadAction == RI_ATTACHMENT_LOAD_ACTION_CLEAR )
                {
                    hasClearColor = true;
                    clearColor[0] = attachment->clearColor[0];
                    clearColor[1] = attachment->clearColor[1];
                    clearColor[2] = attachment->clearColor[2];
                    clearColor[3] = attachment->clearColor[3];
                    clearMask |= (riU32)GL_COLOR_BUFFER_BIT;
                }
        }

    // Depth / Stencil Attachment Load Operation
    //----------------------------------------------------------
    if( info->hasDepthStencilAttachment )
        {
            attachment = &info->depthStencilAttachment;
            RI_GUARD( attachment->textureView != NULL, RI_ERROR_INVALID_PARAM );

            glTextureView = (riGL_TextureView *)attachment->textureView;
            RI_GUARD( glTextureView->isDefaultFramebuffer, RI_ERROR_BACKEND_UNAVAIL );
            RI_GUARD( glTextureView->texture != NULL, RI_ERROR_INVALID_STATE );
            RI_GUARD( RI_FLAG_CHECK( glTextureView->info.usage, RI_TEXTURE_USAGE_DEPTH_STENCIL ) != 0u,
                      RI_ERROR_INVALID_PARAM );

            if( targetWidth == 0u || targetHeight == 0u )
                {
                    targetWidth  = glTextureView->texture->info.width;
                    targetHeight = glTextureView->texture->info.height;
                }

            if( attachment->loadAction == RI_ATTACHMENT_LOAD_ACTION_CLEAR )
                {
                    hasClearDepth   = true;
                    clearDepth      = attachment->clearDepth;
                    hasClearStencil = true;
                    clearStencil    = attachment->clearStencil;
                    clearMask |= (riU32)( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
                }
        }

    RI_GUARD( targetWidth > 0u, RI_ERROR_INVALID_STATE );
    RI_GUARD( targetHeight > 0u, RI_ERROR_INVALID_STATE );
    RI_GUARD( info->renderAreaX <= targetWidth, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderAreaY <= targetHeight, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderWidth <= targetWidth - info->renderAreaX, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderHeight <= targetHeight - info->renderAreaY, RI_ERROR_INVALID_PARAM );

    // Target Binding
    //----------------------------------------------------------
    status = _rhioGL_append_bind_default_framebuffer_command( glCommandList );
    if( RI_FAILED( status ) ) return status;

    // Clear Command Encoding
    //----------------------------------------------------------
    // Clear state is deliberately encoded as granular commands instead of a fat
    // render-pass command, matching the emulated command buffer contract.
    if( clearMask != 0u )
        {
            if( hasClearColor )
                {
                    status = _rhioGL_append_set_clear_color_command( glCommandList, clearColor );
                    if( RI_FAILED( status ) ) return status;
                }

            if( hasClearDepth || hasClearStencil )
                {
                    status = _rhioGL_append_set_clear_depth_stencil_command(
                        glCommandList, clearDepth, clearStencil, hasClearDepth, hasClearStencil );
                    if( RI_FAILED( status ) ) return status;
                }

            clearScissored = info->renderAreaX
                             != 0u
                             || info->renderAreaY
                             != 0u
                             || info->renderWidth
                             != targetWidth
                             || info->renderHeight
                             != targetHeight;

            if( clearScissored )
                {
                    status = _rhioGL_append_set_scissor_command(
                        glCommandList, info->renderAreaX, info->renderAreaY, info->renderWidth, info->renderHeight );
                    if( RI_FAILED( status ) ) return status;
                }

            status = _rhioGL_append_set_scissor_enabled_command( glCommandList, clearScissored );
            if( RI_FAILED( status ) ) return status;

            status = _rhioGL_append_clear_command( glCommandList, clearMask );
            if( RI_FAILED( status ) ) return status;

            if( clearScissored )
                {
                    status = _rhioGL_append_set_scissor_enabled_command( glCommandList, false );
                    if( RI_FAILED( status ) ) return status;
                }
        }

    // Render Pass Handle Allocation
    //----------------------------------------------------------
    glRenderPass = (riGL_RenderPass *)RI_CALLOC( 1, sizeof( *glRenderPass ) );
    if( RI_UNLIKELY( NULL == glRenderPass ) )
        {
            return RI_ERROR_OUT_OF_MEMORY;
        }

    // Render Pass State Initialization
    //----------------------------------------------------------
    glRenderPass->base.vtable       = &s_gl_render_pass_vtable;
    glRenderPass->commandList       = glCommandList;

    glCommandList->activeRenderPass = (riRenderPass)glRenderPass;
    *outRenderPass                  = (riRenderPass)glRenderPass;

    return RI_SUCCESS;
}

// Destroy GL swapchain-private state
static void
_rhioGL_destroy_swapchain( riSwapchain swapchain )
{
    riGL_Swapchain * glSwapchain = (riGL_Swapchain *)swapchain;

    if( NULL != glSwapchain )
        {
            glSwapchain->state = RI_GL_SWAPCHAIN_STATE_LOST;
        }
}

// Return the borrowed default-framebuffer texture for this frame
static riStatus
_rhioGL_get_current_texture( riSwapchain swapchain, riTexture * outTexture )
{
    riGL_Swapchain * glSwapchain = (riGL_Swapchain *)swapchain;

    RI_GUARD( glSwapchain != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outTexture != NULL, RI_ERROR_INVALID_PARAM );
    if( glSwapchain->state == RI_GL_SWAPCHAIN_STATE_LOST ) return RI_ERROR_SURFACE_LOST;
    RI_GUARD( glSwapchain->state == RI_GL_SWAPCHAIN_STATE_IDLE, RI_ERROR_INVALID_STATE );

    glSwapchain->state = RI_GL_SWAPCHAIN_STATE_ACQUIRED;
    *outTexture        = (riTexture)&glSwapchain->currentTexture;

    return RI_SUCCESS;
}

// Present the GL default framebuffer through the optional host-window callback
static riStatus
_rhioGL_present( riSwapchain swapchain )
{
    riGL_Swapchain * glSwapchain = (riGL_Swapchain *)swapchain;
    riStatus         status      = RI_SUCCESS;

    RI_GUARD( glSwapchain != NULL, RI_ERROR_INVALID_PARAM );
    if( glSwapchain->state == RI_GL_SWAPCHAIN_STATE_LOST ) return RI_ERROR_SURFACE_LOST;
    RI_GUARD( glSwapchain->state == RI_GL_SWAPCHAIN_STATE_ACQUIRED, RI_ERROR_INVALID_STATE );

    // Optional Explicit Flush
    //----------------------------------------------------------
    // Best practice is to let the native buffer swap drive synchronization. This
    // flag exists for hosts that explicitly require a flush before presentation
    if( RI_FLAG_CHECK( glSwapchain->info.presentFlags, RI_SWAPCHAIN_PRESENT_FLAG_FORCE_FLUSH ) != 0u )
        {
            glFlush();
        }

    // Native Presentation Callback
    //----------------------------------------------------------
    // OpenGL context/window ownership stays outside rhio, so the host performs
    // the actual swap-buffers operation through this callback when needed
    if( glSwapchain->info.presentCallback != NULL )
        {
            status = glSwapchain->info.presentCallback( swapchain, glSwapchain->info.presentUserData );
        }

    // Swapchain State Transition
    //----------------------------------------------------------
    if( RI_SUCCEEDED( status ) )
        {
            glSwapchain->state = RI_GL_SWAPCHAIN_STATE_IDLE;
        }
    else if( status == RI_ERROR_SURFACE_LOST )
        {
            glSwapchain->state = RI_GL_SWAPCHAIN_STATE_LOST;
        }

    return status;
}

// Swapchain textures are borrowed; destruction is owned by the swapchain
static void
_rhioGL_destroy_texture_noop( riTexture texture )
{
    UNUSED( texture );
}

// Destroy a GL buffer
static void
_rhioGL_destroy_buffer( riBuffer buffer )
{
    riGL_Buffer * glBuffer = (riGL_Buffer *)buffer;

    if( glBuffer != NULL && glBuffer->buffer != 0u )
        {
            glDeleteBuffers( 1, &glBuffer->buffer );
            glBuffer->buffer = 0u;
        }
}

// Return buffer metadata
static void
_rhioGL_get_buffer_info( riBuffer buffer, riBufferInfo * outInfo )
{
    riGL_Buffer * glBuffer = (riGL_Buffer *)buffer;

    if( glBuffer != NULL && outInfo != NULL )
        {
            *outInfo = glBuffer->info;
        }
}

// Return texture metadata
static void
_rhioGL_get_texture_info( riTexture texture, riTextureInfo * outInfo )
{
    riGL_Texture * glTexture = (riGL_Texture *)texture;

    if( glTexture != NULL && outInfo != NULL )
        {
            *outInfo = glTexture->info;
        }
}

// Destroy GL texture view-private state
static void
_rhioGL_destroy_texture_view( riTextureView textureView )
{
    riGL_TextureView * glTextureView = (riGL_TextureView *)textureView;

    if( glTextureView != NULL )
        {
            glTextureView->texture = NULL;
        }
}

// Return texture view metadata
static void
_rhioGL_get_texture_view_info( riTextureView textureView, riTextureViewInfo * outInfo )
{
    riGL_TextureView * glTextureView = (riGL_TextureView *)textureView;

    if( glTextureView != NULL && outInfo != NULL )
        {
            *outInfo = glTextureView->info;
        }
}

// Destroy a GL render pipeline
static void
_rhioGL_destroy_render_pipeline( riRenderPipeline pipeline )
{
    riGL_RenderPipeline * glPipeline = (riGL_RenderPipeline *)pipeline;

    if( glPipeline != NULL && glPipeline->program != 0u )
        {
            glDeleteProgram( glPipeline->program );
            glPipeline->program = 0u;
        }
}

// Return render pipeline metadata
static void
_rhioGL_get_render_pipeline_info( riRenderPipeline pipeline, riRenderPipelineInfo * outInfo )
{
    riGL_RenderPipeline * glPipeline = (riGL_RenderPipeline *)pipeline;

    if( glPipeline != NULL && outInfo != NULL )
        {
            *outInfo = glPipeline->info;
        }
}

// End a GL render pass
static riStatus
_rhioGL_render_pass_end( riRenderPass renderPass )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;

    RI_GUARD( glRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );

    glRenderPass->commandList->activeRenderPass = NULL;
    glRenderPass->ended                         = true;
    RI_FREE( glRenderPass );

    return RI_SUCCESS;
}

// Record a render pipeline bind
static riStatus
_rhioGL_render_pass_set_pipeline( riRenderPass renderPass, riRenderPipeline pipeline )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;

    RI_GUARD( glRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( pipeline != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );

    return _rhioGL_append_bind_pipeline_command( glRenderPass->commandList, pipeline );
}

// Record a shader-buffer bind
static riStatus
_rhioGL_render_pass_set_shader_buffer( riRenderPass renderPass, riU32 slot, riBuffer buffer, riU32 offset )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;
    riGL_Buffer *     glBuffer     = (riGL_Buffer *)buffer;
#        if defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    riGL_CommandQueue * glQueue  = NULL;
    riGL_Device *       glDevice = NULL;
#        endif

    RI_GUARD( glRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( glBuffer != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( slot < RHIO_MAX_SHADER_BUFFER_BINDINGS, RI_ERROR_INVALID_PARAM );
    RI_GUARD( RI_FLAG_CHECK( glBuffer->info.usage, RI_BUFFER_USAGE_SHADER_READ ) != 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( (riSize)offset <= glBuffer->info.size, RI_ERROR_INVALID_PARAM );

#        if defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    glQueue  = (riGL_CommandQueue *)glRenderPass->commandList->queue;
    glDevice = glQueue != NULL ? glQueue->device : NULL;
    RI_GUARD( glDevice != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( ( offset % (riU32)glDevice->uniformBufferOffsetAlignment ) == 0u, RI_ERROR_INVALID_PARAM );
#        endif

    return _rhioGL_append_bind_shader_buffer_command( glRenderPass->commandList, slot, buffer, offset );
}

// Record viewport state
static riStatus
_rhioGL_render_pass_set_viewport( riRenderPass renderPass, riU32 x, riU32 y, riU32 width, riU32 height )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;

    RI_GUARD( glRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( width > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( height > 0u, RI_ERROR_INVALID_PARAM );

    return _rhioGL_append_set_viewport_command( glRenderPass->commandList, x, y, width, height );
}

// Record a non-indexed draw
static riStatus
_rhioGL_render_pass_draw( riRenderPass renderPass, riU32 vertexCount, riU32 instanceCount, riU32 firstVertex,
                          riU32 firstInstance )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;

    RI_GUARD( glRenderPass != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->state == RI_GL_COMMAND_LIST_STATE_RECORDING, RI_ERROR_INVALID_STATE );
    RI_GUARD( vertexCount > 0u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( instanceCount == 1u, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( firstInstance == 0u, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( firstVertex < RHIO_GL_TRIANGLE_VERTEX_COUNT, RI_ERROR_INVALID_PARAM );
    RI_GUARD( vertexCount <= RHIO_GL_TRIANGLE_VERTEX_COUNT - firstVertex, RI_ERROR_INVALID_PARAM );

    return _rhioGL_append_draw_command(
        glRenderPass->commandList, vertexCount, instanceCount, firstVertex, firstInstance );
}

//----------------------------------------------------------------------------------
// OpenGL Command Memory
//----------------------------------------------------------------------------------

// Allocate one fixed-capacity command block
static riGL_CommandBlock *
_rhioGL_command_block_allocate( riU32 capacity )
{
    riGL_CommandBlock * block          = NULL;
    riSize              metadataSize   = 0u;
    riSize              allocationSize = 0u;
    riSize              maxAllocation  = (riSize)( (size_t)-1 );

    if( capacity == 0u ) return NULL;

    // Allocation Size Validation
    //----------------------------------------------------------
    metadataSize = (riSize)sizeof( riGL_CommandBlock ) - (riSize)sizeof( ( (riGL_CommandBlock *)0 )->commands );
    if( metadataSize > maxAllocation ) return NULL;
    if( (riSize)capacity > ( maxAllocation - metadataSize ) / (riSize)sizeof( riGL_Command ) ) return NULL;

    // Block Allocation
    //----------------------------------------------------------
    allocationSize = metadataSize + ( (riSize)capacity * (riSize)sizeof( riGL_Command ) );
    block          = (riGL_CommandBlock *)RI_CALLOC( 1, (size_t)allocationSize );
    if( block == NULL ) return NULL;

    block->capacity = capacity;

    return block;
}

// Destroy a linked list of command memory blocks
static void
_rhioGL_command_block_chain_destroy( riGL_CommandBlock * block )
{
    riGL_CommandBlock * next = NULL;

    while( block != NULL )
        {
            next = block->next;
            RI_FREE( block );
            block = next;
        }
}

// Move a recycled block from the pool into the active command stream
static riStatus
_rhioGL_command_allocator_obtain_block( riGL_CommandAllocator * allocator )
{
    riGL_CommandBlock * block = NULL;

    RI_GUARD( allocator != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( allocator->freeBlocks != NULL, RI_ERROR_OUT_OF_MEMORY );

    block                 = allocator->freeBlocks;
    allocator->freeBlocks = block->next;
#        if defined( RHIO_DEBUG )
    if( allocator->freeBlockCount > 0u )
        {
            --allocator->freeBlockCount;
        }
#        endif

    // Active List Append
    //----------------------------------------------------------
    // Append the block to the active list, preserving command execution order
    block->used = 0u;
    block->next = NULL;

    if( allocator->lastBlock != NULL )
        {
            allocator->lastBlock->next = block;
        }
    else
        {
            allocator->firstBlock = block;
        }

    allocator->lastBlock    = block;
    allocator->currentBlock = block;

#        if defined( RHIO_DEBUG )
    ++allocator->activeBlockCount;
#        endif

    return RI_SUCCESS;
}

// Initialize block-backed command memory for one GL command list
static riStatus
_rhioGL_command_allocator_init( riGL_CommandAllocator * allocator )
{
    riGL_CommandAllocator emptyAllocator = RI_ZERO_INIT;
    riGL_CommandBlock *   block          = NULL;
    riU32                 i              = 0u;

    RI_GUARD( allocator != NULL, RI_ERROR_INVALID_PARAM );

    *allocator                  = emptyAllocator;
    allocator->commandsPerBlock = (riU32)RHIO_GL_COMMANDS_PER_BLOCK;
    if( allocator->commandsPerBlock == 0u )
        {
            allocator->commandsPerBlock = 1u;
        }

    for( i = 0u; i < (riU32)RHIO_GL_COMMAND_BLOCK_POOL_SIZE; ++i )
        {
            block = _rhioGL_command_block_allocate( allocator->commandsPerBlock );
            if( NULL == block )
                {
                    _rhioGL_command_allocator_destroy( allocator );
                    return RI_ERROR_OUT_OF_MEMORY;
                }

            block->next           = allocator->freeBlocks;
            allocator->freeBlocks = block;
#        if defined( RHIO_DEBUG )
            ++allocator->freeBlockCount;
#        endif
        }

    return RI_SUCCESS;
}

// Release all command-memory blocks owned by an allocator
static void
_rhioGL_command_allocator_destroy( riGL_CommandAllocator * allocator )
{
    riGL_CommandAllocator emptyAllocator = RI_ZERO_INIT;

    if( allocator == NULL ) return;

    _rhioGL_command_block_chain_destroy( allocator->firstBlock );
    _rhioGL_command_block_chain_destroy( allocator->freeBlocks );

    *allocator = emptyAllocator;
}

// Recycle active command-memory blocks for reuse by later recordings
static void
_rhioGL_command_allocator_reset( riGL_CommandAllocator * allocator )
{
    riGL_CommandBlock * block     = NULL;
    riGL_CommandBlock * nextBlock = NULL;

    if( allocator == NULL ) return;

    // Active Block Recycle
    //----------------------------------------------------------
    // Reset moves blocks to a free list instead of releasing them; command lists
    // usually re-record similar amounts of data frame-to-frame
    block = allocator->firstBlock;
    while( block != NULL )
        {
            nextBlock             = block->next;
            block->used           = 0u;
            block->next           = allocator->freeBlocks;
            allocator->freeBlocks = block;
            block                 = nextBlock;
#        if defined( RHIO_DEBUG )
            ++allocator->freeBlockCount;
#        endif
        }

    allocator->firstBlock   = NULL;
    allocator->lastBlock    = NULL;
    allocator->currentBlock = NULL;

#        if defined( RHIO_DEBUG )
    allocator->activeBlockCount = 0u;
    allocator->commandCount     = 0u;
#        endif
}

// Reserve one fixed-size command record
static riStatus
_rhioGL_command_allocator_write( riGL_CommandAllocator * allocator, riGL_CommandType type, riGL_Command ** outCommand )
{
    riGL_CommandBlock * block   = NULL;
    riGL_Command *      command = NULL;
    riStatus            status  = RI_ERROR_UNKNOWN;

    RI_GUARD( allocator != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outCommand != NULL, RI_ERROR_INVALID_PARAM );

    *outCommand = NULL;

    // Block Selection
    //----------------------------------------------------------
    block = allocator->currentBlock;
    if( block == NULL || block->used >= block->capacity )
        {
            status = _rhioGL_command_allocator_obtain_block( allocator );
            if( RI_FAILED( status ) ) return status;

            block = allocator->currentBlock;
        }

    // Command Encoding
    //----------------------------------------------------------
    command       = &block->commands[block->used++];
    command->type = (riU32)type;

#        if defined( RHIO_DEBUG )
    ++allocator->commandCount;
    if( allocator->peakCommandCount < allocator->commandCount )
        {
            allocator->peakCommandCount = allocator->commandCount;
        }
#        endif

    *outCommand = command;

    return RI_SUCCESS;
}

// Release all commands recorded in a GL command list
static void
_rhioGL_command_list_clear_commands( riGL_CommandList * commandList )
{
    if( NULL == commandList ) return;

    _rhioGL_command_allocator_reset( &commandList->commandAllocator );
}

// Append target binding
static riStatus
_rhioGL_append_bind_default_framebuffer_command( riGL_CommandList * commandList )
{
    riGL_Command * command = NULL;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    return _rhioGL_command_allocator_write(
        &commandList->commandAllocator, RI_GL_COMMAND_BIND_DEFAULT_FRAMEBUFFER, &command );
}

// Append viewport state
static riStatus
_rhioGL_append_set_viewport_command( riGL_CommandList * commandList, riU32 x, riU32 y, riU32 width, riU32 height )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_SET_VIEWPORT, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.setRect.x      = x;
    command->data.setRect.y      = y;
    command->data.setRect.width  = width;
    command->data.setRect.height = height;

    return RI_SUCCESS;
}

// Append scissor rectangle state
static riStatus
_rhioGL_append_set_scissor_command( riGL_CommandList * commandList, riU32 x, riU32 y, riU32 width, riU32 height )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_SET_SCISSOR, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.setRect.x      = x;
    command->data.setRect.y      = y;
    command->data.setRect.width  = width;
    command->data.setRect.height = height;

    return RI_SUCCESS;
}

// Append scissor enable state
static riStatus
_rhioGL_append_set_scissor_enabled_command( riGL_CommandList * commandList, bool enabled )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write(
        &commandList->commandAllocator, RI_GL_COMMAND_SET_SCISSOR_ENABLED, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.setEnabled.enabled = enabled ? 1u : 0u;

    return RI_SUCCESS;
}

// Append clear color state
static riStatus
_rhioGL_append_set_clear_color_command( riGL_CommandList * commandList, const riF32 color[4] )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( color != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_SET_CLEAR_COLOR, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.setClearColor.r = color[0];
    command->data.setClearColor.g = color[1];
    command->data.setClearColor.b = color[2];
    command->data.setClearColor.a = color[3];

    return RI_SUCCESS;
}

// Append clear depth/stencil state
static riStatus
_rhioGL_append_set_clear_depth_stencil_command( riGL_CommandList * commandList, riF32 depth, riU8 stencil,
                                                bool clearDepth, bool clearStencil )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write(
        &commandList->commandAllocator, RI_GL_COMMAND_SET_CLEAR_DEPTH_STENCIL, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.setClearDepthStencil.depth        = depth;
    command->data.setClearDepthStencil.stencil      = stencil;
    command->data.setClearDepthStencil.clearDepth   = clearDepth ? 1u : 0u;
    command->data.setClearDepthStencil.clearStencil = clearStencil ? 1u : 0u;

    return RI_SUCCESS;
}

// Append framebuffer clear execution
static riStatus
_rhioGL_append_clear_command( riGL_CommandList * commandList, riU32 mask )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_CLEAR, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.clear.mask = mask;

    return RI_SUCCESS;
}

// Append render pipeline bind
static riStatus
_rhioGL_append_bind_pipeline_command( riGL_CommandList * commandList, riRenderPipeline pipeline )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( pipeline != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_BIND_PIPELINE, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.bindPipeline.pipeline = pipeline;

    return RI_SUCCESS;
}

// Append shader-buffer bind
static riStatus
_rhioGL_append_bind_shader_buffer_command( riGL_CommandList * commandList, riU32 slot, riBuffer buffer, riU32 offset )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( buffer != NULL, RI_ERROR_INVALID_PARAM );

    status
        = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_BIND_SHADER_BUFFER, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.bindShaderBuffer.buffer = buffer;
    command->data.bindShaderBuffer.offset = offset;
    command->data.bindShaderBuffer.slot   = slot;

    return RI_SUCCESS;
}

// Append non-indexed draw
static riStatus
_rhioGL_append_draw_command( riGL_CommandList * commandList, riU32 vertexCount, riU32 instanceCount, riU32 firstVertex,
                             riU32 firstInstance )
{
    riGL_Command * command = NULL;
    riStatus       status  = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );

    status = _rhioGL_command_allocator_write( &commandList->commandAllocator, RI_GL_COMMAND_DRAW, &command );
    if( RI_FAILED( status ) ) return status;

    command->data.draw.vertexCount   = vertexCount;
    command->data.draw.instanceCount = instanceCount;
    command->data.draw.firstVertex   = firstVertex;
    command->data.draw.firstInstance = firstInstance;

    return RI_SUCCESS;
}

//----------------------------------------------------------------------------------
// OpenGL Command Execution
//----------------------------------------------------------------------------------

// Execute all commands recorded in a GL command list
static riStatus
_rhioGL_execute_command_list( riGL_CommandList * commandList )
{
    riGL_CommandQueue *       glQueue                                              = NULL;
    riGL_Device *             glDevice                                             = NULL;
    const riGL_CommandBlock * block                                                = NULL;
    const riGL_Command *      command                                              = NULL;
    riGL_RenderPipeline *     pipeline                                             = NULL;
    riGL_Buffer *             shaderBuffers[RHIO_MAX_SHADER_BUFFER_BINDINGS]       = RI_ZERO_INIT;
    riU32                     shaderBufferOffsets[RHIO_MAX_SHADER_BUFFER_BINDINGS] = RI_ZERO_INIT;
    riU32                     i                                                    = 0u;
    riStatus                  status                                               = RI_ERROR_UNKNOWN;

    RI_GUARD( commandList != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( commandList->queue != NULL, RI_ERROR_INVALID_STATE );

    glQueue  = (riGL_CommandQueue *)commandList->queue;
    glDevice = glQueue->device;
    RI_GUARD( glDevice != NULL, RI_ERROR_INVALID_STATE );

#        if defined( RHIO_GL_HAS_VERTEX_ARRAY_OBJECT )
    if( glDevice->defaultVertexArray != 0u )
        {
            glBindVertexArray( glDevice->defaultVertexArray );
        }
#        endif

    // Command Stream Decode
    //----------------------------------------------------------
    block = commandList->commandAllocator.firstBlock;
    while( block != NULL )
        {
            for( i = 0u; i < block->used; ++i )
                {
                    command = &block->commands[i];

                    switch( (riGL_CommandType)command->type )
                        {
                        case RI_GL_COMMAND_BIND_DEFAULT_FRAMEBUFFER: _rhioGL_bind_default_framebuffer(); break;

                        case RI_GL_COMMAND_SET_VIEWPORT:
                            glViewport( (GLint)command->data.setRect.x,
                                        (GLint)command->data.setRect.y,
                                        (GLsizei)command->data.setRect.width,
                                        (GLsizei)command->data.setRect.height );
                            break;

                        case RI_GL_COMMAND_SET_SCISSOR:
                            glScissor( (GLint)command->data.setRect.x,
                                       (GLint)command->data.setRect.y,
                                       (GLsizei)command->data.setRect.width,
                                       (GLsizei)command->data.setRect.height );
                            break;

                        case RI_GL_COMMAND_SET_SCISSOR_ENABLED:
                            if( command->data.setEnabled.enabled )
                                {
                                    glEnable( GL_SCISSOR_TEST );
                                }
                            else
                                {
                                    glDisable( GL_SCISSOR_TEST );
                                }
                            break;

                        case RI_GL_COMMAND_SET_CLEAR_COLOR:
                            glClearColor( command->data.setClearColor.r,
                                          command->data.setClearColor.g,
                                          command->data.setClearColor.b,
                                          command->data.setClearColor.a );
                            break;

                        case RI_GL_COMMAND_SET_CLEAR_DEPTH_STENCIL:
                            if( command->data.setClearDepthStencil.clearDepth )
                                {
                                    glDepthMask( GL_TRUE );
                                    glClearDepth( command->data.setClearDepthStencil.depth );
                                }

                            if( command->data.setClearDepthStencil.clearStencil )
                                {
                                    glStencilMask( 0xFFFFFFFFu );
                                    glClearStencil( (GLint)command->data.setClearDepthStencil.stencil );
                                }
                            break;

                        case RI_GL_COMMAND_CLEAR:
                            glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
                            glClear( (GLbitfield)command->data.clear.mask );
                            break;

                        case RI_GL_COMMAND_BIND_PIPELINE:
                            pipeline = (riGL_RenderPipeline *)command->data.bindPipeline.pipeline;
                            if( pipeline == NULL )
                                {
                                    return RI_ERROR_INVALID_STATE;
                                }
                            status = _rhioGL_apply_pipeline_state( pipeline );
                            if( RI_FAILED( status ) ) return status;
                            glUseProgram( pipeline->program );
                            break;

                        case RI_GL_COMMAND_BIND_SHADER_BUFFER:
                            if( command->data.bindShaderBuffer.slot >= RHIO_MAX_SHADER_BUFFER_BINDINGS )
                                {
                                    return RI_ERROR_INVALID_STATE;
                                }
                            shaderBuffers[command->data.bindShaderBuffer.slot]
                                = (riGL_Buffer *)command->data.bindShaderBuffer.buffer;
                            shaderBufferOffsets[command->data.bindShaderBuffer.slot]
                                = command->data.bindShaderBuffer.offset;
                            break;

                        case RI_GL_COMMAND_DRAW:
                            status
                                = _rhioGL_execute_draw_command( command, pipeline, shaderBuffers, shaderBufferOffsets );
                            if( RI_FAILED( status ) ) return status;
                            break;

                        default: return RI_ERROR_INVALID_STATE;
                        }
                }

            block = block->next;
        }

    return RI_SUCCESS;
}

// Execute one non-indexed draw
static riStatus
_rhioGL_execute_draw_command( const riGL_Command * command, riGL_RenderPipeline * pipeline,
                              riGL_Buffer * shaderBuffers[RHIO_MAX_SHADER_BUFFER_BINDINGS],
                              const riU32   shaderBufferOffsets[RHIO_MAX_SHADER_BUFFER_BINDINGS] )
{
    riGL_Buffer * buffer       = NULL;
    riSize        bufferOffset = 0u;
    riSize        range        = 0u;
    riSize        requiredSize = (riSize)RHIO_GL_TRIANGLE_VERTEX_COUNT * (riSize)sizeof( riF32 ) * 8u;

    RI_GUARD( command != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( pipeline != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( shaderBuffers != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( shaderBufferOffsets != NULL, RI_ERROR_INVALID_PARAM );

#        if !defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    return RI_ERROR_BACKEND_UNAVAIL;
#        else
    buffer       = shaderBuffers[RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING];
    bufferOffset = (riSize)shaderBufferOffsets[RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING];

    RI_GUARD( buffer != NULL, RI_ERROR_INVALID_STATE );
    RI_GUARD( bufferOffset <= buffer->info.size, RI_ERROR_INVALID_STATE );
    RI_GUARD( requiredSize <= buffer->info.size - bufferOffset, RI_ERROR_INVALID_STATE );

    range = buffer->info.size - bufferOffset;
    glBindBufferRange( GL_UNIFORM_BUFFER,
                       (GLuint)RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING,
                       buffer->buffer,
                       (GLintptr)bufferOffset,
                       (GLsizeiptr)range );

    glDrawArrays( pipeline->primitive, (GLint)command->data.draw.firstVertex, (GLsizei)command->data.draw.vertexCount );

    return RI_SUCCESS;
#        endif
}

// Bind the OpenGL default framebuffer
static void
_rhioGL_bind_default_framebuffer( void )
{
    glBindFramebuffer( GL_FRAMEBUFFER, 0u );
}

// Initialize desktop GL entry points through GLEW; GLES and Apple builds link symbols directly
static riStatus
_rhioGL_load_entrypoints( void )
{
#        if defined( RHIO_GL_USE_GLEW )
    GLenum glewStatus = GLEW_OK;

    glewExperimental  = GL_TRUE;
    glewStatus        = glewInit();
    if( RI_UNLIKELY( glewStatus != GLEW_OK ) )
        {
            TRACELOG( RI_LOG_ERROR,
                      "BACKEND GL: Failed to initialize GLEW: %s",
                      (const char *)glewGetErrorString( glewStatus ) );
            return RI_ERROR_BACKEND_INIT;
        }

    UNUSED( glGetError() );
#        endif

    return RI_SUCCESS;
}

// Compile one GLSL shader stage
static riStatus
_rhioGL_compile_shader( GLenum shaderType, const char * source, GLuint * outShader )
{
    GLuint shader = 0u;
    GLint  status = GL_FALSE;
    char   infoLog[1024];

    RI_GUARD( source != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( outShader != NULL, RI_ERROR_INVALID_PARAM );

    *outShader = 0u;

    shader     = glCreateShader( shaderType );
    if( 0u == shader ) return RI_ERROR_SHADER_COMPILE;

    glShaderSource( shader, 1, &source, NULL );
    glCompileShader( shader );
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    if( status != GL_TRUE )
        {
            infoLog[0] = '\0';
            glGetShaderInfoLog( shader, (GLsizei)sizeof( infoLog ), NULL, infoLog );
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: Shader compile failed: %s", infoLog );
            glDeleteShader( shader );
            return RI_ERROR_SHADER_COMPILE;
        }

    *outShader = shader;

    return RI_SUCCESS;
}

// Link the hardcoded passthrough shader used by the minimal triangle pipeline
static riStatus
_rhioGL_create_builtin_program( GLuint * outProgram )
{
#        if !defined( RHIO_GL_HAS_UNIFORM_BUFFER_VERTEX_PULLING )
    UNUSED( outProgram );
    return RI_ERROR_BACKEND_UNAVAIL;
#        elif defined( RHIO_BACKEND_OPENGLES3 )
    static const char * vertexSource   = "#version 300 es\n"
                                         "precision mediump float;\n"
                                         "struct RhioVertex {\n"
                                         "    vec4 position;\n"
                                         "    vec4 color;\n"
                                         "};\n"
                                         "layout(std140) uniform RhioTriangleVertices {\n"
                                         "    RhioVertex rhio_vertices[3];\n"
                                         "};\n"
                                         "out vec4 v_color;\n"
                                         "void main(void) {\n"
                                         "    RhioVertex v = rhio_vertices[gl_VertexID];\n"
                                         "    gl_Position = vec4(v.position.xy, 0.0, 1.0);\n"
                                         "    v_color = v.color;\n"
                                         "}\n";
    static const char * fragmentSource = "#version 300 es\n"
                                         "precision mediump float;\n"
                                         "in vec4 v_color;\n"
                                         "out vec4 rhio_frag_color;\n"
                                         "void main(void) {\n"
                                         "    rhio_frag_color = v_color;\n"
                                         "}\n";
#        else
    static const char * vertexSource   = "#version 330 core\n"
                                         "struct RhioVertex {\n"
                                         "    vec4 position;\n"
                                         "    vec4 color;\n"
                                         "};\n"
                                         "layout(std140) uniform RhioTriangleVertices {\n"
                                         "    RhioVertex rhio_vertices[3];\n"
                                         "};\n"
                                         "out vec4 v_color;\n"
                                         "void main(void) {\n"
                                         "    RhioVertex v = rhio_vertices[gl_VertexID];\n"
                                         "    gl_Position = vec4(v.position.xy, 0.0, 1.0);\n"
                                         "    v_color = v.color;\n"
                                         "}\n";
    static const char * fragmentSource = "#version 330 core\n"
                                         "in vec4 v_color;\n"
                                         "out vec4 rhio_frag_color;\n"
                                         "void main(void) {\n"
                                         "    rhio_frag_color = v_color;\n"
                                         "}\n";
#        endif

    GLuint   vertexShader   = 0u;
    GLuint   fragmentShader = 0u;
    GLuint   program        = 0u;
    GLuint   blockIndex     = GL_INVALID_INDEX;
    GLint    linkStatus     = GL_FALSE;
    riStatus status         = RI_ERROR_UNKNOWN;
    char     infoLog[1024];

    RI_GUARD( outProgram != NULL, RI_ERROR_INVALID_PARAM );
    *outProgram = 0u;

    status      = _rhioGL_compile_shader( GL_VERTEX_SHADER, vertexSource, &vertexShader );
    if( RI_FAILED( status ) ) return status;

    status = _rhioGL_compile_shader( GL_FRAGMENT_SHADER, fragmentSource, &fragmentShader );
    if( RI_FAILED( status ) )
        {
            glDeleteShader( vertexShader );
            return status;
        }

    program = glCreateProgram();
    if( program == 0u )
        {
            glDeleteShader( vertexShader );
            glDeleteShader( fragmentShader );
            return RI_ERROR_SHADER_COMPILE;
        }

    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );
    glLinkProgram( program );

    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );

    glGetProgramiv( program, GL_LINK_STATUS, &linkStatus );
    if( linkStatus != GL_TRUE )
        {
            infoLog[0] = '\0';
            glGetProgramInfoLog( program, (GLsizei)sizeof( infoLog ), NULL, infoLog );
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: Program link failed: %s", infoLog );
            glDeleteProgram( program );
            return RI_ERROR_SHADER_COMPILE;
        }

    blockIndex = glGetUniformBlockIndex( program, "RhioTriangleVertices" );
    if( blockIndex == GL_INVALID_INDEX )
        {
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: Builtin triangle uniform block was optimized out or not found" );
            glDeleteProgram( program );
            return RI_ERROR_SHADER_COMPILE;
        }

    glUniformBlockBinding( program, blockIndex, (GLuint)RHIO_GL_TRIANGLE_VERTEX_PULL_BINDING );

    *outProgram = program;

    return RI_SUCCESS;
}

// Validate the public render-pipeline descriptor against the current GL backend surface
static riStatus
_rhioGL_validate_render_pipeline_info( const riRenderPipelineInfo * info )
{
    GLenum   ignored = 0u;
    riStatus status  = RI_SUCCESS;
    riU32    i       = 0u;

    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->supportsIndirectCommands <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->depthTestEnable <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->depthWriteEnable <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->depthClampEnable <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->stencilTestEnable <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->stencilWriteEnable <= 1u, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->renderTargetCount <= RHIO_MAX_COLOR_ATTACHMENTS, RI_ERROR_INVALID_PARAM );

    RI_GUARD( info->supportsIndirectCommands == 0u, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( info->vertexShader == NULL, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( info->fragmentShader == NULL, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( info->meshShader == NULL, RI_ERROR_BACKEND_UNAVAIL );
    RI_GUARD( info->taskShader == NULL, RI_ERROR_BACKEND_UNAVAIL );

    TRY_CONVERT( _rhioGL_topology_to_gl, info->topology, &ignored );

    switch( info->fillMode )
        {
        case RI_PIPELINE_FILL_MODE_SOLID: break;
        case RI_PIPELINE_FILL_MODE_WIREFRAME:
#        if !defined( RHIO_GL_BACKEND_DESKTOP )
            return RI_ERROR_BACKEND_UNAVAIL;
#        else
            break;
#        endif
        default: return RI_ERROR_INVALID_PARAM;
        }

    switch( info->cullMode )
        {
        case RI_PIPELINE_CULL_MODE_NONE:
        case RI_PIPELINE_CULL_MODE_FRONT:
        case RI_PIPELINE_CULL_MODE_BACK:  break;
        default:                          return RI_ERROR_INVALID_PARAM;
        }

    switch( info->frontFace )
        {
        case RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE:
        case RI_PIPELINE_FRONT_FACE_CLOCKWISE:         break;
        default:                                       return RI_ERROR_INVALID_PARAM;
        }

    TRY_CONVERT( _rhioGL_compare_operation_to_gl, info->depthCompareOp, &ignored );
    TRY_CONVERT( _rhioGL_compare_operation_to_gl, info->stencilCompareOp, &ignored );

    if( info->depthClampEnable != 0u )
        {
#        if !defined( RHIO_GL_BACKEND_DESKTOP )
            return RI_ERROR_BACKEND_UNAVAIL;
#        endif
        }

    if( info->depthStencilFormat
        != RHIO_TEXTURE_FORMAT_UNDEFINED
        && info->depthStencilFormat
        != RHIO_TEXTURE_FORMAT_D24_UNORM_S8_UINT
        && info->depthStencilFormat
        != RHIO_TEXTURE_FORMAT_D32_FLOAT_S8_UINT )
        {
            return RI_ERROR_INVALID_PARAM;
        }

    for( i = 0u; i < info->renderTargetCount; ++i )
        {
            RI_GUARD( info->blendEnable[i] <= 1u, RI_ERROR_INVALID_PARAM );
            RI_GUARD( info->renderTargetFormats[i] != RHIO_TEXTURE_FORMAT_UNDEFINED, RI_ERROR_INVALID_PARAM );

            RI_GUARD( info->renderTargetFormats[i] != RHIO_TEXTURE_FORMAT_D24_UNORM_S8_UINT, RI_ERROR_INVALID_PARAM );
            RI_GUARD( info->renderTargetFormats[i] != RHIO_TEXTURE_FORMAT_D32_FLOAT_S8_UINT, RI_ERROR_INVALID_PARAM );

            if( i > 0u && info->blendEnable[i] != 0u ) return RI_ERROR_BACKEND_UNAVAIL;

            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendSrcRgbFactor[i], &ignored );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendDstRgbFactor[i], &ignored );
            TRY_CONVERT( _rhioGL_blend_operation_to_gl, info->blendRgbOp[i], &ignored );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendSrcAlphaFactor[i], &ignored );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendDstAlphaFactor[i], &ignored );
            TRY_CONVERT( _rhioGL_blend_operation_to_gl, info->blendAlphaOp[i], &ignored );
        }

    return RI_SUCCESS;
}

// Apply the GL-representable subset of the render-pipeline state at command execution time
static riStatus
_rhioGL_apply_pipeline_state( const riGL_RenderPipeline * pipeline )
{
    const riRenderPipelineInfo * info        = NULL;
    GLenum                       compareOp   = GL_ALWAYS;
    GLenum                       blendSrcRgb = GL_ONE;
    GLenum                       blendDstRgb = GL_ZERO;
    GLenum                       blendRgbOp  = GL_FUNC_ADD;
    GLenum                       blendSrcA   = GL_ONE;
    GLenum                       blendDstA   = GL_ZERO;
    GLenum                       blendAOp    = GL_FUNC_ADD;
    riStatus                     status      = RI_SUCCESS;

    RI_GUARD( pipeline != NULL, RI_ERROR_INVALID_PARAM );

    info = &pipeline->info;

    switch( info->fillMode )
        {
        case RI_PIPELINE_FILL_MODE_SOLID:
#        if defined( RHIO_GL_BACKEND_DESKTOP )
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
#        endif
            break;
        case RI_PIPELINE_FILL_MODE_WIREFRAME:
#        if defined( RHIO_GL_BACKEND_DESKTOP )
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            break;
#        else
            return RI_ERROR_BACKEND_UNAVAIL;
#        endif
        default: return RI_ERROR_INVALID_STATE;
        }

    switch( info->cullMode )
        {
        case RI_PIPELINE_CULL_MODE_NONE: glDisable( GL_CULL_FACE ); break;
        case RI_PIPELINE_CULL_MODE_FRONT:
            glEnable( GL_CULL_FACE );
            glCullFace( GL_FRONT );
            break;
        case RI_PIPELINE_CULL_MODE_BACK:
            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            break;
        default: return RI_ERROR_INVALID_STATE;
        }

    switch( info->frontFace )
        {
        case RI_PIPELINE_FRONT_FACE_COUNTER_CLOCKWISE: glFrontFace( GL_CCW ); break;
        case RI_PIPELINE_FRONT_FACE_CLOCKWISE:         glFrontFace( GL_CW ); break;
        default:                                       return RI_ERROR_INVALID_STATE;
        }

    if( info->depthTestEnable != 0u )
        {
            TRY_CONVERT( _rhioGL_compare_operation_to_gl, info->depthCompareOp, &compareOp );
            glEnable( GL_DEPTH_TEST );
            glDepthFunc( compareOp );
        }
    else
        {
            glDisable( GL_DEPTH_TEST );
        }

    glDepthMask( info->depthWriteEnable != 0u ? GL_TRUE : GL_FALSE );

    if( info->depthClampEnable != 0u )
        {
#        if defined( RHIO_GL_BACKEND_DESKTOP )
            glEnable( GL_DEPTH_CLAMP );
#        else
            return RI_ERROR_BACKEND_UNAVAIL;
#        endif
        }
    else
        {
#        if defined( RHIO_GL_BACKEND_DESKTOP )
            glDisable( GL_DEPTH_CLAMP );
#        endif
        }

    if( info->stencilTestEnable != 0u )
        {
            TRY_CONVERT( _rhioGL_compare_operation_to_gl, info->stencilCompareOp, &compareOp );
            glEnable( GL_STENCIL_TEST );
            glStencilFunc( compareOp, 0, 0xFFFFFFFFu );
        }
    else
        {
            glDisable( GL_STENCIL_TEST );
        }

    glStencilMask( info->stencilWriteEnable != 0u ? 0xFFFFFFFFu : 0u );

    if( info->renderTargetCount > 0u && info->blendEnable[0] != 0u )
        {
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendSrcRgbFactor[0], &blendSrcRgb );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendDstRgbFactor[0], &blendDstRgb );
            TRY_CONVERT( _rhioGL_blend_operation_to_gl, info->blendRgbOp[0], &blendRgbOp );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendSrcAlphaFactor[0], &blendSrcA );
            TRY_CONVERT( _rhioGL_blend_factor_to_gl, info->blendDstAlphaFactor[0], &blendDstA );
            TRY_CONVERT( _rhioGL_blend_operation_to_gl, info->blendAlphaOp[0], &blendAOp );

            glEnable( GL_BLEND );
            glBlendFuncSeparate( blendSrcRgb, blendDstRgb, blendSrcA, blendDstA );
            glBlendEquationSeparate( blendRgbOp, blendAOp );
        }
    else
        {
            glDisable( GL_BLEND );
        }

    return RI_SUCCESS;
}

// Map public topology to GL topology
static riStatus
_rhioGL_topology_to_gl( riPipelineTopology topology, GLenum * outTopology )
{
    RI_GUARD( outTopology != NULL, RI_ERROR_INVALID_PARAM );

    switch( topology )
        {
        case RI_PIPELINE_TOPOLOGY_TRIANGLE_LIST: *outTopology = GL_TRIANGLES; return RI_SUCCESS;
        case RI_PIPELINE_TOPOLOGY_LINE_LIST:     *outTopology = GL_LINES; return RI_SUCCESS;
        case RI_PIPELINE_TOPOLOGY_POINT_LIST:    *outTopology = GL_POINTS; return RI_SUCCESS;
        default:                                 return RI_ERROR_INVALID_PARAM;
        }
}

// Map public compare operation to GL compare operation
static riStatus
_rhioGL_compare_operation_to_gl( riCompareOperation operation, GLenum * outOperation )
{
    RI_GUARD( outOperation != NULL, RI_ERROR_INVALID_PARAM );

    switch( operation )
        {
        case RI_COMPARE_OPERATION_NEVER:         *outOperation = GL_NEVER; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_LESS:          *outOperation = GL_LESS; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_EQUAL:         *outOperation = GL_EQUAL; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_LESS_EQUAL:    *outOperation = GL_LEQUAL; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_GREATER:       *outOperation = GL_GREATER; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_NOT_EQUAL:     *outOperation = GL_NOTEQUAL; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_GREATER_EQUAL: *outOperation = GL_GEQUAL; return RI_SUCCESS;
        case RI_COMPARE_OPERATION_ALWAYS:        *outOperation = GL_ALWAYS; return RI_SUCCESS;
        default:                                 return RI_ERROR_INVALID_PARAM;
        }
}

// Map public blend factor to GL blend factor
static riStatus
_rhioGL_blend_factor_to_gl( riBlendFactor factor, GLenum * outFactor )
{
    RI_GUARD( outFactor != NULL, RI_ERROR_INVALID_PARAM );

    switch( factor )
        {
        case RI_BLEND_FACTOR_ZERO:                *outFactor = GL_ZERO; return RI_SUCCESS;
        case RI_BLEND_FACTOR_ONE:                 *outFactor = GL_ONE; return RI_SUCCESS;
        case RI_BLEND_FACTOR_SRC_COLOR:           *outFactor = GL_SRC_COLOR; return RI_SUCCESS;
        case RI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: *outFactor = GL_ONE_MINUS_SRC_COLOR; return RI_SUCCESS;
        case RI_BLEND_FACTOR_DST_COLOR:           *outFactor = GL_DST_COLOR; return RI_SUCCESS;
        case RI_BLEND_FACTOR_ONE_MINUS_DST_COLOR: *outFactor = GL_ONE_MINUS_DST_COLOR; return RI_SUCCESS;
        case RI_BLEND_FACTOR_SRC_ALPHA:           *outFactor = GL_SRC_ALPHA; return RI_SUCCESS;
        case RI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: *outFactor = GL_ONE_MINUS_SRC_ALPHA; return RI_SUCCESS;
        case RI_BLEND_FACTOR_DST_ALPHA:           *outFactor = GL_DST_ALPHA; return RI_SUCCESS;
        case RI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: *outFactor = GL_ONE_MINUS_DST_ALPHA; return RI_SUCCESS;
        case RI_BLEND_FACTOR_SRC_ALPHA_SATURATE:  *outFactor = GL_SRC_ALPHA_SATURATE; return RI_SUCCESS;
        case RI_BLEND_FACTOR_BLEND_COLOR:         *outFactor = GL_CONSTANT_COLOR; return RI_SUCCESS;
        default:                                  return RI_ERROR_INVALID_PARAM;
        }
}

// Map public blend operation to GL blend operation
static riStatus
_rhioGL_blend_operation_to_gl( riBlendOperation operation, GLenum * outOperation )
{
    RI_GUARD( outOperation != NULL, RI_ERROR_INVALID_PARAM );

    switch( operation )
        {
        case RI_BLEND_OPERATION_ADD:              *outOperation = GL_FUNC_ADD; return RI_SUCCESS;
        case RI_BLEND_OPERATION_SUBTRACT:         *outOperation = GL_FUNC_SUBTRACT; return RI_SUCCESS;
        case RI_BLEND_OPERATION_REVERSE_SUBTRACT: *outOperation = GL_FUNC_REVERSE_SUBTRACT; return RI_SUCCESS;
        case RI_BLEND_OPERATION_MIN:              *outOperation = GL_MIN; return RI_SUCCESS;
        case RI_BLEND_OPERATION_MAX:              *outOperation = GL_MAX; return RI_SUCCESS;
        default:                                  return RI_ERROR_INVALID_PARAM;
        }
}

// Populate the backend descriptor with OpenGL implementations
static riStatus
_rhioGL_registerBackend( riBackendDesc * desc )
{
    riBackend backend = RI_BACKEND_OPENGL;
    riFlags   flags   = 0;

    RI_GUARD( desc != NULL, RI_ERROR_INVALID_PARAM );

    // Preserve Requested Flavor
    //----------------------------------------------------------
    flags = desc->flags;

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

    // Backend VTable
    //----------------------------------------------------------
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

    RI_GUARD( desc != NULL, RI_ERROR_INVALID_PARAM );

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

#    pragma region "Validate VTable"

// Shorthand for vtable member guard
#    define RI_GUARD_VTABLE( slot ) RI_GUARD( vtable->slot != NULL, RI_ERROR_INVALID_STATE )

// Ensure backend exposes callbacks required by the device API
static riStatus
_rhioValidateDeviceVTable( const riDeviceVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( init );
    RI_GUARD_VTABLE( shutdown );
    RI_GUARD_VTABLE( create_command_queue );

    return RI_SUCCESS;
}

// Ensure a command queue exposes callbacks required by the public queue API
static riStatus
_rhioValidateCommandQueueVTable( const riCommandQueueVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( create_command_list );
    RI_GUARD_VTABLE( destroy_command_queue );

    // Submit Dispatch Validation
    //----------------------------------------------------------
    // NOTE: The batched slot is optional. minimal/custom backends may expose only
    // single-list submit and let the frontend fan out rhioCommandQueueSubmitInfo()
    if( RI_UNLIKELY( vtable->submit_command_list == NULL && vtable->submit_command_lists == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "COMMAND_QUEUE: Missing required submit vtable slot" );
            return RI_ERROR_INVALID_STATE;
        }

    return RI_SUCCESS;
}

// Ensure a command list exposes callbacks required by the public command-list API
static riStatus
_rhioValidateCommandListVTable( const riCommandListVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( destroy_command_list );
    RI_GUARD_VTABLE( begin );
    RI_GUARD_VTABLE( end );
    RI_GUARD_VTABLE( reset );
    RI_GUARD_VTABLE( begin_render_pass );

    return RI_SUCCESS;
}

// Ensure a swapchain exposes callbacks required by the public swapchain API
static riStatus
_rhioValidateSwapchainVTable( const riSwapchainVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( destroy_swapchain );
    RI_GUARD_VTABLE( get_current_texture );
    RI_GUARD_VTABLE( present );

    return RI_SUCCESS;
}

// Ensure a buffer exposes callbacks required by the public buffer API
static riStatus
_rhioValidateBufferVTable( const riBufferVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( destroy_buffer );
    RI_GUARD_VTABLE( get_buffer_info );

    return RI_SUCCESS;
}

// Ensure a texture view exposes callbacks required by the public texture-view API
static riStatus
_rhioValidateTextureViewVTable( const riTextureViewVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( destroy_texture_view );
    RI_GUARD_VTABLE( get_texture_view_info );

    return RI_SUCCESS;
}

// Ensure a render pipeline exposes callbacks required by the public pipeline API
static riStatus
_rhioValidateRenderPipelineVTable( const riRenderPipelineVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( destroy_render_pipeline );
    RI_GUARD_VTABLE( get_render_pipeline_info );

    return RI_SUCCESS;
}

// Ensure a render pass exposes callbacks required by the public render-pass API
static riStatus
_rhioValidateRenderPassVTable( const riRenderPassVTable * vtable )
{
    RI_GUARD( vtable != NULL, RI_ERROR_INVALID_STATE );

    RI_GUARD_VTABLE( end );

    return RI_SUCCESS;
}

#    undef RI_GUARD_VTABLE

#    pragma endregion // Validate VTable

// Convert device creation info into a concrete backend descriptor
static riStatus
_rhioResolveBackendDesc( const riDeviceInfo * info, riBackendDesc * desc )
{
    const riFlags validFlags = (riFlags)RI_DEVICE_FLAG_DEBUG;

    RI_GUARD( info != NULL, RI_ERROR_INVALID_PARAM );
    RI_GUARD( desc != NULL, RI_ERROR_INVALID_PARAM );

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
