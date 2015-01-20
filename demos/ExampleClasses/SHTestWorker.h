#ifndef _SHTESTWORKER_H_
#define _SHTESTWORKER_H_

#include <sockhop/SHWorker.h>

#ifdef __INTEL__
_EXPORT class SHTestWorker;
#else
#pragma export on
#endif

// This is a very simple SHWorker subclass.  All it does it print
// out text to stdout whenever it receives a BMessage.
class SHTestWorker : public SHWorker 
{
public:
   SHTestWorker(const char * workerName);
   SHTestWorker(BMessage * archive); // for BArchivable compatibility
   ~SHTestWorker();
   
   virtual const char * GetName() const;
   //Returns the name specified in the constructor.
   
   virtual status_t GetAddOnSpec(SHFileSpec & spec) const;
   
   // BArchivable interface
   static BArchivable * Instantiate(BMessage * archive);
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.
   
   // BLooper interface -- just prints messages to stdout
   virtual void MessageReceived(BMessage * msg);
   
private:
   char * _name;
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif
