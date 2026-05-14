// ============================================================================
// rhio - status and flag unit tests
// ============================================================================
// Validates small public helper macros that many later tests and user call
// sites rely on for result classification and bit-flag manipulation
// ============================================================================

#include "rhio.h"

#include <rktest/rktest.h>

//----------------------------------------------------------------------------------
// Result Macro Tests
//----------------------------------------------------------------------------------

// Confirms success/error status macros agree on core public result values
TEST( status, result_macros_classify_errors )
{
    EXPECT_TRUE( RI_SUCCEEDED( RI_SUCCESS ) );
    EXPECT_FALSE( RI_FAILED( RI_SUCCESS ) );

    EXPECT_FALSE( RI_SUCCEEDED( RI_ERROR_INVALID_PARAM ) );
    EXPECT_TRUE( RI_FAILED( RI_ERROR_INVALID_PARAM ) );
}

//----------------------------------------------------------------------------------
// Flag Helper Tests
//----------------------------------------------------------------------------------

// Confirms flag helpers modify one bit without disturbing unrelated bits
TEST( status, flag_helpers_preserve_other_bits )
{
    const riFlags otherBit = (riFlags)( 1u << 8 );
    riFlags       flags    = otherBit;

    // NOTE: otherBit catches helpers that accidentally overwrite the mask
    RI_FLAG_SET( flags, RI_DEVICE_FLAG_DEBUG );
    EXPECT_NE( RI_FLAG_CHECK( flags, RI_DEVICE_FLAG_DEBUG ), 0u );
    EXPECT_NE( RI_FLAG_CHECK( flags, otherBit ), 0u );

    RI_FLAG_TOGGLE( flags, RI_DEVICE_FLAG_DEBUG );
    EXPECT_EQ( RI_FLAG_CHECK( flags, RI_DEVICE_FLAG_DEBUG ), 0u );
    EXPECT_NE( RI_FLAG_CHECK( flags, otherBit ), 0u );

    RI_FLAG_SET( flags, RI_DEVICE_FLAG_DEBUG );
    RI_FLAG_CLEAR( flags, RI_DEVICE_FLAG_DEBUG );
    EXPECT_EQ( flags, otherBit );
}
