
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


#ifndef _SHCALLBACKCONNECTION_H_
#define _SHCALLBACKCONNECTION_H_

#include "shStraightConnection.h"
#include "shAcceptConnection.h"

// This class will connect to the given destination, and wait for the given destination to connect
// back to it before continuing.
class shCallbackConnection : public shStraightConnection
{
public:
  shCallbackConnection();
  virtual ~shCallbackConnection();

  virtual bool StartThreads(int32 threadPri, int32 batchEncoding);
   
protected:
  virtual int SetupConnection();
  // Called by the setup thread.  Should not return till the connection is completely set up,
  // or the setup has failed.  Return a socket fd on success, or -1 on failure.

  virtual void CustomizeIDSpec(SHNodeSpec & spec);
  // Adds the callback info, so the server knows where to call us back at.
    
private:
  static SHAccessPolicy * MakeCallbackPolicy(const char ** setPassword);

  char * _password;  
  SHNodeSpec _acceptorSpec;
  SHAccessPolicy * _policy;
  int _acceptfd;
};

#endif
