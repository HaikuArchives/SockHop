
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


#ifndef _SHFILESPEC_H
#define _SHFILESPEC_H

#include <support/List.h>
#include <support/Flattenable.h>
#include <sockhop/SockHopConstants.h>
#include <sockhop/SHFlavor.h>

/////////////////////////////////////////////////////////
//
// SHFileSpec
//
// An object which describes a file, a set of files,
// or a set of sets of files.  That is, a single SHFileSpec
// can specify a different set of files for each machine
// architecture (PPC, x86, etc) that SockHop supports.
//
// Generally used to specify what files need to be downloaded
// from the root server in order to run a SockHop add-on.
//
/////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHFileSpec;
#else
#pragma export on
#endif 

class SHFileSpec : public BFlattenable
{
public:
  /////////////
  // Lifecycle
  /////////////
  
  SHFileSpec();  
  // Default constructor, creates object with no flavors (i.e. files) specified
  // Use AddFlavor() to add flavors to this object.

  SHFileSpec(const SHFileSpec & copyMe);
  // Copy constructor
    
  virtual ~SHFileSpec();
  
  ////////////////////////
  // Overloaded operators
  ////////////////////////

  SHFileSpec & operator =(const SHFileSpec & copyMe);
  // Assignment operator.  

  SHFileSpec operator + (const SHFileSpec & that) const;
  // Returns an SHFileSpec that has all the SHFlavors of (this) and (that), 
  // discarding duplicate SHFlavors.  
  
  SHFileSpec& operator += (const SHFileSpec & that);
  // Adds (that)'s SHFlavors to (this), discarding duplicates.

  SHFileSpec operator - (const SHFileSpec & that) const;
  // Returns an SHFileSpec that has all the SHFlavors of (this) that are NOT in (that).

  SHFileSpec& operator -=(const SHFileSpec & that);
  // Removes all of (that)'s SHFlavors from (this).

  bool operator ==(const SHFileSpec & compareMe) const;
  // Returns true if (compareMe) has the same set of SHFlavors as (this).
  // SHFlavor ordering is not considered.
  
  bool operator !=(const SHFileSpec & compareMe) const;
  // Returns the opposite of what == returns.

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
      
  status_t AddFlavor(const SHFlavor & flavor);
  // Adds (flavor) to this object.  Returns B_NO_ERROR if the flavor was
  // added sucessfully, or B_BAD_VALUE if (flavor) was invalid.
  // Note that if (flavor) is identical to an SHFlavor already listed in
  // this SHFileSpec, then it will not be added again (but B_NO_ERROR will
  // be returned anyway).  This is to disallow duplicate flavors.

  status_t RemoveFlavor(const SHFlavor & flavor);
  // Removes the SHFlavor equal to (flavor)  (As determined by SHFlavor's == operator).
  // Returns B_NO_ERROR if a flavor was removed, B_NAME_NOT_FOUND if none was found to match.

  status_t RemoveFlavorAt(int32 index);
  // Removes the SHFlavor at position (index).
  // Returns B_NO_ERROR if a flavor was removed, B_BAD_INDEX if the index was invalid.
    
  status_t GetFlavorAt(uint32 index, SHFlavor & setFlavor) const;
  // Returns the flavor at index (i).  Returns B_BAD_INDEX if no flavor exists at that position.
  // On success, the flavor is copied into (setFlavor) and B_NO_ERROR is returned.
  // On failure, some other return value is returned.

  int32 IndexOf(const SHFlavor & flavor) const;
  // Returns the index of (flavor) in our list, or -1 if (flavor) doesn't exist
  // in our list.
  
  void MakeEmpty();
  // Removes all SHFlavors from this object.
    
  void Strip(uint32 whichArchs);
  // Removes all flavors from this SHFileSpec except for the ones that
  // matches the given architecture bitchord.  If no flavors match the architecture
  // bitchord, the SHFileSpec becomes empty.
 
  void PrintToStream() const;
  // Dumps the state of the SHFileSpec to stdout

  uint32 CountFlavors() const;
  // Returns the number of flavors in this SHFileSpec.
  
private:

  BList _flavors;        // List of SHFlavors owned by this object
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
