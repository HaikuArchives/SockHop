// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#include <stdio.h>
#include <stdlib.h>

#include "EllipseDrawingWindow.h"
#include "../SockDemo.h"

EllipseDrawingWindow :: 
EllipseDrawingWindow(BRect rect, const char * name, BMessenger reportCloseTo)
 : BWindow(rect, name, B_TITLED_WINDOW, 0),
   _reportCloseTo(reportCloseTo),
   _drawThread(-1), _tempo(3)
{
   _drawArea = new BView(BRect(0,0,1280,1024),"drawing view", B_FOLLOW_ALL_SIDES, 0);
   AddChild(_drawArea);
   
   // this will start up the draw thread when we run
   BMessage blank;
   PostMessage(&blank);
}

EllipseDrawingWindow ::
~EllipseDrawingWindow()
{
   thread_id temp = _drawThread;
   if (temp >= 0)
   {
      // wait for thread to die.  It will see that _drawThread is -1, and quit
      _drawThread = -1;
      long junk;
      wait_for_thread(temp, &junk);
   }
   
   BMessage closing(LDW_COMMAND_CLOSING);
   _reportCloseTo.SendMessage(&closing);
}

void
EllipseDrawingWindow ::
MessageReceived(BMessage *)
{
   // any message just starts the thread if it isn't started already!
   if (_drawThread < 0)
   {
       _drawThread = spawn_thread(DrawThreadStub, "EllipseDrawingWindow line drawing thread", 
                                  B_LOW_PRIORITY, this);
       if (_drawThread >= 0) resume_thread(_drawThread);
   }
}

void
EllipseDrawingWindow ::
SetTempo(int32 tempo)
{
   _tempo = tempo;
   
   // Get the draw thread to change speed immediately
   if (_drawThread >= 0)
   {
      suspend_thread(_drawThread);
      resume_thread(_drawThread);
   }
}

long
EllipseDrawingWindow ::
DrawThreadStub(void * data)
{
   return ((EllipseDrawingWindow *)data)->DrawThread();
}

long
EllipseDrawingWindow ::
DrawThread()
{
   uint32 lastSecond = 0;
   int32 numOps = 0;
   int numOpsInGroup = 15;
         
   while(_drawThread >= 0)
   {
      rgb_color color;

      if (LockWithTimeout(0) == B_NO_ERROR)
      {
          for (int i=0; i<numOpsInGroup; i++)
          {
	         BRect frame = Frame();
	         color.red = rand() % 256;
   	         color.blue = rand() % 256;
	         color.green = rand() % 256;
	         color.alpha = rand() % 256;
	         _drawArea->SetHighColor(color);
	         _drawArea->FillEllipse(BRect(RandomPointIn(frame), RandomPointIn(frame)));
	      }
	      Unlock();
	      numOps += numOpsInGroup;
	   }

       int32 snoozeTime;
       switch(_tempo)
       {
           default: /* fall-thru */
           case 1:  snoozeTime = 500000;   break;
           case 2:  snoozeTime = 100000;   break;
           case 3:  snoozeTime = 20000;    break;
           case 4:  snoozeTime = 5000;     break;
           case 5:  snoozeTime = 0;        break;
       }
       
       snooze(snoozeTime);  // deliberately give up CPU for a bit...
       	   
	   // Tell the GUI how many ops we did!
	   uint32 newSecond = real_time_clock();
	   if (newSecond > (lastSecond + 3))
	   {
	      BMessage stat(SD_COMMAND_WORKER_STATS);
	      stat.AddInt32(SD_NAME_STAT, numOps);
	      _reportCloseTo.SendMessage(&stat);
	      numOps = 0;
	      lastSecond = newSecond;
	   }   
   }
   return 0;
}

BPoint
EllipseDrawingWindow ::
RandomPointIn(const BRect & frame) const
{
   int x = (rand() % frame.IntegerWidth());
   int y = (rand() % frame.IntegerHeight());
   return BPoint(x,y);
}

