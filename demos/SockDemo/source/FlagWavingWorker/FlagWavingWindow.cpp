// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "FlagWavingWindow.h"
#include "../SockDemo.h"
#include <interface/Screen.h>


FlagWavingWindow :: 
FlagWavingWindow(BRect rect, const char * name, BMessenger reportCloseTo, int32 w, int32 h)
 : BWindow(rect, name, B_TITLED_WINDOW, 0),
   _reportCloseTo(reportCloseTo),
   _width(w), _height(h), _drawThread(-1), _tempo(3) 
{
   sphereRad=0.60f;
   cSphereRad = 0.4f;
   vSphereRad = -.008f;
   aSphereRad = 0.00003f;

   {
      BScreen screen;
      _backBuffer = new BBitmap(screen.Frame(), screen.ColorSpace(), true);
      _drawArea = new BView(screen.Frame(), "drawing view", B_FOLLOW_ALL_SIDES, 0);
      _backDrawArea = new BView(screen.Frame(), "back view", B_FOLLOW_ALL_SIDES, 0);
   }
   
   AddChild(_drawArea);
   _backBuffer->AddChild(_backDrawArea);

   // this will start up the draw thread when we run
   PostMessage(123);  // any message will do
   
   const int numSpots = (sizeof(_cos)/sizeof(float));
   for (int i=0; i<numSpots; i++)
   {
      float x = 6.28f * ((float)i)/numSpots;
      _cos[i] = ::cos(x);
      _sin[i] = ::sin(x);
   }
}

float FlagWavingWindow ::
Cos(float x) const
{
   const int numSpots = (sizeof(_cos)/sizeof(float));
   int ix = ((int) ((x/6.28f) * numSpots)) % numSpots;
   return _cos[ix];
}

float FlagWavingWindow ::
Sin(float x) const
{
   const int numSpots = (sizeof(_cos)/sizeof(float));
   int ix = ((int) ((x/6.28f) * numSpots)) % numSpots;
   return _sin[ix];
}


FlagWavingWindow ::
~FlagWavingWindow()
{
   delete _backBuffer;

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
FlagWavingWindow ::
MessageReceived(BMessage *)
{
   // any message just starts the thread if it isn't started already!
   if (_drawThread < 0)
   {
       _drawThread = spawn_thread(DrawThreadStub, "FlagWavingWindow line drawing thread", 
                                  B_LOW_PRIORITY, this);
       if (_drawThread >= 0) resume_thread(_drawThread);
   }
}

long
FlagWavingWindow ::
DrawThreadStub(void * data)
{
   return ((FlagWavingWindow *)data)->DrawThread();
}

void
FlagWavingWindow ::
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

float FlagWavingWindow :: rand01()
{

   return ((float)rand()) / (float)RAND_MAX;
}

void FlagWavingWindow :: InitializePoint(FlagPoint & p, int x, int y)
{
    float pix = ((float)x/(float)_width) * 6.28f;
    float piy = ((float)y/(float)_height) * 6.28f;

    p.cx = 0.5f;
    p.cy = 0.5f;
    p.cz = 2.0f;
    
    float r = 1.0f;
    
    p.x = (sphereRad * Cos(pix+rand01()*r)) + p.cx;
    p.ay = p.y = 4.f*(sphereRad * Sin(piy+rand01()*r)) + p.cy;
    p.ax = rand01()*0.2f;
    p.z = (sphereRad * Sin(pix+rand01()*r)) + p.cz;    
p.vx = 0.0f;  // radial velocity
p.vy = 0.0f;
p.vz = rand01() * 0.05f;  // extra axial velocity
    
p.angle = rand01() * 6.28f;   
   p._color.red   = rand() % 256;
   p._color.blue  = rand() % 256;
   p._color.green = rand() % 256;
   p._color.alpha = rand() % 256;

   p._origColor = p._color;
      
   p.visible = true;
}

/*
void FlagWavingWindow :: InitializePoint(FlagPoint & p, int x, int y)
{
//   p.cx = ((float)x/(float)_width);
//   p.cy = ((float)y/(float)_height);
//   p.cz = 1.5f;

   p.cx = 0.5f;
   p.cy = 0.5f;
   p.cz = 1.5f;

   float maxDist = 0.35;   
//   p.x = p.cx + (sin((x+y)*6.28f)*maxDist);
//   p.y = p.cy + (cos((x-y)*6.28f)*maxDist);
//   p.z = p.cz + (sin((x+y)*6.28f)*maxDist);

//   p.x = p.cx + (rand01()*6.28f)*maxDist;
//   p.y = p.cy + (rand01()*6.28f)*maxDist;
//   p.z = p.cz + (rand01()*6.28f)*maxDist*maxDist;

   p.x = p.cx - 0.5*p.cx + rand01()*0.03;
   p.y = p.cy - 0.3*p.cy + rand01()*0.02;
   p.z = p.cz + 0.1*p.cz;

   p.ax = p.ay = p.az = 0.001f;
   p.vx = p.vy = p.vz = 0.0f;
   
   p._color.red   = rand() % 256;
   p._color.blue  = rand() % 256;
   p._color.green = rand() % 256;
   p._color.alpha = rand() % 256;
}
*/

/*
void FlagWavingWindow :: UpdatePoint(FlagPoint & p)
{
   // drag
   const float drag = 1.00001f;
   p.vx *= drag;
   p.vy *= drag;
   p.vz *= drag;
         
   // velocity
   p.x += p.vx;
   p.y += p.vy;
   p.z += p.vz;
   
   // acceleration
   if (p.x < p.cx) p.vx += p.ax;
   if (p.x > p.cx) p.vx -= p.ax;

   if (p.y < p.cy) p.vy += p.ay;
   if (p.y > p.cy) p.vy -= p.ay;
   
   if (p.z < p.cz) p.vz += p.az;
   if (p.z > p.cz) p.vz -= p.az;

   if (p.z < 0.5) 
   {
      p.visible = false;
      return;
   }
   
   float w = (_oneWidth / p.z);
   float h = (_oneHeight / p.z);

   p._rect.top    = p.y - h;
   p._rect.bottom = p.y + h;
   p._rect.left   = p.x - w;
   p._rect.right  = p.x + w;
}
*/

void FlagWavingWindow :: UpdatePoint(FlagPoint & p)
{
   const float spAccel = 0.0001f;
   if (p.ax > 0.0f) p.vx -= spAccel;
   if (p.ax < 0.0f) p.vx += spAccel;
const float drag = 0.999f;
p.vx *= drag;
   p.ax += p.vx;

const float epsilon = 0.0004f;
if ((fabs(p.x - sphereRad) < epsilon)&&(fabs(p.vx) < epsilon)) {
  // poke it out!
  p.vx = (rand01()-0.5f)*0.08f;
}
   float sR = sphereRad + p.ax;
   float distToCenter = fabs(p.y - p.cy) / sR;
   float sy = Cos(distToCenter);

   p.angle += 0.04f + p.vz;
   p.z = p.cz + (Sin(p.angle) * sR);
   p.y = p.cy + (p.ay-p.cy)*sR;
   p.x = p.cx + (Cos(p.angle) * sy * sR);

if (p.z < 0.1) {
   p.visible = false;
   return;
}
else p.visible = true;

   float cDiff = p.z/2.0f;
   float red = p._origColor.red / cDiff;
   float green = p._origColor.green / cDiff;
   float blue = p._origColor.blue / cDiff;
   if (red > 255) red = 255;
   if (green > 255) green = 255;
   if (blue > 255) blue = 255;
   p._color.red   = (uint8) red;
   p._color.green = (uint8) green;
   p._color.blue  = (uint8) blue;   

   float w = (_oneWidth / p.z);
   float h = (_oneHeight / p.z);

   p._rect.top    = p.y - h;
   p._rect.bottom = p.y + h;
   p._rect.left   = p.x - w;
   p._rect.right  = p.x + w;

}

void FlagWavingWindow :: DrawPoint(FlagPoint & p)
{	         
if (p.visible == false) return;
   _backDrawArea->SetHighColor(p._color);
   BRect rect = p._rect;
   
   rect.left *= _frameW;
   rect.right *= _frameW;
   rect.top *= _frameH;
   rect.bottom *= _frameH;
   
   _backDrawArea->FillRect(rect);
}

int FlagWavingWindow :: ComparePoints(void * point1, void * point2)
{
   float z1 = (*((FlagPoint **) point1))->z;
   float z2 = (*((FlagPoint **) point2))->z;
   return (z1 == z2) ? 0 : ((z1 > z2) ? -1 : 1);
}

long
FlagWavingWindow ::
DrawThread()
{
   uint32 lastSecond = 0;
   int32 numOps = 0;

   int numPoints = _width * _height;
   if (numPoints <= 0) 
   {
      printf("FlagWavingWindow:  Error, no points (%i)!\n", numPoints);
      numPoints = 1;
      _width = _height = 0;
   }
   FlagPoint ** points = new FlagPoint * [numPoints];

   if (Lock())
   {
      _frameW = Frame().Width();
      _frameH = Frame().Height();
      _oneWidth = 1.0f / _width;
      _oneHeight = 1.0f / _height;
      Unlock();
   }
   
   // Initialize points
   for (int ix = 0; ix < _width; ix++)
     for (int iy = 0; iy < _height; iy++)
     {
        points[iy * _width + ix] = new FlagPoint;
        InitializePoint(*points[iy * _width + ix], ix, iy);
     }

   rgb_color black;  black.red = black.green = black.blue = 0;
          
   while(_drawThread >= 0)
   {
       for (int n=0; n<numPoints; n++) UpdatePoint(*points[n]);

//       qsort(points, numPoints, sizeof(FlagPoint *), (__compar_fn_t) ComparePoints);

// velocity
sphereRad += vSphereRad;
// acceleration
if (sphereRad < cSphereRad) vSphereRad += aSphereRad;
if (sphereRad > cSphereRad) vSphereRad -= aSphereRad;

       if (LockWithTimeout(0) == B_NO_ERROR)
       {
          _frameW = Frame().Width();
          _frameH = Frame().Height();
 
          Unlock();         
       
          _oneWidth = 1.0f / _width;
          _oneHeight = 1.0f / _height;

          if (_backBuffer->Lock())
          {
             _backDrawArea->SetHighColor(black);          
             _backDrawArea->FillRect(BRect(0,0,_frameW, _frameH));
           
             for (int d=0; d<numPoints; d++) DrawPoint(*points[d]);

             _backDrawArea->Sync();
             _backBuffer->Unlock();
          }
          numOps += numPoints;
                     
          if (LockWithTimeout(0) == B_NO_ERROR)
          {
             _drawArea->DrawBitmap(_backBuffer);
	         Unlock();
	      }
	   }

       int32 snoozeTime = 0;
       switch(_tempo)
       {
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

   for (int i=0; i<numPoints; i++) delete points[i];
   
   delete [] points;

   return 0;
}

BPoint
FlagWavingWindow ::
RandomPointIn(const BRect & frame) const
{
   int x = (rand() % frame.IntegerWidth());
   int y = (rand() % frame.IntegerHeight());
   return BPoint(x,y);
}
