#include <stdio.h>
#include <string.h>
#include "SHTestWorker.h"
#include "ExampleClassesAddOnFunctions.h"

#define SHTW_NAME_NAME "TestName"

SHTestWorker::
SHTestWorker(const char * name) 
{
    _name = new char[strlen(name)+1];
    strcpy(_name, name);  
}

SHTestWorker::
SHTestWorker(BMessage * arch)
  : SHWorker(arch)
{
   const char * str = arch->FindString(SHTW_NAME_NAME);
   if (str == NULL)
   {
       printf("SHTestWorker:  couldn't find my name!\n");
       str = "<unknown>";
   }
   _name = new char[strlen(str)+1];
   strcpy(_name, str);
   
   printf("SHTestWorker [%s]:  I have been rehydrated!\n", GetName());
}

status_t
SHTestWorker::GetAddOnSpec(SHFileSpec & spec) const
{
   status_t ret;
   if ((ret = SHWorker::GetAddOnSpec(spec)) != B_NO_ERROR) return ret;
   if ((ret = GetExampleClassesFileSpec(spec)) != B_NO_ERROR) return ret;
   
   return B_NO_ERROR;
}

SHTestWorker::
~SHTestWorker()
{
   Stop();   // kill internal BLooper -- necessary to 
             // avoid potential race conditions.
             // For example, what if the BLooper called 
             // GetName() after we delete _name, below?  Bad...

   printf("~SHTestWorker:  %s is going bye bye\n", _name);

   delete [] _name;
}

const char *
SHTestWorker::
GetName() const
{
   return _name;
}

BArchivable *
SHTestWorker::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "SHTestWorker")) return NULL;
   return new SHTestWorker(archive);
}

status_t
SHTestWorker::
Archive(BMessage * archive, bool deep) const
{
   // It's VERY important to pass the Archive() call
   // up to our parent class!
   status_t ret = SHWorker::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
 
   // Add to (archive) all data necessary to recover our state
   // for this simple case, the only state info is our name!
   archive->AddString(SHTW_NAME_NAME, _name);
   return B_NO_ERROR; 
}

void
SHTestWorker::
MessageReceived(BMessage * msg)
{
   printf("SHTestWorker [%s]:  message received:\n", GetName());
   msg->PrintToStream();
}
