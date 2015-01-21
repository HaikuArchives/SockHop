// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _LINEDRAWINGWINDOW_H_
#define _LINEDRAWINGWINDOW_H_

// This is the window that actually does the line drawing!
// One object of this class is held by each LineDrawingWindow.

#include <interface/Window.h>
#include <app/Messenger.h>
#include <interface/Point.h>

#define LDW_COMMAND_CLOSING 'ldwc'

class LineDrawingWindow : public BWindow
{
public:
    LineDrawingWindow(BRect rect, const char * name, BMessenger reportClosingTo);
    virtual ~LineDrawingWindow();
    
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
