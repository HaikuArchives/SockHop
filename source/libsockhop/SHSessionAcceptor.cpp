
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


#include "socket.h"
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <support/Autolock.h>
#include <sockhop/SHSessionAcceptor.h>
#include <sockhop/SHDirectConnection.h>
#include "shConnection.h"
#include "shNode.h"
#include "shSockIO.h"
#include "shPendingLink.h"

SHSessionAcceptor ::
SHSessionAcceptor(const BMessenger & target, SHAccessPolicy * policy, bool startNow)
   : _messageTarget(target), _policy(policy), _acceptThreadId(-1), _acceptorSock(-1)
{
   if (startNow) (void) Start();
}

bool
SHSessionAcceptor ::
Start()
{
   if (_acceptThreadId >= 0) return true;  // We're already started!

   _acceptorSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (_acceptorSock >= 0)
   {
      _acceptAt = _policy->GetListeningLocation();

      sockaddr_in localAddr;
      localAddr.sin_family = AF_INET;
      localAddr.sin_port = htons(_acceptAt.GetPortNumber());
      localAddr.sin_addr.s_addr = INADDR_ANY;
      memset(localAddr.sin_zero, 0, sizeof(localAddr.sin_zero));

      if (shSharedConnectionImp::HackBind(_acceptorSock, (struct sockaddr *)&localAddr, sizeof(localAddr)) == B_NO_ERROR)
      {
         if (listen(_acceptorSock, 50) == B_NO_ERROR)
         {
            char myHostName[512];
            gethostname(myHostName, sizeof(myHostName));

            _acceptAt.SetHostName(myHostName);
            _acceptAt.SetPortNumber(ntohs(localAddr.sin_port));

            _acceptThreadId = spawn_thread(acceptLoopStub, "SHSessionAcceptor accept thread", _policy->GetDefaultThreadPriority(), this);
            if (_acceptThreadId >= 0)
            {
               if (resume_thread(_acceptThreadId) == B_NO_ERROR) 
               {
                  // success!
                  return true;
               }
               kill_thread(_acceptThreadId);
               _acceptThreadId = -1;
            }
            else printf("SHSessinAcceptor: spawn_thread() failed!\n");
         }
         else printf("SHSessionAcceptor: listen() failed!\n");
      }
      else printf("SHSessionAcceptor:  bind() failed (couldn't bind to port %li)\n", _acceptAt.GetPortNumber());
      
      closesocket(_acceptorSock);
      _acceptorSock = -1;
   }
   return false;
}

SHSessionAcceptor ::
~SHSessionAcceptor()
{
   // This will cause the sessionAcceptor to exit, if it is still running.
   if (_acceptorSock >= 0) 
   {
      // This will make sure the accept socket is closed.  
      // Closing the socket will cause the accept thread to exit.      
      while((closesocket(_acceptorSock) != B_NO_ERROR)&&(errno != EINTR)) {/* empty--try again! */}
   }
   if (_acceptThreadId >= 0)
   {
      long junk;
      wait_for_thread(_acceptThreadId, &junk);
   }
   CloseAllSessions();   // Do this after stopping the accept thread, so no last-minute connections can sneak by
   delete _policy;
}


void
SHSessionAcceptor ::
CloseAllSessions()
{
   BAutolock m(_synchList);   
   int num = _sessions.CountItems();
   for (int i=0; i<num; i++)
   { 
      SHDirectConnection * next = (SHDirectConnection *) _sessions.ItemAt(i);
      delete next;
   }
   _sessions.MakeEmpty();
}


status_t
SHSessionAcceptor ::
CloseSession(int32 sessionID)
{
   BAutolock m(_synchList);
   int indx;
   SHDirectConnection * conn = GetConnectionByID(sessionID, &indx);
   if (conn)
   {
      delete conn;
      (void)_sessions.RemoveItem(indx);
      return B_NO_ERROR;
   }
   return B_ERROR;
}

status_t
SHSessionAcceptor ::
SendMessageToSession(int32 sessionID, const BMessage & message)
{
   BAutolock m(_synchList);
   SHDirectConnection * conn = GetConnectionByID(sessionID);
   return (conn ? conn->GetMessenger().SendMessage((BMessage *)&message) : B_ERROR);
}

status_t
SHSessionAcceptor ::
SendMessageToAllSessions(const BMessage & msg)
{
   status_t ret = B_NO_ERROR;
   BAutolock m(_synchList);
   
   int num = _sessions.CountItems();
   for (int i=0; i<num; i++)
   { 
      SHDirectConnection * next = (SHDirectConnection *) _sessions.ItemAt(i);
      status_t thisRet = next->GetMessenger().SendMessage((BMessage *)&msg);
      if (thisRet != B_NO_ERROR) ret = thisRet;
   }
   return ret;
}

SHDirectConnection *
SHSessionAcceptor ::
DetachSession(int32 sessionID)
{
   BAutolock m(_synchList);
   int indx;
   SHDirectConnection * conn = GetConnectionByID(sessionID, &indx);
   if (conn)
   {
      (void)_sessions.RemoveItem(indx);
      return conn;
   }
   return NULL;
}

void
SHSessionAcceptor ::
DetachAllSessions(BList & list)
{
   BAutolock m(_synchList);
   
   int num = _sessions.CountItems();
   for (int i=0; i<num; i++)
   { 
      SHDirectConnection * next = (SHDirectConnection *) _sessions.ItemAt(i);
      list.AddItem(next);
   }
   _sessions.MakeEmpty();
}


// Note:  Doesn't do locking!
SHDirectConnection *
SHSessionAcceptor ::
GetConnectionByID(int32 id, int * optSetIndex)
{
   int num = _sessions.CountItems();
   for (int i=0; i<num; i++)
   {
      SHDirectConnection * next = (SHDirectConnection *) _sessions.ItemAt(i);
      if (next->GetId() == id)
      {
         if (optSetIndex) *optSetIndex = i;
         return next;
      }      
   }
   return NULL;
}

    
SHNodeSpec 
SHSessionAcceptor ::
GetAcceptSpec() const
{
   return _acceptAt;
}

    
long 
SHSessionAcceptor ::
acceptLoopStub(void * data)
{
    return(((SHSessionAcceptor *)data)->acceptLoop());
}

long 
SHSessionAcceptor ::
acceptLoop()
{
   while(1)
   {
      struct sockaddr_in clientAddr;
      socklen_t size = sizeof(clientAddr);
      int requestSock = accept(_acceptorSock, (struct sockaddr *) &clientAddr, &size);
      if (requestSock >= 0)
      {
          bool closeRequestSock = true;
          struct sockaddr_in saTempAdd;
          socklen_t lLength = sizeof(saTempAdd);
          if (getpeername(requestSock, (struct sockaddr *) &saTempAdd, &lLength) >= 0)
          {
             struct hostent * hePeer = gethostbyaddr((caddr_t)&saTempAdd.sin_addr, sizeof(saTempAdd.sin_addr), AF_INET);  // NULL result is okay here
             SHNodeSpec sessionSpec;

             shSockIO io(requestSock);
             if (RecvFlatObject(io, sessionSpec, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR)
             {
                 sessionSpec.SetHostName(hePeer ? hePeer->h_name : inet_ntoa(saTempAdd.sin_addr));
                 if (_policy->OkayToAcceptConnection(sessionSpec))
                 {                 
                     SHDirectConnection * newConn = new SHDirectConnection(_messageTarget, requestSock, sessionSpec, false, -1, GetNextConnectionID(), _policy->GetDefaultThreadPriority(), _policy->GetDefaultTransmissionEncoding());
                     closeRequestSock = false;  // newConn will take care of it!

                     // Tell him he's good to go!  Any flattened object will do, so how about an SHNodeSpec.
                     SHNodeSpec dummySpec;                        
                     if (SendFlatObject(io, dummySpec, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR)
                     {
                        // Add the new connection to our list!
                        BAutolock m(_synchList);
                        _sessions.AddItem(newConn);
                        if (_policy->OnSessionAccepted(*newConn))
                        {
                           if (newConn->Start()) 
                           {
                              newConn = NULL;  // okay, we'll keep him!  Set to NULL so the delete (below) won't delete it.
                           }
                           else printf("SHSessionAcceptor:  Couldn't start new connection!\n");
                        }
                        else printf("SHSessionAcceptor:  SHAccessPolicy::OnSessionAccepted() rejected connection, or there was an error spawning the connection threads!  SHDirectConnection aborted.\n");
                        
                        if (newConn) _sessions.RemoveItem(newConn);  // oops, nevermind then
                     }
                     else printf("SHSessionAcceptor:  SHAcceptPolicy::OnSessionAccepted() denied the connection.\n");
                 
                     delete newConn;  // No-op if it was set to NULL above, otherwise it's not going to be used so delete it.
                 }
                 else printf("SHSessionAcceptor:  SHAcceptPolicy::OkayToAcceptConnection() denied the connection.\n");
             }
             else printf("SHSessionAcceptor:  RecvFlatObject() failed!\n");
          }
          else printf("SHSessionAcceptor:  Couldn't get peer name for new session, aborting.\n");                   
          if (closeRequestSock) closesocket(requestSock);
      }
      else break;
   }
   return 0;
}

