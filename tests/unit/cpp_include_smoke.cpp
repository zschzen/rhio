#include "rhio.h"

static_assert( RHIO_VERSION == ( ( RHIO_VERSION_MAJOR << 16 ) | ( RHIO_VERSION_MINOR << 8 ) | RHIO_VERSION_PATCH ),
               "RHIO_VERSION must match major/minor/patch encoding" );

int
main()
{
    riDevice device = nullptr;
    (void)device;

    return RI_SUCCEEDED( RI_SUCCESS ) ? 0 : 1;
}
