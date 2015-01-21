
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


#ifndef _SHMESSAGESTREAM_H_
#define _SHMESSAGESTREAM_H_

#include <app/Looper.h>
#include <app/Messenger.h>
#include <support/Locker.h>

//Abstract interface for an object that can send and receive BMessages.
class shMessageStream : public BLooper
{
public:
  shMessageStream(const BMessenger & replyTarget, int32 threadPri);
  virtual ~shMessageStream();

protected:
  void SetReplyTarget(const BMessenger & newTarget);  
  virtual status_t PostReplyMessage(BMessage * msg);
  
  BLocker _synchTarget;
  BMessenger _replyTarget;
};

#endif
