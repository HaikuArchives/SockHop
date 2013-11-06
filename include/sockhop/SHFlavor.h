
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


#ifndef _SHFLAVOR_H
#define _SHFLAVOR_H

#include <support/Flattenable.h>
#include <sockhop/SockHopConstants.h>

/////////////////////////////////////////////////////////////////////////
//
//  SHFlavor - Describes one flavor of a file.  Multiple SHFlavor objects can be
//             added to SHFileSpecs to describe multiple architecture-specific
//             versions of a given file.  For example, a binary might have
//             a PowerPC flavor and an X86 flavor.
//
/////////////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHFlavor;
#else
#pragma export on
#endif

class SHFlavor : public BFlattenable
{
public:
   //////////////////////////////
   // Lifecycle
   //////////////////////////////
   
   SHFlavor();
   // Creates an invalid SHFlavor object.  This object shouldn't be used for anything
   // except to copy data into with the '=' operator.

   SHFlavor(const SHFlavor & copyMe);
   // Copy constructor
   
   SHFlavor(const char * cacheName);
   // Creates an SHFlavor with cacheName and nativeName equal to (cacheName), architecture bits
   // equal to SH_ARCH_ANY, and fills in the fileSize and creationTime fields by examining the file
   // as it exists on disk.  This constructor is good to use with architecture-neutral files, such
   // as data files.  
   // It is assumed the file is not an add-on file, since add-ons contain processor-specific
   // executable data.
   // NOTE:  Be sure to call InitCheck() after calling this constructor, as the object won't be
   //        valid unless the file was found and readable!
   
   SHFlavor(const char * cacheName, uint32 archs, bool isAddOn);
   // As above, only here you can specify the architecture bits.  Use this constructor for files
   // that are architecture-specific (e.g. executables)
   // (isAddOn) should be set true iff this file is an add-on file that should be loaded with
   // load_add_on() before its owner is instantiated.  (Data files should not have this flag set!)
   // NOTE:  Be sure to call InitCheck() after calling this constructor, as the object won't be
   //        valid unless the file was found and readable!
   
   SHFlavor(const char * cacheName, const char * nativeName, uint32 archs, bool isAddOn);
   // As above, only here you can specify a (nativeName) as well.  On construction, the (cacheName)
   // file will be looked for first, and if it can't be found, the (nativeName) file will be examined.
   // During caching, the file will be cached as (nativeName) if the caching machine is included in
   // the (archs) bitchord, otherwise it will be cached as (cacheName).
   // NOTE:  Be sure to call InitCheck() after calling this constructor, as the object won't be
   //        valid unless at lease one of the two files specified was found and readable!
   
   SHFlavor(const char * cacheName, uint32 archs, uint64 fileSize, uint32 creationTime, bool isAddOn);      
   SHFlavor(const char * cacheName, const char * nativeName, uint32 archs, uint64 fileSize, uint32 creationTime, bool isAddOn);   
   // These two constructors operate similarly to those above, only instead of reading a file to get
   // the (fileSize) and (creationTime) attributes, they accept the values you give them.  In general
   // you should avoid using these constructors if possible, since allowing the SHFlavor constructor
   // to read the values off of the file guarantees correct settings for these fields.
   // On the plus side, you don't need to call InitCheck(), as these constructors always succeed.
   
   virtual ~SHFlavor();

   ////////////////////////
   // Overloaded operators
   ////////////////////////

   SHFlavor & operator =(const SHFlavor & copyMe);
   // Assignment operator.  

   bool operator ==(const SHFlavor & compareMe) const;
   bool operator !=(const SHFlavor & compareMe) const;
   // Comparison operators.  Two SHFlavor are equal if all their fields are equal.
   // (isAddOn) is ignored for purposes of the comparison.
   
   //////////////////////////
   // BFlattenable interface
   //////////////////////////

   virtual status_t Flatten(void * buf, ssize_t numBytes) const;
   virtual status_t Unflatten(type_code code, const void * buf, ssize_t numBytes);
   virtual ssize_t FlattenedSize() const;
   virtual type_code TypeCode() const;
   virtual bool IsFixedSize() const;

   ///////////////////////////
   // Accessors
   ///////////////////////////
      
   const char * GetCacheName() const;
   // Returns the location at which the file should be cached on machines that aren't going to actually
   // use it (e.g. on a Mac that it just caching it to transmit to its X86 children)
   // Generally the cache name should be a relative path (e.g. "add-ons/x86/MyAddOn").
   
   const char * GetNativeName() const;
   // Returns the location at which the file should be placed on machines that will use it.
   // In many cases, this will be the same as the value returned by GetCacheName(), but in
   // some situations you may need it to be placed in a special location (e.g. "/boot/home/config/lib/libstuff.so")
   
   const char * GetSuggestedName() const;
   // A convenience method that returns the location this file should be found at on the current machine.
   // Specifically, if this machine is of an architecture supported by this flavor, then this method
   // returns GetNativeName().  If not, it returns GetCacheName().
   
   uint32 GetArchitectureBits() const;
   // Returns architecture bits representing the architectures that this file is appropriate for.
   
   uint64 GetSize() const;
   // Returns the size of this file, in bytes.
   
   time_t GetModificationTime() const;
   // Returns the modification date of the file.

   bool IsAddOn() const;
   // Returns true iff this SHFlavor has its isAddOn bit set in its constructor.
   
   bool SupportsArchitecture(uint32 bits) const;
   // Returns true iff all the bits in (bits) are also in this SHFlavor's GetArchitectureBits()--
   // i.e. iff the file this represents can be used by the architecture specified by (bits).
   // Just a convenience method so I don't have to think about the bit-wise logic so often.
   
   ///////////////////////////
   // Other methods
   ///////////////////////////
   
   void PrintToStream() const;
   // Prints the state of this object to stdout.
   
   status_t InitCheck() const;
   // Returns B_NO_ERROR if this object is valid, B_ERROR if it's not.
   
private:    
   void StudyFile();         // Called by ctors; reads size & date from suggested file
   
   const char * _cacheName;  // Path to file as it will be cached
   const char * _nativeName; // Path to file as it will be stored on native machines.
   uint32 _arch;             // architecture bits specifying which platforms this file is to be used on
   uint64 _size;             // size of file, in bytes
   time_t _modTime;          // modification time of the file, in seconds since the epoch
   bool _isAddOn;            // true iff this SHFlavor represents an add-on file
};

#ifndef __INTEL__
#pragma reset
#endif

#endif

