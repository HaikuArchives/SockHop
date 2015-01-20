// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _EllipseDrawingWINDOW_H_
#define _EllipseDrawingWINDOW_H_

// This is the window that actually does the line drawing!
// One object of this class is held by each EllipseDrawingWindow.

#include <interface/Window.h>
#include <app/Messenger.h>
#include <interface/Point.h>

#define LDW_COMMAND_CLOSING 'ldwc'

class EllipseDrawingWindow : public BWindow
{
public:
    EllipseDrawingWindow(BRect rect, const char * name, BMessenger reportClosingTo);
    virtual ~EllipseDrawingWindow();
    
    virtual void MessageReceived(BMessage * msg);

    void SetTempo(int32 tempo);
        
private:
    static long DrawThreadStub(void * data);
    long DrawThread();
    BPoint RandomPointIn(const BRect & frame) const;
    
    BView * _drawArea;
    BMessenger _reportCloseTo;
    thread_id _drawThread;
    int32 _tempo;
};

#endif
