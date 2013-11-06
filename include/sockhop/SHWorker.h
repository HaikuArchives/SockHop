
/**************************************************************************
SockHop (libsockhop.so):  Distributed network programming system for BeOS
Copyright (C) 1999 by Jeremy Friesner (jaf@chem.ucsd.edu)

This library is free software; you can redistribute it and/or 
modify it under the terms of the GNU Library General Public 
License as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later version. 

This library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
Library General Public License for more details. 

You should have received a copy of the GNU Library General Public 
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
Boston, MA  02111-1307, USA. 
**************************************************************************/


#ifndef _SHWORKER_H_
#define _SHWORKER_H_

#include <sockhop/SHComponent.h>
#include <app/Messenger.h>
#include <app/Looper.h>

class shNode;

///////////////////////////////////////////////////////////////////
//
// SHWorker
//
// This class represents a user thread and its associated data,
// that can be propagated across the SockHop node tree.  
// Its semantics are somewhat similar to a BLooper's semantics,
// except that you can use an SHWorker as if it were a "normal"
// C++ object--that is, you can put it on the stack, delete it
// with the delete operator, etc.
//
///////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHWorker;
#else
#pragma export on
#endif 

class SHWorker : public SHComponent
{
public:
   /////////////
   // Lifecycle
   /////////////
   
   SHWorker();
   // Default constructor
   // Note that the SHWorker doesn't become "live" until you call the Start()
   // method on this object!
   
   virtual ~SHWorker();
   // Note that all subclasses of SHWorker should implement a destructor
   // that calls Stop() before doing anything else!

   /////////////////////////
   // SHComponent interface
   /////////////////////////
   
   ///////////////////////////////////
   // SHDistributableObject interface
   ///////////////////////////////////
   
   /////////////////////////
   // BArchivable interface
   /////////////////////////
   
   SHWorker(BMessage * archive);
   // BArchivable rehydration constructor
   // Note that the SHWorker doesn't become "live" until you call the Start()
   // method on this object!

   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of all subclass's Archive() methods
      
   //////////////////////////
   // SHWorker's new members
   //////////////////////////
      
   virtual bool IsInterestedIn(BMessage * msg);
   // Called in the node's server thread before sending (msg) to the contained BLooper.
   // If it returns false, (msg) will not be sent to this SHWorker.
   // This default implementation of IsInterestedIn() always returns true, 
   // but subclasses may override it to be more selective.
   // Note that since this method is called by a different thread than
   // the held BLooper's thread, you may need to synchronize access
   // to any data that is used by both this method and MessageReceived()
 
   BMessenger GetMessenger() const;
   // Post messages to the held BLooper through this BMessenger.
   
   virtual void Start();
   // Start the contained BLooper running.  Has no effect if it's already running.
   // You only need to override this if you want something special to happen
   // when the worker thread is started.  If you do override it, make sure
   // it calls SHWorker::Start() so that the BLooper is started properly.
   
   virtual void Stop();
   // Lock() and Quit() the contained BLooper, if it's running.
   // This will be called by the SHWorker destructor if necessary.
   // You only need to override this if you want something special to happen
   // when the worker thread is stopped.  If you do override it, make sure
   // it calls SHWorker::Stop() so that the BLooper is stopped properly.

   virtual void MessageReceived(BMessage * msg) = 0;
   // Put all your useful code into this method;  It has the same semantics
   // as BLooper::MessageReceived() does.

private:
   friend class shNode;
   
   BLooper * _heldLooper;
   bool _looperRunning;
   
   virtual void setNode(shNode * node);
   
   // FBC 
   virtual void _SHWorker1();
   virtual void _SHWorker2();
   virtual void _SHWorker3();
   virtual void _SHWorker4();
   uint32 _SHWorkerData[8];
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
