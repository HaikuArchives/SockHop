// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
// This file implements a SockHop add on thread (SHWorker subclass)
// that opens a window at a random position on the screen, and
// draws lots of random lines into the window.  It also sends
// a BMessages every second to the root node, saying how many
// lines it has drawn in the past second.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <interface/Screen.h>
#include "EllipseDrawingWorker.h"
#include "../SockDemo.h"  // Should be changed to correct directory!!

#define LDW_COMMAND_STARTUP 'ldws'

#define LDW_NAME_NAME  "ldwName"
#define LDW_NAME_FRAME "ldwFrame"


EllipseDrawingWorker :: 
EllipseDrawingWorker(const char * name)
  : _name(new char[strlen(name)+1]), _window(NULL)
{
   strcpy(_name, name);

   // Tell myself to open the window when the thread starts running
   BMessage startup(LDW_COMMAND_STARTUP);
   GetMessenger().SendMessage(&startup);
}

EllipseDrawingWorker ::
EllipseDrawingWorker(BMessage * archive)
   : SHWorker(archive), _name(NULL), _window(NULL)
{
   // get our window name/worker name from the archive
   const char * temp;   
   char temp2[100];
   if (archive->FindString(LDW_NAME_NAME, &temp) != B_NO_ERROR)
   {
      // Pick a random name...
      sprintf(temp2, "EllipseDrawingWorker %Li",system_time());
      temp = temp2;      
   }
   _name = new char[strlen(temp)+1];
   strcpy(_name, temp);
   
   // Tell myself to open the window when the thread starts running
   BMessage startup(LDW_COMMAND_STARTUP);
   GetMessenger().SendMessage(&startup);
}

EllipseDrawingWorker ::
~EllipseDrawingWorker()
{ 
   Stop();  // SockHop requires this to be first in SHWorker destructors

   // This will tell the SockDemo GUI we're gone.
   if (GetNodePath())
   {
      BMessage dead(SD_COMMAND_WORKER_QUITTING);
      dead.AddString(SH_NAME_TO, "/..");
      dead.AddString(SH_NAME_FROM, GetNodePath());
      dead.AddString(SD_NAME_WORKERNAME, GetName());
      (void)GetNodeMessenger().SendMessage(&dead);
   }

   if ((_window)&&(_window->Lock())) _window->Quit();

   // clean up   
   delete [] _name;
}

BArchivable *
EllipseDrawingWorker ::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "EllipseDrawingWorker")) return NULL;
   return new EllipseDrawingWorker(archive);
}

status_t
EllipseDrawingWorker ::
GetAddOnSpec(SHFileSpec & spec) const
{
    status_t ret;
    
    if ((ret = SHWorker::GetAddOnSpec(spec)) != B_NO_ERROR) return ret;    
    if ((ret = spec.AddFlavor(SHFlavor("add-ons/x86/EllipseDrawingWorker", SH_ARCH_BEOS_X86, true))) != B_NO_ERROR) 
    {
       printf("EllipseDrawingWorker::GetAddOnSpec:  Error adding intel EllipseDrawingWorker flavor 'add-ons/x86/EllipseDrawingWorker'\n");
       return ret;
    }
    if ((ret = spec.AddFlavor(SHFlavor("add-ons/ppc/EllipseDrawingWorker", SH_ARCH_BEOS_PPC, true))) != B_NO_ERROR) 
    {
       printf("EllipseDrawingWorker::GetAddOnSpec:  Error adding powerpc EllipseDrawingWorker flavor 'add-ons/ppc/EllipseDrawingWorker'\n");
       return ret;
    }
    return B_NO_ERROR;
}

const char * 
EllipseDrawingWorker ::
GetName() const
{
   return _name;
}

status_t
EllipseDrawingWorker ::
Archive(BMessage * archive, bool deep) const
{
   status_t ret = SHWorker::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
   
   archive->AddString(LDW_NAME_NAME, _name);
   return B_NO_ERROR;
}

void
EllipseDrawingWorker ::
MessageReceived(BMessage * msg)
{
   switch(msg->what)
   {
      case LDW_COMMAND_STARTUP:
         if (_window == NULL)
         {
            const int windowWidth = 300;
            const int windowHeight = 300;
            
            BRect frame;
            {
               BScreen screen;
               frame = screen.Frame();
            }
            
            // set our bounds to be screenSize - windowSize
            int width = (int)frame.Width() - windowWidth;
            int height = (int)frame.Height() - windowHeight;
            if (width < 0) width = 0;
            if (height < 0) height = 0;
            
            // pick a random spot on the screen...
            frame.left += rand() % width;
            frame.top += rand() % height;
            frame.right = frame.left + windowWidth;
            frame.bottom = frame.top + windowHeight;
                        
            _window = new EllipseDrawingWindow(frame, _name, GetMessenger());
            _window->Show();
         }
      break;
      
      case SD_COMMAND_WORKER_STATS:
         // relay stats info back to the SockDemo GUI
         msg->AddString(SH_NAME_TO, "/..");
         GetNodeMessenger().SendMessage(msg);
      break;

      case SD_COMMAND_WINDOW_TO_FRONT:
         if (_window->Lock())
         {
            _window->Activate();
            _window->Unlock(); 
         }         
      break;
            
      case LDW_COMMAND_CLOSING:
         if (GetMessenger().SendMessage(B_QUIT_REQUESTED) != B_NO_ERROR)
            printf("EllipseDrawingWorker: Warning, couldn't send the 'I'm closing' BMessage!\n");
      break;

      case SD_COMMAND_TEMPO:
      {
          int32 tempo;
          if ((msg->FindInt32(SD_NAME_TEMPO, &tempo) == B_NO_ERROR) &&
              (_window->Lock()))
          {
             _window->SetTempo(tempo);
             _window->Unlock();
          }
      }
      break;
                  
      default:
         printf("EllipseDrawingWorker [%s]:  Unknown BMessage received, listing follows:\n", GetName());
         msg->PrintToStream();
      break;
   }
}

bool
EllipseDrawingWorker ::
IsInterestedIn(BMessage * msg)
{
   // The only BMessage SockHop could send us that we care about is the SD_COMMAND_TEMPO message.
   return ((msg->what == SD_COMMAND_TEMPO)||(msg->what == SD_COMMAND_WINDOW_TO_FRONT));
}

