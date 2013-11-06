
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


#ifndef SHDIRECTCONNECTION_H
#define SHDIRECTCONNECTION_H

#include <app/Messenger.h>
#include <sockhop/SockHopConstants.h>
#include <sockhop/SHNodeSpec.h>
#include <sockhop/SHAccessPolicy.h>

/////////////////////////////////////////////////////////////////////////////////////////
//
// SHDirectConnection
//
// A simple utility class to make direct, one to one TCP connections between programs.  
// Can be used both to listen for active connections, and as an active connection.
// Useful for programs that aren't components of the SockHop tree, and thus can't 
// simply send messages up and down the tree.  Note that an SHDirectConnection object 
// can only connect to another SHDirectConnection object, or a SHSessionAcceptor object!
//
/////////////////////////////////////////////////////////////////////////////////////////

class shConnection;
class shNode;
class SHSessionAcceptor;

#ifdef __INTEL__
_EXPORT class SHDirectConnection;
#else
#pragma export on
#endif 

class SHDirectConnection
{
public:
   //////////////
   // Lifecycle
   /////////////
   
   SHDirectConnection(const BMessenger & target, const SHNodeSpec & spec, bool startImmediately, int32 threadPri = B_NORMAL_PRIORITY, int32 transmissionEncoding = SH_ENCODING_NONE);
   // Creates an SHDirectConnection object that will try to connect to (spec).
   // BMessages that the SHDirectConnection receives from the connection will be forwarded to (target).
   // If (startImmediately) is set to false, then the send and receive threads won't be started
   // until Start() is called.
   
   SHDirectConnection(const BMessenger & target, SHAccessPolicy * policy, bool startImmediately);
   // Creates an SHDirectConnection object that will listen on the port dictated by (policy) for
   // an incoming connection.  BMessages received from the connection will be forwarded to (target).
   // If (startImmediately) is set to false, then the accept thread won't be started
   // until Start() is called.   
   // (policy) becomes the property of this SHDirectConnection object.
   
   virtual ~SHDirectConnection();
   // Closes any existing connection and destroys the object.

   ////////////////////////////////////
   // SHDirectConnection's new members
   ////////////////////////////////////
   
   bool Start();
   // If you specified (startImmediately == false) for this SHDirectConnection,
   // then you will need to call this method to get your SHDirectConnection to
   // spawn its internal threads and start doing work.
   // Returns true on success, false on failure.
   // If the SHDirectConnection is already started, this method does nothing and returns true.
   
   bool IsStarted() const;
   // Returns true iff the SHDirectConnection has been started.
   
   BMessenger GetMessenger() const;
   // Send BMessages to the BMessenger this method returns, and they will go out across the TCP connection.

   int32 GetId() const;
   // Returns a unique ID number for this SHDirectConnection object.
   
   BMessage & GetUserMessage();
   // Returns a BMessage that is held by this SHDirectConnection.
   // You can use this to store info associated with this connection, if you want.

   SHNodeSpec GetNodeSpec() const;
   // Returns the SHNodeSpec associated with this SHDirectConnection.
   // If this SHDirectConnection is the passive type, you can pass this SHNodeSpec
   // to other programs, and they can use it to connect to this SHDirectConnection.
   
   void SetTagMessage(const char * name, const BMessage & msg);   
   // This call will set the SHDirectConnection so that it adds a copy of (msg) to every
   // BMessage it forwards back to the target (as set in the constructor or by
   // SetTarget()), under the field name (name).  Call SetTagMessage with a field
   // name of NULL to disable the message tagging.

   void SetTarget(const BMessenger & target);
   // This call lets you change where the BMessages received over the TCP connection
   // by this SHDirectConnection are forwarded to.   
      
private:
   static int32 GetNextIdNumber();
   
   SHDirectConnection(const BMessenger & target, int socketfd, const SHNodeSpec & spec, bool startImmediately, int32 parentId, int32 id, int32 threadPri, int32 transmissionEncoding);

   friend class shConnection;
   friend class shNode;
   friend class SHSessionAcceptor;
      
   shConnection * _connection;
   
   BMessage _userMessage;
   bool _isStarted;
};

#ifndef __INTEL__
#pragma reset
#endif

#endif

