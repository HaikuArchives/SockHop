
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


#ifndef SHACCESSPOLICY_H
#define SHACCESSPOLICY_H

#include <sockhop/SHComponent.h>
#include <sockhop/SHNodeSpec.h>

// This class defines an interface that sets per-server
// policies on what the code running on the SockHopServer is
// and isn't allowed to do.
// Override methods in this class with your own to customize
// your own security mechanism.

class SHDirectConnection;

#ifdef __INTEL__
_EXPORT class SHAccessPolicy;
#else
#pragma export on
#endif 

class SHAccessPolicy : public SHComponent
{
public:
   ////////////////////
   // Lifecycle
   ////////////////////

   SHAccessPolicy();
   ~SHAccessPolicy();

   /////////////////////////
   // SHComponent interface
   /////////////////////////

   ///////////////////////////////////
   // SHDistributableObject interface
   ///////////////////////////////////

   ////////////////////////////
   // BArchivable interface
   ////////////////////////////
   
   SHAccessPolicy(BMessage * archive);
   // Rehydration constructor
   
   virtual status_t Archive(BMessage * archive, bool deep = true) const;
   // Dehydration method

   ////////////////////////////////
   // SHAccessPolicy's new members
   //////////////////////////////// 
     
   virtual void OnServerStartup();
   // Called when the server starts.  Default version does nothing.
   
   virtual void OnServerShutdown();
   // Called when the server exits.  Default version does nothing.
   // (Note: not currently called when shutdown is due to a CTRL-C)
   
   virtual SHNodeSpec GetListeningLocation() = 0;
   // Should return an SHNodeSpec with its port number field set to the port
   // number the owning object should listen on for incoming connections.
   // If it returns an SHNodeSpec with a port number zero, the system will
   // choose a port number.
   // The other fields in the SHNodeSpec aren't used by SockHop, although
   // your code can fill them for its own use, if it wants.
   
   virtual bool OkayToAcceptConnection(SHNodeSpec & connectingSpec) = 0;
   // Should return true iff it's okay to accept a connection from the given host.
   // (connectingSpec) contains information about the potential acceptee,
   // and (conn) can be used to send a BMessage to the connectee.
   // You can also modify (connectingSpec), if you feel the need to.
   
   virtual bool OnSessionAccepted(SHDirectConnection & connection);
   // Called when an SHDirectConnection has been created for a new connection.
   // Default implementation calls connection.Start() so that the
   // SHDirectConnection can begin operation.
   // Should return true if the SHDirectConnection is to continue, or
   // false if there was an error and the SHDirectConnection should be aborted.
   
   virtual void OnNodeStartup();
   // Called when the node starts running.  Default version does nothing.
   
   virtual void OnNodeShutdown();
   // Called when the node quits running.  Default version does nothing.
   
   virtual bool OkayToWriteFile(const char * fileName) = 0;
   // Should return true iff it's okay to write to the given file.
   
   virtual bool OkayToReadFile(const char * fileName) = 0;
   // Should return true iff it's okay to read from the given file.

   virtual bool OkayToInstantiateObject(const BMessage & archive) = 0;
   // Should return true iff it's okay to instantiate an object using
   // an add-on file, as specified in (archive).

   virtual SHAccessPolicy * MakeNodePolicy();
   // This method is called whenever a new node is created.  This method should 
   // return a new SHAccessPolicy object for the new node to use, or return NULL 
   // if you want the node to use a clone of this SHAccessPolicy.
   // This default implementation returns NULL.
   // When returning an SHAccessPolicy, make sure it was allocated either with
   // SHCreateDistributableObject(), or the new operator.
   
   virtual int GetDefaultDebugLevel();
   // Should return the initial debugging level for the server or node.
   // Currently, 0 means no debug printing, >0 means debug printing; more
   // levels may be implemented in the future.
   // The default implementation of this method returns 0.

   virtual int32 GetDefaultThreadPriority();
   // Returns the priority that spawned TCP send and receive threads should run at.
   // The default implementation returns B_NORMAL_PRIORITY.

   virtual int32 GetDefaultTransmissionEncoding();
   // Returns the ID token of the batch encoding method that should be used
   // when sending BMessage groups over the TCP link.  The default implementation
   // returns SH_ENCODING_NONE.
      
private:
   /* FBC */
   virtual void _SHAccessPolicy1();
   virtual void _SHAccessPolicy2();
   virtual void _SHAccessPolicy3();
   virtual void _SHAccessPolicy4();
   virtual void _SHAccessPolicy5();
   virtual void _SHAccessPolicy6();
   virtual void _SHAccessPolicy7();
   virtual void _SHAccessPolicy8();
   uint32 _SHAccessPolicyData[64];   
};

#ifndef __INTEL__
#pragma reset
#endif

#endif