
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


#include <stdio.h>
#include <support/Autolock.h>
#include <sockhop/SHWorker.h>
#include "shNode.h"
#include "SockHopInternalConstants.h"

class shWorkerLooper : public BLooper
{
public:
   shWorkerLooper(SHWorker * owner, int32 id);
   ~shWorkerLooper();
       
   virtual void MessageReceived(BMessage * msg);

   void setNode(shNode * node);
   shNode * getNode() const;
   
private:
   SHWorker * _owner;
   shNode *   _node;
   int32      _id;
};


shWorkerLooper::
shWorkerLooper(SHWorker * owner, int32 id)
   : _owner(owner), _node(NULL), _id(id)
{
   // empty
}

shWorkerLooper::
~shWorkerLooper()
{
   // Make sure we get removed from the node's list.
   // This message won't have any effect if our owner has already been removed from the list...
   if (_node)
   {
      BMessage message(SH_INTERNAL_REMOVEWORKER);
      message.AddInt32(SH_NAME_WORKERID, _id);
      message.AddString(SH_NAME_WHICHSORTER, _node->GetSystemSorterName());
      message.AddString(SH_NAME_TO, "");
      if (BMessenger(_node).SendMessage(&message) != B_NO_ERROR)
         printf("shWorkerLooper::~shWorkerLooper() : Warning, couldn't send delete-me message!\n");
   }
}

void
shWorkerLooper::
setNode(shNode * node)
{
   _node = node;
}

shNode *
shWorkerLooper::
getNode() const
{
   return _node;
}


void
shWorkerLooper::
MessageReceived(BMessage * msg)
{
   _owner->MessageReceived(msg);
}

   
SHWorker::
SHWorker() 
   : _heldLooper(new shWorkerLooper(this, GetId())), _looperRunning(false)
{
   // empty
}

SHWorker::
SHWorker(BMessage * archive)
   : SHComponent(archive), _heldLooper(new shWorkerLooper(this, GetId())), _looperRunning(false)
{
   // empty
}

SHWorker::
~SHWorker()
{
   if ((_node)&&(_looperRunning))
   {
     _node->printf("SHWorker::~SHWorker():  WARNING!! I *strongly* recommend that you\n"
                   "call Stop() as the first thing in all of your SHWorker subclass destructors!\n"
                   "Otherwise, you will suffer crashes due to race conditions!\n");
   }
   if (_heldLooper->Lock()) _heldLooper->Quit();
}

BMessenger
SHWorker::
GetMessenger() const
{
   BMessenger ret(_heldLooper);
   return ret;
}

void
SHWorker::
Start()
{
   if (_looperRunning == false) 
   {
      _heldLooper->Run();
      _looperRunning = true;
   }
}

void
SHWorker::
Stop()
{
   if (_heldLooper->Lock()) _heldLooper->Quit();
   _heldLooper = new shWorkerLooper(this, GetId());   // to collect messages while we're not running
   _looperRunning = false;
}

bool
SHWorker::
IsInterestedIn(BMessage *)
{
   return true;  // By default, all messages interest us
}

status_t
SHWorker::
Archive(BMessage * archive, bool deep) const
{
   return SHComponent::Archive(archive, deep);
}

void
SHWorker::
setNode(shNode * node)
{
   SHComponent::setNode(node);
   if (_heldLooper) {((shWorkerLooper *)_heldLooper)->setNode(node);}
}


/* FBC */
void SHWorker::_SHWorker1() {}
void SHWorker::_SHWorker2() {}
void SHWorker::_SHWorker3() {}
void SHWorker::_SHWorker4() {}