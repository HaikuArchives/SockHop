#ifndef ExampleClassesAddOnFunctions_h
#define ExampleClassesAddOnFunctions_h

#include <sockhop/SHFileSpec.h>

#ifndef __INTEL__
#pragma export on
#endif

// Adds the files needed for this example add-on to (spec).
_EXPORT status_t GetExampleClassesFileSpec(SHFileSpec & spec);

#ifndef __INTEL__
#pragma export reset
#endif

#endif