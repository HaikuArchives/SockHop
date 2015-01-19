
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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "socket.h"
#include <sockhop/SHDefaultAccessPolicy.h>
#include "shCallbackConnection.h"
#include "shNode.h"
#include "shSockIO.h"
#include "SockHopInternalConstants.h"

shCallbackConnection ::
shCallbackConnection()
   : _acceptfd(-1)
{
   // Generate a password that the call-backer will need to echo to us so we can know its the same guy.
   // (Cheesy security, but slightly better than nothing!  At least it'll stop accidental misconnections)
   char temp[50];
   srand(time(NULL));
   for (int i=0; i<sizeof(temp); i++) temp[i] = (rand() % 255) + 1;
   temp[sizeof(temp)-1] = '\0';
   
   _password = new char[strlen(temp)+1];
   strcpy(_password, temp);
   _policy = new SHDefaultAccessPolicy(0, 0, temp);
}

shCallbackConnection ::
~shCallbackConnection()
{
   delete _policy;
   delete [] _password;
   
   if (_acceptfd >= 0) closesocket(_acceptfd);
}

bool 
shCallbackConnection ::
StartThreads(int32 threadPri, int32 batchEncoding)
{
   _acceptorSpec = _policy->GetListeningLocation();
   _acceptfd = shAcceptConnection::BindPort(_acceptorSpec);
   return ((_acceptfd >= 0) && (shStraightConnection::StartThreads(threadPri, batchEncoding)));
}

int 
shCallbackConnection ::
SetupConnection()
{
   if (_acceptfd < 0) 
   {
      printf("shCallbackConnection: accept port was never set up???\n");
      return -1;
   }
   
   // First, contact the server...
   int sockfd = shStraightConnection :: SetupConnection();
   if (sockfd >= 0)
   {
      SHNodeSpec junk;

      // If the server is going to call us back, he'll send us a BMessage to tell us that.
      BMessage reply;
      shSockIO io(sockfd);
      if (reply.Unflatten(&io) == B_NO_ERROR)//&&(reply.what == SH_INTERNAL_WILLCONNECTBACK))
      {
          closesocket(sockfd);
          int ret = shAcceptConnection::SetupConnection(_acceptfd, _policy);  // will close _acceptfd
          _acceptfd = -1;
          return ret;
      }
      else printf("shCallbackConnection:  Error, didn't get the WILLCONNECTBACK from the server!  (Got [%s]?)\n", shNode::TypeToString(reply.what));
      closesocket(sockfd);
   }  
   return -1;   
}

void
shCallbackConnection ::
CustomizeIDSpec(SHNodeSpec & spec)
{
   spec.GetSpecMessage().AddFlat(SH_NAME_CALLBACKSPEC, &_acceptorSpec);
   spec.GetSpecMessage().AddString("password", _password);
}
