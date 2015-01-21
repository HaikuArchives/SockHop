#include "ExampleClassesAddOnFunctions.h"
#include <sockhop/SHFileSpec.h>

status_t GetExampleClassesFileSpec(SHFileSpec & spec)
{
   status_t ret;
   
   if ((ret = spec.AddFlavor(SHFlavor("add-ons/x86/ExampleClassesAddOn", SH_ARCH_BEOS_X86, true))) != B_NO_ERROR) return ret;
   if ((ret = spec.AddFlavor(SHFlavor("add-ons/ppc/ExampleClassesAddOn", SH_ARCH_BEOS_PPC, true))) != B_NO_ERROR) return ret;
   return ret;
}