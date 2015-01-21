// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _FLAGWAVINGWORKER_H_
#define _FLAGWAVINGWORKER_H_

// This file implements a SockHop add on thread (SHWorker subclass)
// that opens a window at a random position on the screen, and
// draws lots of animated boxes into the window.  It also sends
// a BMessages every second to the root node, saying how many
// lines it has drawn in the past second.

#include <sockhop/SockHop.h>
#include "FlagWavingWindow.h"

#ifdef __INTEL__
_EXPORT class FlagWavingWorker;
#else
#pragma export on
#endif

class FlagWavingWorker : public SHWorker
{
public:
   FlagWavingWorker(const char * name, int width, int height);
   FlagWavingWorker(BMessage * archive);
   ~FlagWavingWorker();
   
   static BArchivable * Instantiate(BMessage * archive);
   
   virtual status_t GetAddOnSpec(SHFileSpec & spec) const;
   
   virtual const char * GetName() const;
   
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   
   virtual void MessageReceived(BMessage * msg);
   
   virtual bool IsInterestedIn(BMessage * msg);

private:   
   char * _name;
   FlagWavingWindow * _window;
   int32 _width;
   int32 _height;
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif
