
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


#ifndef _SHACCEPTCONNECTION_H_
#define _SHACCEPTCONNECTION_H_

#include "shConnection.h"
#include <sockhop/SHAccessPolicy.h>

class shCallbackConnection;

// An shConnection that will wait on a port as described by (policy), and
// accept or reject a single connection (also as determined by (policy))
class shAcceptConnection : public shSharedConnectionImp
{
public:
   shAcceptConnection(SHAccessPolicy * policy);
   // (policy) becomes the property of this object.
  
   virtual ~shAcceptConnection();
 
   virtual bool StartThreads(int32 threadPri, int32 batchEncoding);
   // Calls BindPort(), and if that succeeds, calls shConnection::StartThreads().
   
   virtual SHNodeSpec GetAcceptorSpec() const;
   // Returns an SHNodeSpec based on (policy)'s ListeningSpec, but with the hostname and port number
   // set so that it will connect to the port this connection is listening on.
   // Only valid after StartThreads() has succeeded, and before a valid connection has been made.

   static int BindPort(SHNodeSpec & spec);
   // Called by StartThreads(), and by shCallbackConnection.
      
protected:
   friend class shCallbackConnection;
   
   virtual int SetupConnection();
   // Called by the setup thread.  Should not return till the connection is completely set up,
   // or the setup has failed.  Return a socket fd on success, or -1 on failure.

   static int SetupConnection(int acceptSock, SHAccessPolicy * policy);
   // Called by SetupConnection(), and by shCallbackConnection, since BeOS doesn't like me
   // to instantiate a spare shAcceptConnection (it hangs on shutdown)
   
private:  
   SHNodeSpec _acceptorSpec;
   int _sockfd;
   SHAccessPolicy * _policy;
};

#endif
