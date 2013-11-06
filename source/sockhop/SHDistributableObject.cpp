
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


#include <stdio.h>
#include <sockhop/SockHopConstants.h>
#include <sockhop/SHDistributableObject.h>
#include "shNode.h"

// Synchronized, monotonically increasing ID counter for SHWorkers
static int32 GetNextObjectId()
{
   static BLocker guardWorkerIdCounter;
   static int32 workerIdCounter = 0;
   return(workerIdCounter++);
}


SHDistributableObject::
SHDistributableObject()
   : _id(GetNextObjectId())
{
   // empty
}

SHDistributableObject::
SHDistributableObject(BMessage * archive)
   : BArchivable(archive), _id(GetNextObjectId())
{
   // empty
}

SHDistributableObject::
~SHDistributableObject()
{
   if (_addOnTags.CountItems() > 0)
      printf("SHDistributableObject::~SHDistributableObject:  WARNING, leaking add-on tags!  To avoid this, use SHDeleteDistributableObject() to delete objects you created with SHCreateDistributableObject()\n");
}

status_t
SHDistributableObject::
Archive(BMessage * archive, bool deep) const
{
   status_t ret;
   SHFileSpec spec;

   if ((ret = BArchivable::Archive(archive, deep))        != B_NO_ERROR) return ret;   
   if ((ret = GetAddOnSpec(spec))                         != B_NO_ERROR) return ret;
   if ((ret = archive->AddFlat(SH_NAME_ADDONSPEC, &spec)) != B_NO_ERROR) return ret;
   return B_NO_ERROR;
}

status_t
SHDistributableObject ::
GetAddOnSpec(SHFileSpec & writeTo) const
{   
   SHFlavor x86("/boot/home/config/lib/libsockhop.so.x86", "/boot/home/config/lib/libsockhop.so", SH_ARCH_BEOS_X86, false);
   SHFlavor ppc("/boot/home/config/lib/libsockhop.so.ppc", "/boot/home/config/lib/libsockhop.so", SH_ARCH_BEOS_PPC, false);

   // Note:  I'm ignoring any errors here since being able to auto-upgrade 
   //        libsockhop.so isn't really a critical function.  Obviously if there
   //        is a SockHop node receiving our data, then it must have some version
   //        of libsockhop.so available to it.  I would print a warning message,
   //        except that if you are working on a single-architecture setup, you may
   //        not have multiple versions of libsockhop.so installed on purpose, and
   //        so the warnings would get annoying.
   (void) writeTo.AddFlavor(x86);
   (void) writeTo.AddFlavor(ppc);

   return B_NO_ERROR;
}

int32
SHDistributableObject::
GetId() const
{
   return _id;
}

/* FBC */
void SHDistributableObject::_SHDistributableObject1() {}
void SHDistributableObject::_SHDistributableObject2() {}
void SHDistributableObject::_SHDistributableObject3() {}
void SHDistributableObject::_SHDistributableObject4() {}
