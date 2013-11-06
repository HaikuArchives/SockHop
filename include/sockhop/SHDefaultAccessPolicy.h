
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


#ifndef SHDEFAULTACCESSPOLICY_H
#define SHDEFAULTACCESSPOLICY_H

#include <sockhop/SockHopConstants.h>
#include <sockhop/SHAccessPolicy.h>

// This class is used by the SockHopServer to set
// policy if no policy add-on is explicitely specified on 
// the command line.  It implements a very liberal
// security policy, with either simple password-checking (by
// matching a string in the "password" field of BMessages, or
// no access control at all.  You may subclass it if you wish;
// it's easier than subclassing SHAccessPolicy directly
// because you won't have to define all the methods.
//
// With the default node policy in use, the server executable
// (libsockhop.so) will understand any of the following parameters on the
// command line (examples below):
//
// port=2958   // causes server to listen on port 2958
// debug=1     // sets debug printing level to 1
// password=topsecret  // Causes the server to only accept connections who have this string in their SHNodeSpec's BMessage (e.g. fieldname "password", value="topsecret")
// priority=10  // causes send & receive threads to be spawned at priority 10

#ifdef __INTEL__
_EXPORT class SHDefaultAccessPolicy;
#else
#pragma export on
#endif 

class SHDefaultAccessPolicy : public SHAccessPolicy
{
public:
   ///////////////
   // Lifecycle
   ///////////////
   
   SHDefaultAccessPolicy(int port = SH_DEFAULT_PORT, int debugLevel = 0, const char * password = NULL, int32 threadPri = B_NORMAL_PRIORITY, int32 batchEncoding = SH_ENCODING_NONE);
   // Specify port, debug level, password, thread priority, and encoding method explicitely.
   
   virtual ~SHDefaultAccessPolicy();
 
   ////////////////////////////////////
   // SHAccessPolicy interface
   ////////////////////////////////////
   
   virtual SHNodeSpec GetListeningLocation();
   // Returns an SHNodeSpec with the port number set to the port number specified
   // in the constructor.  If (debugLevel) or (password) were specified in the
   // constructor, the field "password" will be added to the SpecMessage of the returned SHNodeSpec.
   
   virtual bool OkayToAcceptConnection(SHNodeSpec & connectingSpec);
   // If no password was specified for this object, then this method always returns true.
   // If a password was specified, then this method will only return true if the client's
   // (connectingSpec) has a field named "password" in its SpecMessage, and that field's
   // contents is a string that matches our password exactly.
   
   virtual bool OkayToWriteFile(const char * fileName);
   // Always returns true.
   
   virtual bool OkayToReadFile(const char * fileName);
   // Always returns true.

   virtual bool OkayToInstantiateObject(const BMessage & archive);
   // Always returns true.
   
   virtual int GetDefaultDebugLevel();
   // Returns the debug level specified in the constructor
   
   virtual int32 GetDefaultThreadPriority();
   // Returns the thread priority to be used for spawned threads, as specified in the constructor

   virtual int32 GetDefaultTransmissionEncoding();
   // Returns the encoding method for transmitted BMessages, as specified in the constructor
   
   virtual void OnServerStartup();
   // Prints out a warning message if we are running without any password required.
    
   //////////////////////////////
   // SHComponent interface
   //////////////////////////////
   
   virtual const char * GetName() const;
   // Returns "default"

   ////////////////////////////////////
   // SHDistributableObject interface
   ////////////////////////////////////
   
   ////////////////////////////  
   // BArchivable interface   
   ////////////////////////////

   SHDefaultAccessPolicy(BMessage * archive);
   // Looks for *string* fields named "port", "password" and "debug" in (archive).
   // Yes, even "port" and "debug" should be stored in (archive) as string
   // fields, and they will be parsed back into integers.  This is so that
   // command-line arguments for the SockHop server can be passed in without
   // special knowledge of their meaning...
      
   static BArchivable * Instantiate(BMessage * archive);
   // Needed to be an instantiatable BArchivable object.
   
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.

private:
   int32 _port;
   int32 _debugLevel;
   const char * _password;
   int32 _threadPri;
   int32 _batchEncoding;
   
   /* FBC */
   virtual void _SHDefaultAccessPolicy1();
   virtual void _SHDefaultAccessPolicy2();
   virtual void _SHDefaultAccessPolicy3();
   virtual void _SHDefaultAccessPolicy4();
   uint32 _SHDefaultAccessPolicyData[8];   
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
