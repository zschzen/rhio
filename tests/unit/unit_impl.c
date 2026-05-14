//----------------------------------------------------------------------------------
// rhio Implementation Translation Unit
//----------------------------------------------------------------------------------
// Single-header libraries need exactly one C translation unit to define the
// implementation before including the public header
//
// NOTE: The other unit-test *.c files include `rhio.h` declaration-only.
// This file owns the implementation so the test binary does not
// violate one-definition rules.

#define RHIO_IMPLEMENTATION
#include "rhio.h"
