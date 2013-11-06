
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


#ifndef _SHNODESPEC_H
#define _SHNODESPEC_H

#include <app/Message.h>
#include <support/List.h>
#include <support/Flattenable.h>
#include <sockhop/SockHopConstants.h>

///////////////////////////////////////////////////////////////////////////
//
// SHNodeSpec
//
// A convenient data container object, that describes how to connect to a
// given SockHop server or other SockHop network entity.  Contains fields 
// describing a potential node's name, the name of the host computer it 
// will run on, the TCP port number to connect to, and a BMessage with 
// customizable information to give to the server.
//
////////////////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHNodeSpec;
#else
#pragma export on
#endif 

class SHNodeSpec : public BFlattenable
{
public:
  /////////////////
  // Lifecycle
  /////////////////

  SHNodeSpec();  
  // Default constructor, creates object with all fields empty

  SHNodeSpec(const SHNodeSpec & copyMe);
  // Copy constructor - creates a clone of (copyMe)
    
  SHNodeSpec(const char * nodeName);
  SHNodeSpec(const char * nodeName, const BMessage & specMsg);
  // Constructors that specify a child node that runs in the same process space as its parent
  
  SHNodeSpec(const char * nodeName, const char * hostName);
  SHNodeSpec(const char * nodeName, const char * hostName, const BMessage & specMsg);
  SHNodeSpec(const char * nodeName, const char * hostName, int portNum);
  SHNodeSpec(const char * nodeName, const char * hostName, int portNum, const BMessage & specMsg);
  // Constructors that specify a child node that runs remotely, via TCP, on a specified host
      
  virtual ~SHNodeSpec();

  ////////////////////////
  // Overloaded operators
  ////////////////////////
  
  SHNodeSpec & operator =(const SHNodeSpec & copyMe);
  // Assignment operator
    
  bool operator ==(const SHNodeSpec & compareMe) const;  
  // Comparison operator
  // Note:  The (specMsg) fields of the two objects are NOT examined as part of this comparison!
  
  bool operator !=(const SHNodeSpec & compareMe) const;
  // Comparison operator
  // Note:  The (specMsg) fields of the two objects are NOT examined as part of this comparison!
  
  virtual bool IsNodeNameValid() const;
  // Returns true only if the node name field of this object is valid.  Validity means:  
  // the node name may not be equal to "", "." or "..", and it may not have a '/' character in it, 
  // nor may it be longer than SH_MAX_NODENAME_LENGTH bytes.
  
  //////////////////////////
  // BFlattenable interface
  //////////////////////////
  
  virtual status_t Flatten(void * buf, ssize_t numBytes) const;
  virtual status_t Unflatten(type_code code, const void * buf, ssize_t numBytes);
  virtual ssize_t FlattenedSize() const;
  virtual type_code TypeCode() const;
  virtual bool IsFixedSize() const;

  ////////////////////////////
  // SHNodeSpec's new members 
  ////////////////////////////
     
  void SetHostName(const char * newHostName);
  // Sets the hostname field of this object.  An internal copy of the string is created
  // and retained.
  
  const char * GetHostName() const;  
  // Returns a pointer to the host name string held by this SHNodeSpec.
  // String will be valid till this object is destroyed, or SetHostName() is called on it.
  // Guaranteed to never return NULL (returns "" if unset)
   
  void SetNodeName(const char * newNodeName);
  // Sets the node name field of this object.  An internal copy of the string is created
  // and retained.
  
  const char * GetNodeName() const;  
  // Returns a pointer to the node name string held by this SHNodeSpec.
  // String will be valid till this object is destroyed, or SetNodeName() is called on it.
  // Guaranteed to never return NULL (returns "" if unset)
   
  void SetPortNumber(uint32 newPortNum);
  // Sets the port number field of this SHNodeSpec to (newPortNum).
  
  uint32 GetPortNumber() const;  
  // Returns the port number of this SHNodeSpec.  By default, this field
  // is set to the value SH_DEFAULT_PORT, as defined in <sockhop/SockHopConstants.h>

  BMessage & GetSpecMessage();
  // Returns a reference to the held BMessage.  User code may read or
  // write this message any way it likes.
  // This BMessage may be used by the SHAccessPolicy objects running
  // on the SockHop servers to decide whether or not your connection
  // is worthy of continuing.  For example, the SHDefaultAccessPolicy
  // object often looks up the string field "password" in this BMessage.

  const BMessage & GetConstSpecMessage() const;
  // Same as GetSpecMessage(), but returned as a const reference,
  // so you can't write to it.
  
  void PrintToStream() const;
  // Dump the state of the SHNodeSpec to stdout

private:
  void MakeFlatMessage(BMessage & msg) const;
  void clear(bool freeMem);
    
  char * _nodeName;  
  char * _hostName;  
  uint32 _portNum;   

  BMessage _specMessage;
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
