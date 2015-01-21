
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


#include "shNodeMessageStream.h"
#include "shNode.h"

shNodeMessageStream::
shNodeMessageStream(const BMessenger & r, int32 parentId, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, int32 threadPri)
: shMessageStream(r, threadPri), _parentId(parentId), _id(id), _nodeSpec(spec), _activated(false), _connectTagID(-1), 
  _flags(0), _startupMessage(startupMessage.what)
{
   copyFieldTo(SH_NAME_ONSUCCESS, B_MESSAGE_TYPE, startupMessage, _startupMessage);
   copyFieldTo(SH_NAME_ONFAILURE, B_MESSAGE_TYPE, startupMessage, _startupMessage);
}

shNodeMessageStream::
~shNodeMessageStream()
{
   // empty
}

void
shNodeMessageStream::
SetConnectTagID(int id)
{
   _connectTagID = id;
}

int
shNodeMessageStream::
GetAndClearConnectTagID()
{
   int ret = _connectTagID;
   _connectTagID = -1;
   return ret;
}

const BMessage &
shNodeMessageStream::
GetStartupMessage()
{
   return _startupMessage;
}

int32
shNodeMessageStream::
GetID() const
{
   return _id; 
}

int32
shNodeMessageStream::
GetParentID() const
{
   return _parentId; 
}

const SHNodeSpec &
shNodeMessageStream::
GetNodeSpec()
{
   return _nodeSpec;
}

bool
shNodeMessageStream::
GetAndClearActivated()
{
   bool ret = _activated;
   _activated = false;
   return ret;
}

void
shNodeMessageStream::
SetActivated()
{
   _activated = true;
}

status_t
shNodeMessageStream::
PostReplyMessage(BMessage * msg)
{
   msg->RemoveName(SH_NAME_CONNECTIONID);
   if (_id != -1) msg->AddInt32(SH_NAME_CONNECTIONID, _id);
   return(shMessageStream::PostReplyMessage(msg));
}

void 
shNodeMessageStream::
AddFlag(uint32 flag)
{
   _flags |= flag;
}

void 
shNodeMessageStream::
RemoveFlag(uint32 flag)
{
   _flags &= ~(flag);
}

uint32 
shNodeMessageStream::
GetFlags() const
{
   return _flags;
}
