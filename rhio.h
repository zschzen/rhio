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
#define RHIO_VERSION_MINOR 1
#define RHIO_VERSION_PATCH 0
#define RHIO_VERSION       ( ( RHIO_VERSION_MAJOR << 16 ) | ( RHIO_VERSION_MINOR << 8 ) | RHIO_VERSION_PATCH )
#define RHIO_VERSION_STRING                                                                                            \
    RHIO_STR( RHIO_VERSION_MAJOR ) "." RHIO_STR( RHIO_VERSION_MINOR ) "." RHIO_STR( RHIO_VERSION_PATCH )

// Compile-time resource limits
#ifndef RHIO_MAX_COLOR_ATTACHMENTS
#    define RHIO_MAX_COLOR_ATTACHMENTS 8
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
#define RI_SUCCEEDED( result ) ( ( result ) >= RI_SUCCESS )
#define RI_FAILED( result )    ( RI_UNLIKELY( !RI_SUCCEEDED( result ) ) )

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

// Texture dimensionality for concrete textures and views
typedef enum
{
    RI_TEXTURE_DIMENSIONS_UNDEFINED = 0,
    RI_TEXTURE_DIMENSIONS_1D,
    RI_TEXTURE_DIMENSIONS_2D,
    RI_TEXTURE_DIMENSIONS_2D_ARRAY,
    RI_TEXTURE_DIMENSIONS_3D,
    RI_TEXTURE_DIMENSIONS_CUBE,

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

// Attachment parameters consumed when beginning a render pass
typedef struct riRenderPassAttachmentInfo
{
    riTextureView           textureView; // Render target/depth view
    riAttachmentLoadAction  loadAction;  // Attachment load behavior
    riAttachmentStoreAction storeAction; // Attachment store behavior

    riColor clearColor;                  // Used for color attachments when loadAction is CLEAR
    riF32   clearDepth;                  // Used for depth attachments when loadAction is CLEAR
    riU8    clearStencil;                // Used for stencil attachments when loadAction is CLEAR

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

// Texture view dispatch table
typedef struct riTextureViewVTable
{
    void ( *destroy_texture_view )( riTextureView textureView );
    void ( *get_texture_view_info )( riTextureView textureView, riTextureViewInfo * outInfo );

} riTextureViewVTable;

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
DECLARE_RI_BASE( riCommandList );
DECLARE_RI_BASE( riSwapchain );
DECLARE_RI_BASE( riTexture );
DECLARE_RI_BASE( riTextureView );
DECLARE_RI_BASE( riRenderPass );

typedef struct riCommandQueueBase
{
    const riCommandQueueVTable * vtable;
} riCommandQueueBase;

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
static riStatus _rhioValidateCommandListVTable( const riCommandListVTable * vtable );
static riStatus _rhioValidateSwapchainVTable( const riSwapchainVTable * vtable );
static riStatus _rhioValidateTextureViewVTable( const riTextureViewVTable * vtable );
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
    riCommandListBase *  listBase  = NULL;
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

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD_NULL( listBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( listBase->vtable->begin, RI_ERROR_INVALID_STATE );

    return listBase->vtable->begin( commandList );
}

// Finish recording commands into a command list
RI_API riStatus
rhioCommandListEnd( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD_NULL( listBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( listBase->vtable->end, RI_ERROR_INVALID_STATE );

    return listBase->vtable->end( commandList );
}

// Reset a command list so it can record a new frame
RI_API riStatus
rhioCommandListReset( riCommandList commandList )
{
    riCommandListBase * listBase = NULL;

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD_NULL( listBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( listBase->vtable->reset, RI_ERROR_INVALID_STATE );

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
    RI_GUARD_NULL( outRenderPass, RI_ERROR_INVALID_PARAM );
    *outRenderPass = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    listBase = (riCommandListBase *)commandList;

    RI_GUARD_NULL( listBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( listBase->vtable->begin_render_pass, RI_ERROR_INVALID_STATE );

    // Backend Render Pass Begin
    //----------------------------------------------------------
    status = listBase->vtable->begin_render_pass( commandList, info, outRenderPass );
    if( RI_FAILED( status ) )
        {
            *outRenderPass = NULL;
            return status;
        }

    RI_GUARD_NULL( *outRenderPass, RI_ERROR_INVALID_STATE );

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

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

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

    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info->commandLists, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandListCount > 0u, RI_ERROR_INVALID_PARAM );

    queueBase = (riCommandQueueBase *)queue;

    RI_GUARD_NULL( queueBase->vtable, RI_ERROR_INVALID_STATE );

    // Submit Batch Validation
    //----------------------------------------------------------
    // Validate every handle before dispatch so the fallback path cannot submit a
    // partial batch before discovering a bad command list
    for( i = 0u; i < info->commandListCount; ++i )
        {
            RI_GUARD_NULL( info->commandLists[i], RI_ERROR_INVALID_PARAM );
        }

    if( queueBase->vtable->submit_command_lists != NULL )
        {
            return queueBase->vtable->submit_command_lists( queue, info );
        }

    RI_GUARD_NULL( queueBase->vtable->submit_command_list, RI_ERROR_INVALID_STATE );

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
    RI_GUARD_NULL( outSwapchain, RI_ERROR_INVALID_PARAM );
    *outSwapchain = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( deviceBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( deviceBase->vtable->create_swapchain, RI_ERROR_INVALID_STATE );

    // Backend Swapchain Creation
    //----------------------------------------------------------
    status = deviceBase->vtable->create_swapchain( device, queue, info, outSwapchain );
    if( RI_FAILED( status ) )
        {
            *outSwapchain = NULL;
            return status;
        }

    RI_GUARD_NULL( *outSwapchain, RI_ERROR_INVALID_STATE );

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

    RI_GUARD_NULL( outTexture, RI_ERROR_INVALID_PARAM );
    *outTexture = NULL;

    RI_GUARD_NULL( swapchain, RI_ERROR_INVALID_PARAM );

    swapchainBase = (riSwapchainBase *)swapchain;

    RI_GUARD_NULL( swapchainBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( swapchainBase->vtable->get_current_texture, RI_ERROR_INVALID_STATE );

    return swapchainBase->vtable->get_current_texture( swapchain, outTexture );
}

// Present the current swapchain image
RI_API riStatus
rhioSwapchainPresent( riSwapchain swapchain )
{
    riSwapchainBase * swapchainBase = NULL;

    RI_GUARD_NULL( swapchain, RI_ERROR_INVALID_PARAM );

    swapchainBase = (riSwapchainBase *)swapchain;

    RI_GUARD_NULL( swapchainBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( swapchainBase->vtable->present, RI_ERROR_INVALID_STATE );

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
    RI_GUARD_NULL( outTextureView, RI_ERROR_INVALID_PARAM );
    *outTextureView = NULL;

    // Input Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );

    deviceBase = (riDeviceBase *)device;

    // Device Dispatch Validation
    //----------------------------------------------------------
    RI_GUARD_NULL( deviceBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( deviceBase->vtable->create_texture_view, RI_ERROR_INVALID_STATE );

    // Backend Texture View Creation
    //----------------------------------------------------------
    status = deviceBase->vtable->create_texture_view( device, info, outTextureView );
    if( RI_FAILED( status ) )
        {
            *outTextureView = NULL;
            return status;
        }

    RI_GUARD_NULL( *outTextureView, RI_ERROR_INVALID_STATE );

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

    RI_GUARD_NULL( renderPass, RI_ERROR_INVALID_PARAM );

    passBase = (riRenderPassBase *)renderPass;

    RI_GUARD_NULL( passBase->vtable, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( passBase->vtable->end, RI_ERROR_INVALID_STATE );

    return passBase->vtable->end( renderPass );
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

// Default bytes per OpenGL command memory block
#        ifndef RHIO_GL_COMMAND_BLOCK_SIZE
#            define RHIO_GL_COMMAND_BLOCK_SIZE ( 16u * 1024u )
#        endif

//----------------------------------------------------------------------------------
// Platform GL Headers and Entry Points
//----------------------------------------------------------------------------------
// NOTE: rhio does not own an OpenGL context. The application/windowing owns it
// This block provides only the minimal types and headers

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

#            define _RHIO_GL_LOAD( type, name ) ( (type)name )

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
#            include <GL/glext.h>
#            include <windows.h>

typedef HDC   riGL_DeviceContext;
typedef HGLRC riGL_RenderContext;

#            define _RHIO_GL_LOAD( type, name ) ( (type)wglGetProcAddress( #name ) )

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

#            define _RHIO_GL_LOAD( type, name ) ( (type)name )

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

#                define _RHIO_GL_LOAD( type, name ) ( (type)name )

#            else /* RHIO_BACKEND_OPENGLES */

#                include <GL/gl.h>
#                include <GL/glext.h>
#                include <GL/glx.h>
#                include <X11/Xlib.h>

typedef Display *  riGL_DeviceContext;
typedef GLXContext riGL_RenderContext;

#                define _RHIO_GL_LOAD( type, name ) ( (type)glXGetProcAddress( (const GLubyte *)#name ) )

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

#        if !defined( RHIO_BACKEND_OPENGLES )                                                                          \
            && !defined( RHIO_BACKEND_OPENGLES2 )                                                                      \
            && !defined( RHIO_BACKEND_OPENGLES3 )                                                                      \
            && !defined( __APPLE__ )
#            define RHIO_GL_NEEDS_FRAMEBUFFER_BIND_LOADER
static PFNGLBINDFRAMEBUFFERPROC rhio_glBindFramebufferProc = NULL;
#        endif

//----------------------------------------------------------------------------------
// Types and Structures Definition                                    [>>GL_TYPES<<]
//----------------------------------------------------------------------------------

// Internal OpenGL device state
typedef struct riGL_Device
{
    riDeviceBase base; // Frontend handle dispatch

} riGL_Device;

typedef struct riGL_Texture
{
    riTextureBase base; // Frontend handle dispatch
    riTextureInfo info; // Normalized metadata
    bool          isDefaultFramebuffer;

} riGL_Texture;

typedef struct riGL_TextureView
{
    riTextureViewBase base;                 // Frontend handle dispatch
    riTextureViewInfo info;                 // Normalized view metadata
    riGL_Texture *    texture;              // Borrowed texture backing this view
    bool              isDefaultFramebuffer; // View targets framebuffer 0

} riGL_TextureView;

typedef enum riGL_SwapchainState
{
    RI_GL_SWAPCHAIN_STATE_IDLE = 0,
    RI_GL_SWAPCHAIN_STATE_ACQUIRED,
    RI_GL_SWAPCHAIN_STATE_LOST,

} riGL_SwapchainState;

typedef struct riGL_Swapchain
{
    riSwapchainBase     base;           // Frontend handle dispatch
    riSwapchainInfo     info;           // Creation info copy
    riGL_Texture        currentTexture; // Borrowed default-framebuffer facade
    riGL_SwapchainState state;          // Acquire / present lifetime state

} riGL_Swapchain;

typedef enum riGL_CommandListState
{
    RI_GL_COMMAND_LIST_STATE_INITIAL = 0,
    RI_GL_COMMAND_LIST_STATE_RECORDING,
    RI_GL_COMMAND_LIST_STATE_EXECUTABLE,
    RI_GL_COMMAND_LIST_STATE_PENDING,
    RI_GL_COMMAND_LIST_STATE_SUBMITTED,

} riGL_CommandListState;

typedef enum riGL_CommandType
{
    RI_GL_COMMAND_CLEAR_FRAMEBUFFER = 0,

} riGL_CommandType;

typedef struct riGL_CommandHeader
{
    riU32 type; // riGL_CommandType
    riU32 size; // Aligned command record size in bytes

} riGL_CommandHeader;

typedef struct riGL_ClearCommand
{
    GLbitfield mask;

    bool    hasColor;
    riColor color;

    bool  hasDepth;
    riF32 depth;

    bool hasStencil;
    riU8 stencil;

    bool    useScissor;
    GLint   scissorX;
    GLint   scissorY;
    GLsizei scissorWidth;
    GLsizei scissorHeight;

} riGL_ClearCommand;

typedef struct riGL_ClearCommandRecord
{
    riGL_CommandHeader header;
    riGL_ClearCommand  clear;

} riGL_ClearCommandRecord;

typedef union riGL_CommandBlockData
{
    riUPtr alignment;
    riU8   bytes[1];

} riGL_CommandBlockData;

typedef struct riGL_CommandBlock
{
    struct riGL_CommandBlock * next;     // Next active or recycled block
    riSize                     used;     // Bytes consumed in data
    riSize                     capacity; // Usable byte capacity in data
    riGL_CommandBlockData      data;     // Aligned command storage

} riGL_CommandBlock;

typedef struct riGL_CommandAllocator
{
    riGL_CommandBlock * firstBlock;   // First active block in execution order
    riGL_CommandBlock * lastBlock;    // Last active block for O(1) appends
    riGL_CommandBlock * currentBlock; // Current write target
    riGL_CommandBlock * freeBlocks;   // Recycled blocks kept for the next reset
    riSize              blockSize;    // Default block allocation size
#        if defined( RHIO_DEBUG )
    riU32  activeBlockCount;
    riU32  freeBlockCount;
    riU32  commandCount;
    riSize bytesUsed;
    riSize peakBytesUsed;
#        endif

} riGL_CommandAllocator;

typedef struct riGL_CommandList riGL_CommandList;

typedef struct riGL_CommandQueue
{
    riCommandQueueBase base;         // Frontend handle dispatch
    riGL_CommandList * commandLists; // Queue-owned command lists

} riGL_CommandQueue;

struct riGL_CommandList
{
    riCommandListBase     base;             // Frontend handle dispatch
    riGL_CommandList *    next;             // Next queue-owned command list
    riCommandQueue        queue;            // Owning queue handle
    riGL_CommandListState state;            // Recording/submission lifecycle
    riGL_CommandAllocator commandAllocator; // Linear command memory
    riRenderPass          activeRenderPass; // Currently open pass, if any
};

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
static void _rhioGL_destroy_texture_noop( riTexture texture );
static void _rhioGL_get_texture_info( riTexture texture, riTextureInfo * outInfo );
static void _rhioGL_destroy_texture_view( riTextureView textureView );
static void _rhioGL_get_texture_view_info( riTextureView textureView, riTextureViewInfo * outInfo );

// Render pass
static riStatus _rhioGL_render_pass_end( riRenderPass renderPass );

// Command memory
static void     _rhioGL_command_allocator_init( riGL_CommandAllocator * allocator );
static void     _rhioGL_command_allocator_destroy( riGL_CommandAllocator * allocator );
static void     _rhioGL_command_allocator_reset( riGL_CommandAllocator * allocator );
static riStatus _rhioGL_command_allocator_write( riGL_CommandAllocator * allocator, riGL_CommandType type,
                                                 riSize recordSize, riGL_CommandHeader ** outHeader );
static void     _rhioGL_command_list_clear_commands( riGL_CommandList * commandList );
static riStatus _rhioGL_append_clear_command( riGL_CommandList * commandList, const riGL_ClearCommand * clear );

// Command execution
static riStatus _rhioGL_execute_command_list( riGL_CommandList * commandList );
static riStatus _rhioGL_execute_clear_command( const riGL_ClearCommand * clear );
static void     _rhioGL_bind_default_framebuffer( void );

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

static const riTextureViewVTable s_gl_texture_view_vtable = {
    BIND_FUNC( destroy_texture_view ),
    BIND_FUNC( get_texture_view_info ),
};

static const riRenderPassVTable s_gl_render_pass_vtable = {
    .end = _rhioGL_render_pass_end,
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

    // Context Diagnostics
    //------------------------------------------------------------------------------
    vendor      = glGetString( GL_VENDOR );
    renderer    = glGetString( GL_RENDERER );
    glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

#        if defined( RHIO_GL_NEEDS_FRAMEBUFFER_BIND_LOADER )
    // Entry Point Loading
    //------------------------------------------------------------------------------
    // Desktop GL headers may expose FBO entry points only through the platform
    // loader. GLES/WebGL and Apple paths bind them directly above
    rhio_glBindFramebufferProc = _RHIO_GL_LOAD( PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer );
    if( RI_UNLIKELY( rhio_glBindFramebufferProc == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "BACKEND GL: Missing required entry point glBindFramebuffer" );
            return RI_ERROR_BACKEND_INIT;
        }
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

    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outSwapchain, RI_ERROR_INVALID_PARAM );

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

    RI_GUARD_NULL( device, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outTextureView, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info->texture, RI_ERROR_INVALID_PARAM );

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
    glCommandList->base.vtable = &s_gl_command_list_vtable;
    glCommandList->queue       = queue;

    // Queue Ownership Link
    //----------------------------------------------------------
    // The queue tracks live lists so queue destruction can clean up command memory
    // even if the frontend destroys the queue before individual lists
    glCommandList->next   = glQueue->commandLists;
    glQueue->commandLists = glCommandList;

    _rhioGL_command_allocator_init( &glCommandList->commandAllocator );

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

    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

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

    RI_GUARD_NULL( queue, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info->commandLists, RI_ERROR_INVALID_PARAM );
    RI_GUARD( info->commandListCount > 0u, RI_ERROR_INVALID_PARAM );

    // Submit Validation
    //----------------------------------------------------------
    // Validate the full batch before executing any command list. This keeps the
    // fallback and batched paths from partially submitting malformed batches
    for( i = 0u; i < info->commandListCount; ++i )
        {
            RI_GUARD_NULL( info->commandLists[i], RI_ERROR_INVALID_PARAM );

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
                    glCommandList->state = RI_GL_COMMAND_LIST_STATE_EXECUTABLE;
                    return status;
                }

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

    RI_GUARD_NULL_VOID( glCommandList );

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

    RI_GUARD_NULL( glCommandList, RI_ERROR_INVALID_PARAM );
    RI_GUARD( glCommandList->state == RI_GL_COMMAND_LIST_STATE_INITIAL, RI_ERROR_INVALID_STATE );

    glCommandList->state = RI_GL_COMMAND_LIST_STATE_RECORDING;

    return RI_SUCCESS;
}

// Finish command recording
static riStatus
_rhioGL_command_list_end( riCommandList commandList )
{
    riGL_CommandList * glCommandList = (riGL_CommandList *)commandList;

    RI_GUARD_NULL( glCommandList, RI_ERROR_INVALID_PARAM );
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

    RI_GUARD_NULL( glCommandList, RI_ERROR_INVALID_PARAM );
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
    riGL_CommandList *                 glCommandList = (riGL_CommandList *)commandList;
    riGL_RenderPass *                  glRenderPass  = NULL;
    const riRenderPassAttachmentInfo * attachment    = NULL;
    riGL_TextureView *                 glTextureView = NULL;
    riGL_ClearCommand                  clear         = RI_ZERO_INIT;
    riU32                              i             = 0;
    riStatus                           status        = RI_ERROR_UNKNOWN;

    RI_GUARD_NULL( glCommandList, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( info, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outRenderPass, RI_ERROR_INVALID_PARAM );

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
            RI_GUARD_NULL( attachment->textureView, RI_ERROR_INVALID_PARAM );

            glTextureView = (riGL_TextureView *)attachment->textureView;
            RI_GUARD( glTextureView->isDefaultFramebuffer, RI_ERROR_BACKEND_UNAVAIL );
            RI_GUARD( RI_FLAG_CHECK( glTextureView->info.usage, RI_TEXTURE_USAGE_RENDER_TARGET ) != 0u,
                      RI_ERROR_INVALID_PARAM );

            if( attachment->loadAction == RI_ATTACHMENT_LOAD_ACTION_CLEAR )
                {
                    clear.hasColor = true;
                    clear.color    = attachment->clearColor;
                    clear.mask |= GL_COLOR_BUFFER_BIT;
                }
        }

    // Depth / Stencil Attachment Load Operation
    //----------------------------------------------------------
    if( info->hasDepthStencilAttachment )
        {
            attachment = &info->depthStencilAttachment;
            RI_GUARD_NULL( attachment->textureView, RI_ERROR_INVALID_PARAM );

            glTextureView = (riGL_TextureView *)attachment->textureView;
            RI_GUARD( glTextureView->isDefaultFramebuffer, RI_ERROR_BACKEND_UNAVAIL );
            RI_GUARD( RI_FLAG_CHECK( glTextureView->info.usage, RI_TEXTURE_USAGE_DEPTH_STENCIL ) != 0u,
                      RI_ERROR_INVALID_PARAM );

            if( attachment->loadAction == RI_ATTACHMENT_LOAD_ACTION_CLEAR )
                {
                    clear.hasDepth   = true;
                    clear.depth      = attachment->clearDepth;
                    clear.hasStencil = true;
                    clear.stencil    = attachment->clearStencil;
                    clear.mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
                }
        }

    // Clear Command Encoding
    //----------------------------------------------------------
    // The render area becomes a temporary GL scissor at execution time. This keeps
    // render-pass clear semantics consistent with explicit APIs
    if( clear.mask != 0u )
        {
            clear.useScissor    = true;
            clear.scissorX      = (GLint)info->renderAreaX;
            clear.scissorY      = (GLint)info->renderAreaY;
            clear.scissorWidth  = (GLsizei)info->renderWidth;
            clear.scissorHeight = (GLsizei)info->renderHeight;

            status              = _rhioGL_append_clear_command( glCommandList, &clear );
            if( RI_FAILED( status ) ) return status;
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

    RI_GUARD_NULL( glSwapchain, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outTexture, RI_ERROR_INVALID_PARAM );
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

    RI_GUARD_NULL( glSwapchain, RI_ERROR_INVALID_PARAM );
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

// End a GL render pass
static riStatus
_rhioGL_render_pass_end( riRenderPass renderPass )
{
    riGL_RenderPass * glRenderPass = (riGL_RenderPass *)renderPass;

    RI_GUARD_NULL( glRenderPass, RI_ERROR_INVALID_PARAM );
    RI_GUARD( !glRenderPass->ended, RI_ERROR_INVALID_STATE );
    RI_GUARD_NULL( glRenderPass->commandList, RI_ERROR_INVALID_STATE );
    RI_GUARD( glRenderPass->commandList->activeRenderPass == renderPass, RI_ERROR_INVALID_STATE );

    glRenderPass->commandList->activeRenderPass = NULL;
    glRenderPass->ended                         = true;
    RI_FREE( glRenderPass );

    return RI_SUCCESS;
}

//----------------------------------------------------------------------------------
// OpenGL Command Memory
//----------------------------------------------------------------------------------

// Align a byte size to a power-of-two boundary
static riSize
_rhioGL_align_up_size( riSize value, riSize alignment )
{
    riSize mask = 0u;

    if( alignment == 0u ) return value;

    // Alignment Validation
    //----------------------------------------------------------
    // A zero return means either "invalid alignment" or "overflow"; callers treat
    // both as allocation/stream construction failure
    mask = alignment - 1u;
    if( ( alignment & mask ) != 0u ) return 0u;
    if( value > ( ~(riSize)0u ) - mask ) return 0u;

    return ( value + mask ) & ~mask;
}

// Allocate one command memory block with aligned storage
static riGL_CommandBlock *
_rhioGL_command_block_allocate( riSize capacity )
{
    riGL_CommandBlock * block          = NULL;
    riSize              metadataSize   = 0u;
    riSize              allocationSize = 0u;
    riSize              maxAllocation  = (riSize)( (size_t)-1 );

    // Capacity Normalization
    //----------------------------------------------------------
    // Command payload starts after flexible metadata, so the storage portion must
    // remain pointer-aligned for every record written into it
    capacity = _rhioGL_align_up_size( capacity, (riSize)sizeof( riUPtr ) );
    if( capacity == 0u ) return NULL;

    // Allocation Size Validation
    //----------------------------------------------------------
    metadataSize = (riSize)sizeof( riGL_CommandBlock ) - (riSize)sizeof( ( (riGL_CommandBlock *)0 )->data );
    if( metadataSize > maxAllocation ) return NULL;
    if( capacity > maxAllocation - metadataSize ) return NULL;

    // Block Allocation
    //----------------------------------------------------------
    allocationSize = metadataSize + capacity;
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

// Move a recycled or newly allocated block into the active command stream
static riStatus
_rhioGL_command_allocator_obtain_block( riGL_CommandAllocator * allocator, riSize minimumCapacity )
{
    riGL_CommandBlock ** cursor   = NULL;
    riGL_CommandBlock *  block    = NULL;
    riSize               capacity = 0u;

    RI_GUARD_NULL( allocator, RI_ERROR_INVALID_PARAM );

    // Required Capacity Selection
    //----------------------------------------------------------
    capacity = allocator->blockSize;
    if( capacity < minimumCapacity )
        {
            capacity = minimumCapacity;
        }

    // Free List Search
    //----------------------------------------------------------
    // Reuse the first free block that can fit the requested command record
    cursor = &allocator->freeBlocks;
    while( *cursor != NULL )
        {
            if( ( *cursor )->capacity >= minimumCapacity )
                {
                    block   = *cursor;
                    *cursor = block->next;
#        if defined( RHIO_DEBUG )
                    if( allocator->freeBlockCount > 0u )
                        {
                            --allocator->freeBlockCount;
                        }
#        endif
                    break;
                }

            cursor = &( *cursor )->next;
        }

    // Fresh Block Allocation
    //----------------------------------------------------------
    if( block == NULL )
        {
            block = _rhioGL_command_block_allocate( capacity );
            if( block == NULL ) return RI_ERROR_OUT_OF_MEMORY;
        }

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
static void
_rhioGL_command_allocator_init( riGL_CommandAllocator * allocator )
{
    riGL_CommandAllocator emptyAllocator = RI_ZERO_INIT;

    if( allocator == NULL ) return;

    *allocator           = emptyAllocator;
    allocator->blockSize = (riSize)RHIO_GL_COMMAND_BLOCK_SIZE;
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
    riGL_CommandBlock * block = NULL;

    if( allocator == NULL ) return;

    // Active Block Recycle
    //----------------------------------------------------------
    // Reset moves blocks to a free list instead of releasing them; command lists
    // usually re-record similar amounts of data frame-to-frame
    block = allocator->firstBlock;
    while( block != NULL )
        {
            block->used = 0u;
            block       = block->next;
        }

    if( allocator->firstBlock != NULL )
        {
            allocator->lastBlock->next = allocator->freeBlocks;
            allocator->freeBlocks      = allocator->firstBlock;
        }

    allocator->firstBlock   = NULL;
    allocator->lastBlock    = NULL;
    allocator->currentBlock = NULL;

#        if defined( RHIO_DEBUG )
    allocator->freeBlockCount += allocator->activeBlockCount;
    allocator->activeBlockCount = 0u;
    allocator->commandCount     = 0u;
    allocator->bytesUsed        = 0u;
#        endif
}

// Reserve one linearly stored command record
static riStatus
_rhioGL_command_allocator_write( riGL_CommandAllocator * allocator, riGL_CommandType type, riSize recordSize,
                                 riGL_CommandHeader ** outHeader )
{
    riGL_CommandBlock *  block             = NULL;
    riGL_CommandHeader * header            = NULL;
    riSize               alignedRecordSize = 0u;
    riStatus             status            = RI_ERROR_UNKNOWN;

    RI_GUARD_NULL( allocator, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( outHeader, RI_ERROR_INVALID_PARAM );
    RI_GUARD( recordSize >= (riSize)sizeof( riGL_CommandHeader ), RI_ERROR_INVALID_PARAM );

    // Record Size Normalization
    //----------------------------------------------------------
    // Records are size-prefixed and pointer-aligned so decoding can skip unknown
    // payloads safely once new command types are added
    *outHeader        = NULL;
    alignedRecordSize = _rhioGL_align_up_size( recordSize, (riSize)sizeof( riUPtr ) );
    RI_GUARD( alignedRecordSize >= recordSize, RI_ERROR_OUT_OF_MEMORY );
    RI_GUARD( alignedRecordSize <= (riSize)0xFFFFFFFFu, RI_ERROR_OUT_OF_MEMORY );

    // Block Selection
    //----------------------------------------------------------
    // The subtraction form avoids overflow when checking remaining block capacity
    block = allocator->currentBlock;
    if( block == NULL || block->used > block->capacity || block->capacity - block->used < alignedRecordSize )
        {
            status = _rhioGL_command_allocator_obtain_block( allocator, alignedRecordSize );
            if( RI_FAILED( status ) ) return status;

            block = allocator->currentBlock;
        }

    // Record Header Encoding
    //----------------------------------------------------------
    header = (riGL_CommandHeader *)(void *)&block->data.bytes[block->used];
    block->used += alignedRecordSize;

    header->type = (riU32)type;
    header->size = (riU32)alignedRecordSize;

#        if defined( RHIO_DEBUG )
    ++allocator->commandCount;
    allocator->bytesUsed += alignedRecordSize;
    if( allocator->peakBytesUsed < allocator->bytesUsed )
        {
            allocator->peakBytesUsed = allocator->bytesUsed;
        }
#        endif

    *outHeader = header;

    return RI_SUCCESS;
}

// Release all commands recorded in a GL command list
static void
_rhioGL_command_list_clear_commands( riGL_CommandList * commandList )
{
    if( NULL == commandList ) return;

    _rhioGL_command_allocator_reset( &commandList->commandAllocator );
}

// Append a framebuffer clear command to a GL command list
static riStatus
_rhioGL_append_clear_command( riGL_CommandList * commandList, const riGL_ClearCommand * clear )
{
    riGL_CommandHeader *      header = NULL;
    riGL_ClearCommandRecord * record = NULL;
    riStatus                  status = RI_ERROR_UNKNOWN;

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );
    RI_GUARD_NULL( clear, RI_ERROR_INVALID_PARAM );

    // Command Record Allocation
    //----------------------------------------------------------
    status = _rhioGL_command_allocator_write(
        &commandList->commandAllocator, RI_GL_COMMAND_CLEAR_FRAMEBUFFER, (riSize)sizeof( *record ), &header );
    if( RI_FAILED( status ) ) return status;

    // Command Payload Encoding
    //----------------------------------------------------------
    record        = (riGL_ClearCommandRecord *)(void *)header;
    record->clear = *clear;

    return RI_SUCCESS;
}

//----------------------------------------------------------------------------------
// OpenGL Command Execution
//----------------------------------------------------------------------------------

// Execute all commands recorded in a GL command list
static riStatus
_rhioGL_execute_command_list( riGL_CommandList * commandList )
{
    const riGL_CommandBlock *       block        = NULL;
    const riGL_CommandHeader *      header       = NULL;
    const riGL_ClearCommandRecord * clear        = NULL;
    riSize                          offset       = 0u;
    riSize                          commandSize  = 0u;
    riSize                          expectedSize = 0u;
    riStatus                        status       = RI_ERROR_UNKNOWN;

    RI_GUARD_NULL( commandList, RI_ERROR_INVALID_PARAM );

    // Command Stream Decode
    //----------------------------------------------------------
    // Every record begins with riGL_CommandHeader. The stored size is trusted only
    // after checking it stays inside the current memory block
    block = commandList->commandAllocator.firstBlock;
    while( block != NULL )
        {
            offset = 0u;
            while( offset < block->used )
                {
                    RI_GUARD( block->used - offset >= (riSize)sizeof( riGL_CommandHeader ), RI_ERROR_INVALID_STATE );

                    header      = (const riGL_CommandHeader *)(const void *)&block->data.bytes[offset];
                    commandSize = (riSize)header->size;

                    RI_GUARD( commandSize >= (riSize)sizeof( riGL_CommandHeader ), RI_ERROR_INVALID_STATE );
                    RI_GUARD( commandSize <= block->used - offset, RI_ERROR_INVALID_STATE );

                    switch( (riGL_CommandType)header->type )
                        {
                        case RI_GL_COMMAND_CLEAR_FRAMEBUFFER:
                            // Clear Payload Validation
                            //----------------------------------------------------------
                            expectedSize = _rhioGL_align_up_size( (riSize)sizeof( *clear ), (riSize)sizeof( riUPtr ) );
                            RI_GUARD( expectedSize != 0u, RI_ERROR_INVALID_STATE );
                            RI_GUARD( commandSize >= expectedSize, RI_ERROR_INVALID_STATE );

                            clear  = (const riGL_ClearCommandRecord *)(const void *)header;
                            status = _rhioGL_execute_clear_command( &clear->clear );
                            if( RI_FAILED( status ) ) return status;
                            break;

                        default: return RI_ERROR_INVALID_STATE;
                        }

                    offset += commandSize;
                }

            block = block->next;
        }

    return RI_SUCCESS;
}

// Execute one framebuffer clear command
static riStatus
_rhioGL_execute_clear_command( const riGL_ClearCommand * clear )
{
    GLboolean restoreScissorEnabled = GL_FALSE;
    GLint     restoreScissorBox[4]  = { 0, 0, 0, 0 };

    GLboolean restoreColorMask[4]   = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    GLboolean restoreDepthMask      = GL_TRUE;
    GLint     restoreStencilMask    = 0;

    if( NULL == clear || clear->mask == 0u ) return RI_SUCCESS;

    // Framebuffer Binding
    //----------------------------------------------------------
    _rhioGL_bind_default_framebuffer();

    // Scissor State Setup
    //----------------------------------------------------------
    // Render-pass clear rectangles are implemented with GL scissor. Existing
    // scissor state is restored after glClear so callers keep ownership of state
    if( clear->useScissor )
        {
            restoreScissorEnabled = glIsEnabled( GL_SCISSOR_TEST );
            glGetIntegerv( GL_SCISSOR_BOX, restoreScissorBox );

            glEnable( GL_SCISSOR_TEST );
            glScissor( clear->scissorX, clear->scissorY, clear->scissorWidth, clear->scissorHeight );
        }

    // Clear Write-Mask Setup
    //----------------------------------------------------------
    // glClear obeys color/depth/stencil write masks. Temporarily force enabled
    // masks for attachments requested by the render pass, then restore below
    if( clear->hasColor )
        {
            glGetBooleanv( GL_COLOR_WRITEMASK, restoreColorMask );
            glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
            glClearColor( clear->color.r, clear->color.g, clear->color.b, clear->color.a );
        }

    if( clear->hasDepth )
        {
            glGetBooleanv( GL_DEPTH_WRITEMASK, &restoreDepthMask );
            glDepthMask( GL_TRUE );
            glClearDepth( clear->depth );
        }

    if( clear->hasStencil )
        {
            glGetIntegerv( GL_STENCIL_WRITEMASK, &restoreStencilMask );
            glStencilMask( 0xFFFFFFFFu );
            glClearStencil( (GLint)clear->stencil );
        }

    // Clear Execution
    //----------------------------------------------------------
    glClear( clear->mask );

    // Clear State Restore
    //----------------------------------------------------------
    if( clear->hasStencil )
        {
            glStencilMask( (GLuint)restoreStencilMask );
        }

    if( clear->hasDepth )
        {
            glDepthMask( restoreDepthMask );
        }

    if( clear->hasColor )
        {
            glColorMask( restoreColorMask[0], restoreColorMask[1], restoreColorMask[2], restoreColorMask[3] );
        }

    if( clear->useScissor )
        {
            glScissor( restoreScissorBox[0], restoreScissorBox[1], restoreScissorBox[2], restoreScissorBox[3] );

            if( restoreScissorEnabled )
                {
                    glEnable( GL_SCISSOR_TEST );
                }
            else
                {
                    glDisable( GL_SCISSOR_TEST );
                }
        }

    return RI_SUCCESS;
}

// Bind the OpenGL default framebuffer
static void
_rhioGL_bind_default_framebuffer( void )
{
#        if defined( RHIO_GL_NEEDS_FRAMEBUFFER_BIND_LOADER )
    // Desktop GL loader path
    //----------------------------------------------------------
    if( rhio_glBindFramebufferProc != NULL )
        {
            rhio_glBindFramebufferProc( GL_FRAMEBUFFER, 0u );
        }
#        else
    // Direct GL/GLES path
    //----------------------------------------------------------
    glBindFramebuffer( GL_FRAMEBUFFER, 0u );
#        endif
}

// Populate the backend descriptor with OpenGL implementations
static riStatus
_rhioGL_registerBackend( riBackendDesc * desc )
{
    riBackend backend = RI_BACKEND_OPENGL;
    riFlags   flags   = 0;

    RI_GUARD_NULL( desc, RI_ERROR_INVALID_PARAM );

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
                        return RI_ERROR_INVALID_STATE;                                                                 \
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

    // Submit Dispatch Validation
    //----------------------------------------------------------
    // NOTE: The batched slot is optional; minimal/custom backends may expose only
    // single-list submit and let the frontend fan out rhioCommandQueueSubmitInfo()
    if( RI_UNLIKELY( vtable->submit_command_list == NULL && vtable->submit_command_lists == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "COMMAND_QUEUE: Missing required submit vtable slot" );
            return RI_ERROR_INVALID_STATE;
        }

#    undef CHECK_QUEUE_VTABLE_SLOT

    return RI_SUCCESS;
}

// Ensure a command list exposes callbacks required by the public command-list API
static riStatus
_rhioValidateCommandListVTable( const riCommandListVTable * vtable )
{
    if( RI_UNLIKELY( vtable == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "COMMAND_LIST: Missing dispatch table" );
            return RI_ERROR_INVALID_STATE;
        }

#    define CHECK_LIST_VTABLE_SLOT( name )                                                                             \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( vtable->name == NULL ) )                                                              \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "COMMAND_LIST: Missing required vtable slot '%s'", #name );            \
                        return RI_ERROR_INVALID_STATE;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_LIST_VTABLE_SLOT( destroy_command_list );
    CHECK_LIST_VTABLE_SLOT( begin );
    CHECK_LIST_VTABLE_SLOT( end );
    CHECK_LIST_VTABLE_SLOT( reset );
    CHECK_LIST_VTABLE_SLOT( begin_render_pass );

#    undef CHECK_LIST_VTABLE_SLOT

    return RI_SUCCESS;
}

// Ensure a swapchain exposes callbacks required by the public swapchain API
static riStatus
_rhioValidateSwapchainVTable( const riSwapchainVTable * vtable )
{
    if( RI_UNLIKELY( vtable == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "SWAPCHAIN: Missing dispatch table" );
            return RI_ERROR_INVALID_STATE;
        }

#    define CHECK_SWAPCHAIN_VTABLE_SLOT( name )                                                                        \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( vtable->name == NULL ) )                                                              \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "SWAPCHAIN: Missing required vtable slot '%s'", #name );               \
                        return RI_ERROR_INVALID_STATE;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_SWAPCHAIN_VTABLE_SLOT( destroy_swapchain );
    CHECK_SWAPCHAIN_VTABLE_SLOT( get_current_texture );
    CHECK_SWAPCHAIN_VTABLE_SLOT( present );

#    undef CHECK_SWAPCHAIN_VTABLE_SLOT

    return RI_SUCCESS;
}

// Ensure a texture view exposes callbacks required by the public texture-view API
static riStatus
_rhioValidateTextureViewVTable( const riTextureViewVTable * vtable )
{
    if( RI_UNLIKELY( vtable == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "TEXTURE_VIEW: Missing dispatch table" );
            return RI_ERROR_INVALID_STATE;
        }

#    define CHECK_TEXTURE_VIEW_VTABLE_SLOT( name )                                                                     \
        do                                                                                                             \
            {                                                                                                          \
                if( RI_UNLIKELY( vtable->name == NULL ) )                                                              \
                    {                                                                                                  \
                        TRACELOG( RI_LOG_ERROR, "TEXTURE_VIEW: Missing required vtable slot '%s'", #name );            \
                        return RI_ERROR_INVALID_STATE;                                                                 \
                    }                                                                                                  \
            }                                                                                                          \
        while( 0 )

    CHECK_TEXTURE_VIEW_VTABLE_SLOT( destroy_texture_view );
    CHECK_TEXTURE_VIEW_VTABLE_SLOT( get_texture_view_info );

#    undef CHECK_TEXTURE_VIEW_VTABLE_SLOT

    return RI_SUCCESS;
}

// Ensure a render pass exposes callbacks required by the public render-pass API
static riStatus
_rhioValidateRenderPassVTable( const riRenderPassVTable * vtable )
{
    if( RI_UNLIKELY( vtable == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "RENDER_PASS: Missing dispatch table" );
            return RI_ERROR_INVALID_STATE;
        }

    if( RI_UNLIKELY( vtable->end == NULL ) )
        {
            TRACELOG( RI_LOG_ERROR, "RENDER_PASS: Missing required vtable slot 'end'" );
            return RI_ERROR_INVALID_STATE;
        }

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
