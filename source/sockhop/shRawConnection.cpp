
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
#include <socket.h>
#include "shRawConnection.h"

shRawConnection ::
shRawConnection(int sockfd)
  : _sockfd(sockfd)
{
   // empty
}


shRawConnection ::
~shRawConnection()
{
   if (_sockfd >= 0) closesocket(_sockfd);  // just in case SetupConnection() was never called!
}
 
int 
shRawConnection ::
SetupConnection()
{
   int temp = _sockfd;
   _sockfd = -1;  // This marks that we've been through the setup process;  the socket is no longer
   return temp;   // our responsibility.
}
