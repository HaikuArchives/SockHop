
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


#include <sockhop/SHSorter.h>
#include "shNode.h"

SHSorter::
SHSorter()
{
   // empty
}

SHSorter::
SHSorter(BMessage * archive)
   : SHComponent(archive)
{
   // empty
}

SHSorter::
~SHSorter()
{
   // empty
}

void
SHSorter::
BeforeMessageRelay(BMessage &)
{
   // empty
}

void
SHSorter::
BeforeLocalMessageDistribute(BMessage &)
{
   // empty
}

status_t
SHSorter::
Archive(BMessage * archive, bool deep) const
{
   return(SHComponent::Archive(archive, deep));
}

bool
SHSorter ::
DoesMessageGoToWorker(BMessage &, const char *)
{
   return true;
}

/* FBC */
void SHSorter::_SHSorter1() {}
void SHSorter::_SHSorter2() {}
void SHSorter::_SHSorter3() {}
void SHSorter::_SHSorter4() {}