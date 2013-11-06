
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


#ifndef _SHDistributableObject_H_
#define _SHDistributableObject_H_

#include <support/Archivable.h>
#include <sockhop/SHFileSpec.h>

////////////////////////////////////////////
//
// SHDistributableObject
//
// Abstract base class that represents any C++ object
// that SockHop can automatically archive and send
// across the tree in a cross-platform compatible manner.
//
////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHDistributableObject;
#else
#pragma export on
#endif 

class SHDistributableObject : public BArchivable
{
public:
   SHDistributableObject();
   // Default constructor
   
   SHDistributableObject(BMessage * archive);
   // BArchivable rehydration constructor
   
   virtual ~SHDistributableObject();

   virtual status_t GetAddOnSpec(SHFileSpec & writeTo) const;
   // This method should be implemented to add to (writeTo) specifications
   // for all the files that must be available on the local disk in order 
   // for the SHDistributableObject to be successfully instantiated.
   // Derived classes should call their parent's implementation of this
   // method from somewhere within their own implementation!
   //
   // (SHDistributableObject's implementation adds the following flavors:
   //  X86: cache=/boot/home/config/lib/libsockhop.so.x86 native=/boot/home/config/lib/libsockhop.so
   //  PPC: cache=/boot/home/config/lib/libsockhop.so.ppc native=/boot/home/config/lib/libsockhop.so
   // Which means that if you are the root node of a multi-architecture
   // SockHop network, then you should have the native version of libsockhop.so   
   // installed as "libsockhop.so" in your /boot/home/config/lib directory,
   // and the non-native versions in that same directory with their 3-letter
   // extensions (".x86" or ".ppc") appended.  
   //
   // Note also that the effect of specifying these files is that every
   // time your SockHop program runs, all child nodes will check their
   // versions of libsockhop.so and download the new version if it has
   // changed on the root node!  Unfortunately, the upgrade won't take
   // effect until the next time your program runs (since the old version
   // was already in memory when the download took place).  If you don't
   // want this auto-update of libsockhop.so to occur, then your derived
   // classes' GetAddOnSpec() methods should not call back to 
   // SHDistributableObject::GetAddOnSpec().
   
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of all subclass's Archive() methods

   int32 GetId() const;
   // An ID number for this object, mostly for SockHop's internal use.
   // Note that this Id number is not propagated with the object as
   // it travels from node to node!  It is reassigned every time the
   // object is unarchived, and is only guaranteed to be unique with
   // respect to SHDistributableObjects that live on the same node as
   // this object currently does.
   
private:   
   friend SHDistributableObject * SHCreateDistributableObject(const BMessage & archive);
   friend void SHDeleteDistributableObject(SHDistributableObject *);
   
   BList _addOnTags;
   int32 _id;
      
   /* FBC */
   virtual void _SHDistributableObject1();
   virtual void _SHDistributableObject2();
   virtual void _SHDistributableObject3();
   virtual void _SHDistributableObject4();
   uint32 _SHDistributableObjectData[8];
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
