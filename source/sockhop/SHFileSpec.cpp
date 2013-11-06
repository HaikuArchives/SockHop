
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
#include <string.h>
#include <malloc.h>
#include <storage/Entry.h>
#include <sockhop/SHFileSpec.h>
#include <support/ByteOrder.h>



/////////////////////////////////////////////////////////////////////////
//
//  SHFileSpec
//
/////////////////////////////////////////////////////////////////////////

//:Describes a host File to be connected to.
SHFileSpec::
SHFileSpec()
{
   // empty
}  

SHFileSpec::
SHFileSpec(const SHFileSpec & copyMe)
{
   *this = copyMe;
}

SHFileSpec::
~SHFileSpec()
{
   MakeEmpty();
}
 
type_code
SHFileSpec::
TypeCode() const
{
   return SH_FILESPEC_TYPECODE;
}

status_t 
SHFileSpec::
Flatten(void * b, ssize_t numBytes) const
{
   char * buf = (char *)b;
   int indx = 0;

   uint32 numItems = CountFlavors();   
   numItems = B_HOST_TO_BENDIAN_INT32(numItems);
   memcpy(&buf[indx], &numItems, sizeof(numItems));
   indx += sizeof(numItems);
   
   for (int i=0; i<_flavors.CountItems(); i++)
   {
      SHFlavor * next = (SHFlavor *) _flavors.ItemAt(i);
      ssize_t nextSize = next->FlattenedSize();  
      status_t ret = next->Flatten(&buf[indx], numBytes - indx);
      if (ret != B_NO_ERROR) return ret;
      indx += nextSize;
   }
   return B_NO_ERROR;
}

SHFileSpec SHFileSpec :: operator + (const SHFileSpec & b) const 
{
   SHFileSpec ret(*this);
   ret += b;
   return ret;
}

SHFileSpec& SHFileSpec :: operator += (const SHFileSpec &b) 
{
   int numItems = b.CountFlavors();
   for (int i=0; i<numItems; i++) AddFlavor(*((SHFlavor *)b._flavors.ItemAt(i)));
   return *this;
}

SHFileSpec SHFileSpec :: operator -(const SHFileSpec &b) const 
{
   SHFileSpec ret(*this);
   ret -= b;
   return ret;
}

SHFileSpec& SHFileSpec :: operator -=(const SHFileSpec &b) 
{
   int numItems = b.CountFlavors();
   for (int i=0; i<numItems; i++) RemoveFlavor(*((SHFlavor *)b._flavors.ItemAt(i)));
   return *this;
}

// The two specs are equal if each has all the other's flavors!
bool SHFileSpec :: operator ==(const SHFileSpec & compareMe) const
{
   if (CountFlavors() != compareMe.CountFlavors()) return false;  // duh
   
   int numItems1 = CountFlavors();
   for (int i=0; i<numItems1; i++) if (compareMe.IndexOf(*((SHFlavor *)_flavors.ItemAt(i))) == -1) return false;

   int numItems2 = compareMe.CountFlavors();
   for (int j=0; j<numItems2; j++) if (IndexOf(*((SHFlavor *)compareMe._flavors.ItemAt(j))) == -1) return false;
   
   return true;
}

bool SHFileSpec :: operator !=(const SHFileSpec & compareMe) const
{
   return !(*this == compareMe);
}

bool
SHFileSpec::
IsFixedSize() const
{
   return false;
}

status_t 
SHFileSpec::
Unflatten(type_code code, const void * b, ssize_t numBytes)
{
   const char * buf = (const char *)b;   
   if (code != TypeCode()) return(B_ERROR);

   MakeEmpty();
   
   int indx = 0;
   uint32 numItems;
   if (numBytes < sizeof(numItems)) return B_ERROR;
   memcpy(&numItems, &buf[indx], sizeof(numItems));
   numItems = B_BENDIAN_TO_HOST_INT32(numItems);
   indx += sizeof(numItems);

   SHFlavor next;
   for (int i=0; i<numItems; i++)
   {
      status_t ret = next.Unflatten(SH_FLAVOR_TYPECODE, &buf[indx], numBytes - indx);
      if (ret != B_NO_ERROR) return ret;

      AddFlavor(next);

      ssize_t flattenedSize = next.FlattenedSize();
      if (numBytes < (indx + flattenedSize)) return B_ERROR;
      indx += flattenedSize;
   }
   return B_OK;
}

ssize_t
SHFileSpec::
FlattenedSize() const
{
   ssize_t size = sizeof(uint32);  // the SHFlavor item count   
   for (int i=0; i<_flavors.CountItems(); i++) size += ((SHFlavor *)_flavors.ItemAt(i))->FlattenedSize();
   return size;   
}

void
SHFileSpec::
MakeEmpty()
{
   for (int i=0; i<_flavors.CountItems(); i++) delete ((SHFlavor *)_flavors.ItemAt(i));
   _flavors.MakeEmpty();
}

SHFileSpec &
SHFileSpec ::
operator =(const SHFileSpec & copyMe)
{
   MakeEmpty();
   for (int i=0; i<copyMe._flavors.CountItems(); i++) AddFlavor(*((SHFlavor *)copyMe._flavors.ItemAt(i)));
   return(*this);  
}

status_t
SHFileSpec ::
AddFlavor(const SHFlavor & flav)
{
   if (flav.InitCheck() != B_NO_ERROR) return B_BAD_VALUE;   
   if (IndexOf(flav) >= 0) return B_NO_ERROR;  // it's okay; it's already there but we don't want duplicates
   _flavors.AddItem(new SHFlavor(flav));
   return B_NO_ERROR;
}

status_t 
SHFileSpec ::
RemoveFlavor(const SHFlavor & flavor)
{
   return (RemoveFlavorAt(IndexOf(flavor)) == B_NO_ERROR) ? B_NO_ERROR : B_NAME_NOT_FOUND;
}

status_t
SHFileSpec ::
RemoveFlavorAt(int32 idx)
{
   SHFlavor * deadFlavor = (SHFlavor *) _flavors.RemoveItem(idx);
   if (deadFlavor) 
   {
      delete deadFlavor;
      return B_NO_ERROR;
   }
   else return B_BAD_INDEX;
}

status_t
SHFileSpec ::
GetFlavorAt(uint32 i, SHFlavor & setFlav) const
{
   if ((i < 0)||(i >= _flavors.CountItems())) return B_BAD_INDEX;
   setFlav = *((SHFlavor *)_flavors.ItemAt(i));
   return B_NO_ERROR;
}

int32
SHFileSpec :: 
IndexOf(const SHFlavor & flavor) const
{
   int num = CountFlavors();
   for (int i = 0; i < num; i++) if (*((SHFlavor *)_flavors.ItemAt(i)) == flavor) return i;
   return -1;
}

void
SHFileSpec ::
PrintToStream() const
{
   printf("SHFileSpec:\n");
   for (int i=0; i<_flavors.CountItems(); i++)
   {
      printf("Flavor %i:\n", i);
      ((SHFlavor *)_flavors.ItemAt(i))->PrintToStream();
   }
}

// Removes all flavors from this SHFileSpec except for the ones that
// matches the given architecture bitchord.  If no flavors match the architecture
// bitchord, this SHFileSpec becomes empty.
void 
SHFileSpec ::
Strip(uint32 whichArchs)
{
   int num = _flavors.CountItems();
   for (int i=num-1; i>=0; i--)
   {
      SHFlavor * next = ((SHFlavor *)_flavors.ItemAt(i));
      if (next->SupportsArchitecture(whichArchs) == false) RemoveFlavorAt(i);
   }
}


uint32
SHFileSpec :: CountFlavors() const
{
   return _flavors.CountItems();
}


