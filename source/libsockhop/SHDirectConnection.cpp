
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


#include <support/Autolock.h>
#include <sockhop/SHDirectConnection.h>
#include "shStraightConnection.h"
#include "shAcceptConnection.h"
#include "shRawConnection.h"

static BMessage MakeDirectConnectionMessage()
{
   BMessage m(1);
   BMessage openMsg(SH_CODE_CONNECTIONOPEN);
   m.AddMessage(SH_NAME_ONSUCCESS, &openMsg);
   BMessage closedMsg(SH_CODE_CONNECTIONCLOSED);
   m.AddMessage(SH_NAME_ONFAILURE, &closedMsg);
   return m;
}

int32 
SHDirectConnection ::
GetNextIdNumber()
{
   static int32 idCounter = 0;
   static BLocker lock;
   
   BAutolock m(lock);
   return(idCounter++);
}

SHDirectConnection ::
SHDirectConnection(const BMessenger & target, const SHNodeSpec & spec, bool startImmediately, int32 threadPri, int32 batchEncoding)
  : _connection(new shConnection(new shStraightConnection, target, -1, GetNextIdNumber(), MakeDirectConnectionMessage(), spec, threadPri, batchEncoding)),
     _isStarted(false)
{
    if (startImmediately) _isStarted = _connection->StartThreads();
   _connection->Run();
}


SHDirectConnection ::
SHDirectConnection(const BMessenger & target, SHAccessPolicy * policy, bool startImmediately)
  : _connection(new shConnection(new shAcceptConnection(policy), target, -1, GetNextIdNumber(), MakeDirectConnectionMessage(), policy->GetListeningLocation(), policy->GetDefaultThreadPriority(), policy->GetDefaultTransmissionEncoding())),
    _isStarted(false)
{
   if (startImmediately) _isStarted = _connection->StartThreads();
   _connection->Run();
}


// This constructor is specially for the SHSessionAcceptor's use
SHDirectConnection ::
SHDirectConnection(const BMessenger & target, int requestSock, const SHNodeSpec & spec, bool startImmediately, int32 parentId, int32 id, int32 threadPri, int32 batchEncoding) 
  : _connection(new shConnection(new shRawConnection(requestSock), target, parentId, id, MakeDirectConnectionMessage(), spec, threadPri, batchEncoding)),
    _isStarted(false)
{
   if (startImmediately) _isStarted = _connection->StartThreads();
   _connection->Run();
}

SHDirectConnection ::
~SHDirectConnection()
{
   // stop the transfer thread and go away
   // Note that _connection may be NULL at this point, because
   // the shNode sometimes rips the connection out of our hands.
   if ((_connection)&&(_connection->Lock())) _connection->Quit();
}

void
SHDirectConnection ::
SetTagMessage(const char * name, const BMessage & msg)
{
   _connection->SetTagMessage(name, msg);
}

void
SHDirectConnection ::
SetTarget(const BMessenger & target)
{
   _connection->SetReplyTarget(target);
}

BMessenger
SHDirectConnection ::
GetMessenger() const
{
   return BMessenger(_connection);
}

int32
SHDirectConnection ::
GetId() const
{
   return _connection->GetID();
}

SHNodeSpec
SHDirectConnection ::
GetNodeSpec() const
{
   return _connection->GetNodeSpec();
}

BMessage &
SHDirectConnection ::
GetUserMessage()
{
   return _userMessage;
}

bool
SHDirectConnection ::
IsStarted() const
{
   return _isStarted;
}

bool
SHDirectConnection ::
Start()
{
   if (_isStarted) return true;
   _isStarted = _connection->StartThreads();
   return _isStarted;
}