
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


#ifndef _SOCKHOPINTERNALCONSTANTS_H_
#define _SOCKHOPINTERNALCONSTANTS_H_

#include <sockhop/SockHopConstants.h>

// The message codes below are reserved for SockHop's internal use.
// The user is NOT allowed to send messages in with these
// typecodes.
enum {
  SH_INTERNAL_BASE = SH_INTERNAL_COMMANDS_BEGIN,
  SH_INTERNAL_NODEPATH,
  SH_INTERNAL_KILLME,
  SH_INTERNAL_NEEDFILE,
  SH_INTERNAL_HEREISFILE,
  SH_INTERNAL_TAGUNREF,
  SH_INTERNAL_NOOP,
  SH_INTERNAL_LINKREQUEST,
  SH_INTERNAL_NEWLINK,    // Send by shPendingLink to shNode
  SH_INTERNAL_LINKFAILED, // Posted by request-receiving nodes.
  SH_INTERNAL_LINKREQUESTCOMPLETE,  // posted (whenDone)
  SH_INTERNAL_REMOVEWORKER,
  SH_INTERNAL_WILLCONNECTBACK,
  SH_INTERNAL_RECEIVETHREADERROR,
  SH_INTERNAL_SERVERSTARTINFO,
  SH_INTERNAL_CONNECTIONACCEPTED,  // Sent by connection acceptor to confirm that the connection was accepted.
  //...
  SH_INTERNAL_LARGEST   // marks the end of the range
};

#endif