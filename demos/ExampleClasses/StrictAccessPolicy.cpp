#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "ExampleClassesAddOnFunctions.h"
#include "StrictAccessPolicy.h"
 
StrictAccessPolicy ::
StrictAccessPolicy(const char * sub, int port)
   : _port(port), _sub(strdup(sub))
{ 
   // empty
}
   
StrictAccessPolicy ::
StrictAccessPolicy(BMessage * archive)
   : SHAccessPolicy(archive), _port(SH_DEFAULT_PORT)
{ 
   const char * temp;
   if (archive->FindString("sub", &temp) == B_NO_ERROR) _sub = strdup(temp);
   else 
   {
      printf("StrictAccessPolicy:  Couldn't find sub field in rehydration message?\n");
      _sub = strdup("<error>");
   }
}
   
StrictAccessPolicy ::
~StrictAccessPolicy()
{ 
   if (_sub) ::free(_sub);
}
   
SHNodeSpec
StrictAccessPolicy ::
GetListeningLocation()
{ 
   // Note that the hostname and nodename fields aren't used here
   return(SHNodeSpec("", "", _port));
}
   
bool 
StrictAccessPolicy ::
OkayToAcceptConnection(SHNodeSpec & connectingSpec)
{ 
   // Only accept the connection if the hostname contains this string...
   // A real implementation might compare against a list of "approved" ip names,
   // or use wildcarding, or something like that.
   return(strstr(connectingSpec.GetHostName(), _sub) != 0);
}
   
bool    
StrictAccessPolicy ::
OkayToWriteFile(const char * fileName)
{ 
   return (fileName[0] != '/'); 
}
   
bool 
StrictAccessPolicy ::
OkayToReadFile(const char * fileName)
{ 
   return (fileName[0] != '/');
}

bool
StrictAccessPolicy ::
OkayToInstantiateObject(const BMessage &)
{
   return false;  // No user add-ons allowed!!!
}

// SHComponent interface
const char *
StrictAccessPolicy ::
GetName() const
{
   return "strict";
}

status_t
StrictAccessPolicy::GetAddOnSpec(SHFileSpec & spec) const
{
   status_t ret;
   if ((ret = SHAccessPolicy::GetAddOnSpec(spec)) != B_NO_ERROR) return ret;
   if ((ret = GetExampleClassesFileSpec(spec)) != B_NO_ERROR) return ret;
   
   return B_NO_ERROR;
}

     
// BArchivable interface   
BArchivable *
StrictAccessPolicy ::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "StrictAccessPolicy")) return NULL;
   return new StrictAccessPolicy(archive);
}

   
status_t   
StrictAccessPolicy ::
Archive(BMessage * archive, bool deep) const
{
   status_t ret = SHComponent::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;

   if (_sub) archive->AddString("sub", _sub);
      
   return ret;
}

