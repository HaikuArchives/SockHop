
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
#include "socket.h"
#include <stdio.h>
#include <kernel/OS.h>
#include <errno.h>
#include <posix/netdb.h>
#include "shSockIO.h"
#include <sys/select.h>

shSockIO::
shSockIO(int sockfd)
: _sockfd(sockfd), _watchMe(NULL)
{
   // empty
}

shSockIO::
shSockIO(int sockfd, int * watchMe, int requiredVal, int pollIntervalSecs)
: _sockfd(sockfd), _watchMe(watchMe), _requiredVal(requiredVal), _pollIntervalSecs(pollIntervalSecs)
{
   // empty
}

shSockIO::
~shSockIO()
{
   // empty
}

ssize_t
shSockIO::
Read(void * b, size_t numBytes)
{
   int indx = 0;
   char * buf = (char *)b;

   while(indx < numBytes)
   {
      bool readSomeBytes = true;  // default
      
      if (_watchMe)
      {
         struct fd_set fsReadSet;      
         FD_ZERO(&fsReadSet);
         FD_SET(_sockfd, &fsReadSet);
      
         struct timeval timeout;
         timeout.tv_sec  = _pollIntervalSecs;
         timeout.tv_usec = 0;

         // This call will return when there is data available to read, or after
         // (_pollIntervalSecs) seconds, whichever comes first.
         select(_sockfd+1, &fsReadSet, NULL, NULL, &timeout);               
            
         if (*_watchMe != _requiredVal) break;
         
         readSomeBytes = FD_ISSET(_sockfd, &fsReadSet);
      }
         
      if (readSomeBytes)
      {
         int result = recv(_sockfd, &buf[indx], numBytes-indx, 0);
         int err = errno;
      
              if (result  > 0) indx += result;
         else if (result == 0) return (indx > 0) ? indx : B_ERROR;
         else if (err != EINTR) return B_ERROR;
      }
  }
   
   return(indx);
}

ssize_t 
shSockIO::
Write(const void * b, size_t numBytes)
{
   int indx = 0;
   char * buf = (char *)b;
   
   while(indx < numBytes)
   {
      int result = send(_sockfd, &buf[indx], numBytes-indx, 0);      
      int err = errno;
      
           if (result  > 0) indx += result;
      else if (result == 0) return (indx > 0) ? indx : B_ERROR;
      else if (err != EINTR) return B_ERROR;
   }   
   return(indx);
}
