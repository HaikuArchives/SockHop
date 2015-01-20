// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _FLAGWAVINGWINDOW_H_
#define _FLAGWAVINGWINDOW_H_

// This is the window that actually does the animation drawing!
// One object of this class is held by each FlagWavingWindow.

#include <interface/Window.h>
#include <app/Messenger.h>
#include <interface/Point.h>
#include <interface/Bitmap.h>

#define LDW_COMMAND_CLOSING 'ldwc'

class FlagWavingWindow : public BWindow
{
public:
    FlagWavingWindow(BRect rect, const char * name, BMessenger reportClosingTo, int32 width, int32 height);
    virtual ~FlagWavingWindow();
    
    virtual void MessageReceived(BMessage * msg);

    void SetTempo(int32 tempo);

private:
    class FlagPoint
    {
    public:
       rgb_color _color;
       rgb_color _origColor;
       BRect _rect;
       float x, y, z;
       float vx, vy, vz;
       float cx, cy, cz;
       float ax, ay, az;
       float angle;
       bool visible;
    };
    
    void InitializePoint(FlagPoint & point, int x, int y);
    void UpdatePoint(FlagPoint & point);
    void DrawPoint(FlagPoint & point);

    float rand01();

    static int ComparePoints(void *, void *);  // used by qsort
        
    static long DrawThreadStub(void * data);
    long DrawThread();
    BPoint RandomPointIn(const BRect & frame) const;

    BMessenger _reportCloseTo;
    int32 _width, _height;
    BView * _drawArea;
    thread_id _drawThread;
    int32 _tempo;
    
    float _frameW;  // cache in local mem
    float _frameH;  
    
    float _oneWidth;
    float _oneHeight;

    BBitmap * _backBuffer;    
    BView * _backDrawArea;
    
    float Sin(float x) const;
    float Cos(float x) const;
    
    float _sin[6280];
    float _cos[6280];
    
    float sphereRad;   
    float cSphereRad;
    float vSphereRad;
    float aSphereRad;

};

#endif
