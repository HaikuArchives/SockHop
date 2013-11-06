
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


#ifndef _SHSOCKIO_H_
#define _SHSOCKIO_H_

#include <support/DataIO.h>
#include <support/Locker.h>

class shSockIO : public BDataIO
{
public:
   shSockIO(int sockfd);

   shSockIO(int sockfd, int * watchMe, int requiredVal, int pollIntervalSecs);
   // Tells the receive code to timeout every (pollInterval) microseconds,
   // look at *watchMe, and error out if it isn't equal to requiredVal.
   // This polling only occurs at this rate when the connection is quiet;  
   // when it is actively receiving, the poll will happen much more quickly--
   // once per loop.   

   virtual ~shSockIO();
   
   virtual ssize_t Read(void * buffer, size_t numBytes);
   virtual ssize_t Write(const void * buffer, size_t numBytes);

private:
   int _sockfd;

   int * _watchMe;
   int _requiredVal;
   int _pollIntervalSecs;
};

#endif
