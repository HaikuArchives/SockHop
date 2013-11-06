#include <stdio.h>
#include "SHTestWorker.h"

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
}

status_t
SHTestWorker::GetAddOnSpec(SHFileSpec & spec) const
{
   spec.AddFlavor(SHFlavor("add-ons/x86/SHTestWorker", SH_ARCH_BEOS_X86, true));
   spec.AddFlavor(SHFlavor("add-ons/ppc/SHTestWorker", SH_ARCH_BEOS_PPC, true));
   return B_NO_ERROR;
}

SHTestWorker::
~SHTestWorker()
{
printf("~SHTestWorker:  %s is going bye bye\n",_name);
   Stop();   // necessary to avoid race conditions in IsInterestedIn()
   delete []_name;
}

const char *
SHTestWorker::
GetName() const
{
   return _name;
}

bool
SHTestWorker::
IsInterestedIn(BMessage *)
{
   // Note that this method is called by a different thread than
   // this BLooper's thread.   So you may need to synchronize access
   // to any data that is used by both this method and MessageReceived()
   return true;
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
   status_t ret = SHWorker::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
 
   archive->AddString(SHTW_NAME_NAME, _name);
   return B_NO_ERROR; 
}

void
SHTestWorker::
MessageReceived(BMessage * msg)
{
   printf("SHTestWorker [%s]:  message received:\n", _name);
   msg->PrintToStream();
}

void
SHTestWorker::
SetName(const char * newName)
{
   delete[] _name;
   _name = new char[strlen(newName)+1];
   strcpy(_name, newName);
}