
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


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>

#include "socket.h"
#include "shAcceptConnection.h"
#include "shNode.h"
#include "shSockIO.h"

#include <support/Autolock.h>

shAcceptConnection ::
shAcceptConnection(SHAccessPolicy * policy)
 : _policy(policy), _sockfd(-1)
{
   // empty
}

shAcceptConnection ::
~shAcceptConnection()
{
   if (_sockfd >= 0) closesocket(_sockfd);
   delete _policy;
}

int
shAcceptConnection ::
BindPort(SHNodeSpec & spec)
{
   int ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (ret >= 0)
   {
      sockaddr_in newAddr;
      newAddr.sin_family      = AF_INET;
      newAddr.sin_addr.s_addr = INADDR_ANY;
      newAddr.sin_port        = htons(spec.GetPortNumber());
      memset(newAddr.sin_zero, 0, sizeof(newAddr.sin_zero));
  
      if (HackBind(ret, (struct sockaddr *)&newAddr, sizeof(newAddr)) == B_NO_ERROR)
      {
         if (listen(ret, 1) == B_NO_ERROR)
         {
            struct sockaddr_in queryAddr;
            int len = sizeof(queryAddr);

            if (getsockname(ret, (struct sockaddr *)&queryAddr, &len) == B_NO_ERROR)
	        {
               char hostName[512];
	           gethostname(hostName, sizeof(hostName));

               spec.SetPortNumber(ntohs(queryAddr.sin_port));
	           spec.SetHostName(hostName);
	           return ret;
	        }
	        else printf("BindPort:  getsockname() failed!\n");
	     }
	     else printf("BindPort:  Couldn't listen() on socket!\n");
	  }
      else printf("BindPort():  Couldn't bind socket to port %li!\n", spec.GetPortNumber());
   }
   else printf("BindPort():  Couldn't create socket!\n");

   closesocket(ret);
   ret = -1;
      
   return false;
}

SHNodeSpec
shAcceptConnection ::
GetAcceptorSpec() const
{
   return _acceptorSpec;
}

bool
shAcceptConnection ::
StartThreads(int32 threadPri, int32 batchEncoding)
{
   _acceptorSpec = _policy->GetListeningLocation();
   _sockfd = BindPort(_acceptorSpec);
   return ((_sockfd >= 0) ? shSharedConnectionImp::StartThreads(threadPri, batchEncoding) : false);
}

int
shAcceptConnection ::
SetupConnection()
{
   int temp = _sockfd;
   _sockfd = -1;  // because SetupConnection() will close it for us
   return SetupConnection(temp, _policy);  
}

int
shAcceptConnection ::
SetupConnection(int acceptSock, SHAccessPolicy * policy)
{
   if (acceptSock >= 0)
   {
      struct sockaddr_in saAddr;
      int addrSize = sizeof(saAddr);
      int newSockfd = accept(acceptSock, (struct sockaddr *)&saAddr, &addrSize); 

      if (newSockfd >= 0)
      {
         struct hostent * hePeer;
         
         hePeer = gethostbyaddr((caddr_t)&saAddr.sin_addr, sizeof(saAddr.sin_addr), AF_INET);

         // Check his credentials...
         SHNodeSpec clientIDSpec;
         shSockIO io(newSockfd);
         if (RecvFlatObject(io, clientIDSpec, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR)
         {
            clientIDSpec.SetHostName(hePeer ? hePeer->h_name : inet_ntoa(saAddr.sin_addr));
            if (policy->OkayToAcceptConnection(clientIDSpec)) 
            {
               closesocket(acceptSock);

               // Let the caller know his connection has been confirmed, by sending back this lovely free gift...
               SHNodeSpec dummySpec;
               if (SendFlatObject(io, dummySpec, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR) return newSockfd;
            }
            else printf("shAcceptConnection:  SHAccessPolicy denied the connection!\n");
         }
         else printf("shAcceptConnection:  RecvFlatObject failed!\n");         
         closesocket(newSockfd);
      }
      else printf("shAcceptConnection:  accept() failed!\n");
      closesocket(acceptSock);
   }
   return -1;   
}
