#ifndef _SequenceReceiver_H_
#define _SequenceReceiver_H_

#include <sockhop/SHWorker.h>

#define SHSR_NAME_COUNT "count"

#ifdef __INTEL__
_EXPORT class SequenceReceiver;
#else
#pragma export on
#endif

// Just prints out debug output...
class SequenceReceiver : public SHWorker 
{
public:
   SequenceReceiver(const char * workerName);
   SequenceReceiver(BMessage * archive); // for BArchivable compatibility
   ~SequenceReceiver();
   
   virtual const char * GetName() const;
   //Returns the name specified in the constructor.
   
   virtual status_t GetAddOnSpec(SHFileSpec & spec) const;
   
   // BArchivable interface
   static BArchivable * Instantiate(BMessage * archive);
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.
   
   // BLooper interface -- just prints messages to stdout
   virtual void MessageReceived(BMessage * msg);
   
   // If you want to just change the name of this SequenceReceiver...
   void SetName(const char * newName);
   
private:
   char * _name;
   int _count;
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif
