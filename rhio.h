/**********************************************************************************************
*
*   rhio v0.1.0 - Single-file header-only C99 RHI
*
*   DESCRIPTION:
*       A tiny, single-header rendering layer for C that abstracts
*       OpenGL 3.3, ES 2.0/3.0, and Vulkan 1.2 behind one straightforward API.
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

#include <stdarg.h> /* va_list, va_start, va_end */
#include <stdint.h> /* uint8_t, uint64_t, int32_t */
#include <stdlib.h> /* malloc, calloc, realloc, free */

//----------------------------------------------------------------------------------
// Module Defines and Macros
//----------------------------------------------------------------------------------

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
#    include <assert.h>
#    define RHIO_ASSERT( x ) assert( x )
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
#    define TRACELOG( l, ... ) TraceLog( l, __VA_ARGS__ )
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

/*
 * Guard Clause
 *
 * Use these at the top of every public function to validate non-nullable parameters.
 * They return immediately on failure.
 *
 *   RI_GUARD_NULL( ptr, retval )    - return retval if ptr is NULL
 *   RI_GUARD( cond, retval )        - return retval if condition is false
 */
#define RI_GUARD_NULL( ptr, retval )                                                                                   \
    do                                                                                                                 \
        {                                                                                                              \
            if( RI_UNLIKELY( ( ptr ) == NULL ) )                                                                       \
                {                                                                                                      \
                    TRACELOG( RI_LOG_ERROR, "%s: unexpected NULL argument", __func__ );                                \
                    return ( retval );                                                                                 \
                }                                                                                                      \
        }                                                                                                              \
    while( 0 )

#define RI_GUARD( cond, retval )                                                                                       \
    do                                                                                                                 \
        {                                                                                                              \
            if( RI_UNLIKELY( !( cond ) ) )                                                                             \
                {                                                                                                      \
                    TRACELOG( RI_LOG_ERROR, "%s: guard failed: " #cond, __func__ );                                    \
                    return ( retval );                                                                                 \
                }                                                                                                      \
        }                                                                                                              \
    while( 0 )

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

//----------------------------------------------------------------------------------
// Enumerations
//----------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------
// Callbacks
//----------------------------------------------------------------------------------

// Logging Callback Signature
typedef void ( *TraceLogCallback )( int logType, const char * text, va_list args );

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

// ...

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------

// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

RI_API bool rhioInit( const riInitInfo * info );
RI_API void rhioShutdown( void );

// Logging Configuration API
RI_API void SetTraceLogLevel( int logType );
RI_API void SetTraceLogCallback( TraceLogCallback callback );
RI_API void TraceLog( int logType, const char * text, ... );

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
#ifdef RHIO_IMPLEMENTATION

#    include <stdio.h>

#    if defined( PLATFORM_ANDROID )
#        include <android/log.h>
#    endif

//----------------------------------------------------------------------------------
// Module Internal State
//----------------------------------------------------------------------------------
static int              rhio_logTypeLevel = RI_LOG_INFO;
static TraceLogCallback rhio_traceLog     = NULL;

//----------------------------------------------------------------------------------
// Module Functions Definition: Window and Graphics Device
//----------------------------------------------------------------------------------

RI_API bool
rhioInit( const riInitInfo * info )
{
    UNUSED( info );

    TRACELOG( RI_LOG_INFO, "Initialized" );

    return true;
}

RI_API void
rhioShutdown( void )
{
}

//----------------------------------------------------------------------------------
// Module Functions Definition: Logging
//----------------------------------------------------------------------------------

// Set the minimum log level
RI_API void
SetTraceLogLevel( int logType )
{
    rhio_logTypeLevel = logType;
}

// Set a custom trace log
RI_API void
SetTraceLogCallback( TraceLogCallback callback )
{
    rhio_traceLog = callback;
}

// Emit trace log messages
RI_API void
TraceLog( int logType, const char * text, ... )
{
#    if _RHIO_LOG_SUPPORT
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
    const char * prefix = "";
    switch( logType )
        {
        case RI_LOG_TRACE:   prefix = "TRACE: "; break;
        case RI_LOG_DEBUG:   prefix = "DEBUG: "; break;
        case RI_LOG_INFO:    prefix = "INFO: "; break;
        case RI_LOG_WARNING: prefix = "WARNING: "; break;
        case RI_LOG_ERROR:   prefix = "ERROR: "; break;
        case RI_LOG_FATAL:   prefix = "FATAL: "; break;
        }

    FILE * stream = ( RI_LOG_WARNING <= logType ) ? stderr : stdout;
    fprintf( stream, "[rhio] %s", prefix );
    vfprintf( stream, text, args );
    fprintf( stream, "\n" );
    fflush( stream );
#        endif

    va_end( args );

    if( RI_LOG_FATAL == logType ) exit( EXIT_FAILURE );
#    endif
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// ...

#endif // RHIO_IMPLEMENTATION

#endif // RHIO_H
