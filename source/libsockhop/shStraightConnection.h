
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


#ifndef _SHSTRAIGHTCONNECTION_H_
#define _SHSTRAIGHTCONNECTION_H_

#include "shConnection.h"

// This class just connects to the hostname/port given in (spec), and it's done.  Simple as that.
class shStraightConnection : public shSharedConnectionImp
{
public:
  shStraightConnection();
  virtual ~shStraightConnection();
 
protected:
  virtual void CustomizeIDSpec(SHNodeSpec & spec);
  // Allows subclasses to add to our ID spec before it is transmitted. 
  // Default implementation does nothing.
  
  virtual int SetupConnection();
  // Called by the setup thread.  Should not return till the connection is completely set up,
  // or the setup has failed.  Return a socket fd on success, or -1 on failure.
};

#endif
