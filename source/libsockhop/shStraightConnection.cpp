
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
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include "shStraightConnection.h"
#include "shNode.h"
#include "shSockIO.h"

shStraightConnection ::
shStraightConnection()
{
   // empty
}


shStraightConnection ::
~shStraightConnection()
{
   // empty
}
 
void
shStraightConnection ::
CustomizeIDSpec(SHNodeSpec &)
{
   // empty
}

int 
shStraightConnection ::
SetupConnection()
{
   int socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (socketfd >= 0)
   {
      SHNodeSpec spec = GetNodeSpec();
      const char * hostname = spec.GetHostName();
      struct hostent * hent = ((hostname)&&(*hostname)) ? gethostbyname(hostname) : NULL;
      if (hent)
      {
         struct sockaddr_in address;
         memset(&address, 0, sizeof(address));
         address.sin_family = AF_INET;
         address.sin_port = htons(spec.GetPortNumber());
         address.sin_addr.s_addr = *(unsigned long *)(hent->h_addr);
         memset(address.sin_zero, 0, sizeof(address.sin_zero));

         if (connect(socketfd, (struct sockaddr *)&address, sizeof(address)) == 0)
         {
            CustomizeIDSpec(spec);
            
            // Send our ID!
            shSockIO io(socketfd);
            if (SendFlatObject(io, spec, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR) 
            {
               // Wait for response
               SHNodeSpec dummy;
               if (RecvFlatObject(io, dummy, SH_MAX_INITSPEC_SIZE) == B_NO_ERROR) 
               {
                   return socketfd;
               }
               else printf("shStraightConnection:  Didn't get the reply SHNodeSpec! (Guess the server doesn't like me)\n"); 
            }
            else printf("shStraightConnection:  Error sending my SHNodeSpec ID.\n");
         }
         else printf("shStraightConnection:  connect() failed!\n");
      }
      else printf("shStraightConnection:  Error looking up hostname [%s]\n", hostname);

      closesocket(socketfd);
   }
   else printf("shStraightConnection:  socket() failed.\n");

   return -1;
}
