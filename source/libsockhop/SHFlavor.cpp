
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
#include <support/ByteOrder.h>
#include <sockhop/SockHopConstants.h>
#include <sockhop/SockHopFunctions.h>
#include <sockhop/SHFlavor.h>

// Convenience function, works like strdup() only with C++ char arrays
static const char * CopyString(const char * from)
{
   if (from == NULL) return NULL;
   char * str = new char[strlen(from)+1];
   strcpy(str, from);
   return str;
}

SHFlavor :: SHFlavor()
   : _cacheName(NULL), _nativeName(NULL)
{
   // empty
}

SHFlavor :: SHFlavor(const SHFlavor & copyMe)
   : _cacheName(NULL), _nativeName(NULL)
{
   *this = copyMe;
}

SHFlavor :: SHFlavor(const char * cacheName)
   : _cacheName(CopyString(cacheName)), _nativeName(CopyString(cacheName)), _arch(SH_ARCH_ANY), _isAddOn(false)
{
    StudyFile();
}

SHFlavor :: SHFlavor(const char * cacheName, uint32 arch, bool isAddOn)
   : _cacheName(CopyString(cacheName)), _nativeName(CopyString(cacheName)), _arch(arch), _isAddOn(isAddOn)
{
    StudyFile();
}

SHFlavor :: SHFlavor(const char * cacheName, const char * nativeName, uint32 arch, bool isAddOn)
   : _cacheName(CopyString(cacheName)), _nativeName(CopyString(nativeName)), _arch(arch), _isAddOn(isAddOn)
{
    StudyFile();
}

SHFlavor :: SHFlavor(const char * cacheName, uint32 arch, uint64 fileSize, uint32 creationTime, bool isAddOn)
   : _cacheName(CopyString(cacheName)), _nativeName(CopyString(cacheName)), _arch(arch), _size(fileSize), _modTime(creationTime), _isAddOn(isAddOn)
{
   // empty
}

SHFlavor :: SHFlavor(const char * cacheName, const char * nativeName, uint32 arch, uint64 fileSize, uint32 creationTime, bool isAddOn)   
   : _cacheName(CopyString(cacheName)), _nativeName(CopyString(nativeName)), _arch(arch), _size(fileSize), _modTime(creationTime), _isAddOn(isAddOn)
{
   // empty
}

SHFlavor :: ~SHFlavor()
{
   delete [] _cacheName;
   delete [] _nativeName;
}

// Reads the file and sets the _modTime and _size values from the suggested-file-name.  
// If the file can't be examined, then this object will be rendered invalid by deleting both file names!
void SHFlavor :: StudyFile()
{
   BEntry entry(GetSuggestedName(), true);

   if ((entry.InitCheck()                 != B_NO_ERROR) ||
       (entry.Exists()                    != true)       ||
       (entry.GetSize((off_t *)&_size)    != B_NO_ERROR) ||
       (entry.GetModificationTime(&_modTime) != B_NO_ERROR))
   {
      // oops, file data could not be obtained!  Lobotomize the SHFlavor!
      delete [] _cacheName;
      _cacheName = NULL;
         
      delete [] _nativeName;
      _nativeName = NULL;
   }
}

SHFlavor & SHFlavor :: operator =(const SHFlavor & copyMe)
{
   delete [] _cacheName;
   _cacheName = CopyString(copyMe._cacheName);
   
   delete [] _nativeName;
   _nativeName = CopyString(copyMe._nativeName);
   
   _arch = copyMe._arch;
   _size = copyMe._size;
   _modTime = copyMe._modTime;
   _isAddOn = copyMe._isAddOn;
      
   return *this;
}

bool SHFlavor :: operator ==(const SHFlavor & compareMe) const
{
   // If one object is valid and the other isn't, then they can't be equal
   if ((InitCheck() == B_NO_ERROR) != (compareMe.InitCheck() == B_NO_ERROR)) return false;
   
   // If both objects are invalid, then they are always equal
   if ((InitCheck() != B_NO_ERROR) && (compareMe.InitCheck() != B_NO_ERROR)) return true;
   
   // normal case - both objects valid (ignoring isAddOn flag)
   return ((_arch == compareMe._arch) &&
           (_size == compareMe._size) &&
           (_modTime == compareMe._modTime) &&
           (strcmp(_cacheName,  compareMe._cacheName)  == 0) &&
           (strcmp(_nativeName, compareMe._nativeName) == 0));
}

bool SHFlavor :: operator !=(const SHFlavor & compareMe) const
{
   return(!(*this == compareMe));
}

status_t SHFlavor :: Flatten(void * b, ssize_t numBytes) const
{
   int indx = 0;
   char * buf = (char *) b;
   
   if (InitCheck() != B_NO_ERROR) return B_ERROR;
   if (numBytes < FlattenedSize()) return B_ERROR;

   // _isAddOn
   memcpy(&buf[indx], &_isAddOn, sizeof(_isAddOn));
   indx += sizeof(_isAddOn);

   // _arch
   uint32 arch = B_HOST_TO_BENDIAN_INT32(_arch);
   memcpy(&buf[indx], &arch, sizeof(arch));
   indx += sizeof(arch);

   // _size
   uint64 size = B_HOST_TO_BENDIAN_INT64(_size);
   memcpy(&buf[indx], &size, sizeof(size));
   indx += sizeof(size);

   // _modTime
   uint32 date = B_HOST_TO_BENDIAN_INT32(_modTime);
   memcpy(&buf[indx], &date, sizeof(date));
   indx += sizeof(date);

   // _cacheName
   int cacheLen = strlen(_cacheName) + 1;
   memcpy(&buf[indx], _cacheName, cacheLen);
   indx += cacheLen;         

   // _nativeName
   int nativeLen = strlen(_nativeName) + 1;
   memcpy(&buf[indx], _nativeName, nativeLen);
   indx += nativeLen;            

   return B_NO_ERROR;
}

#define bufferOK(x) (indx + x <= numBytes)
status_t SHFlavor :: Unflatten(type_code code, const void * b, ssize_t numBytes)
{
   const char * buf = (const char *) b;
   if (code != TypeCode()) return B_ERROR;      

   int indx = 0;
   status_t ret = B_ERROR;
   
   // _isAddOn
   if (bufferOK(sizeof(_isAddOn)))
   {
      memcpy(&_isAddOn, &buf[indx], sizeof(_isAddOn));
      indx += sizeof(_isAddOn);
      
      // _arch   
      if (bufferOK(sizeof(_arch)))
      {
         memcpy(&_arch, &buf[indx], sizeof(_arch));
         _arch = B_BENDIAN_TO_HOST_INT32(_arch);
         indx += sizeof(_arch);
     
         // _size
         if (bufferOK(sizeof(_size)))
         {
            memcpy(&_size, &buf[indx], sizeof(_size));
            _size = B_BENDIAN_TO_HOST_INT64(_size);
            indx += sizeof(_size);

            // _modTime
            if (bufferOK(sizeof(_modTime)))
            {
               memcpy(&_modTime, &buf[indx], sizeof(_modTime));
		       _modTime = B_BENDIAN_TO_HOST_INT32(_modTime);
               indx += sizeof(_modTime);

               // _cacheName
               delete [] _cacheName;
               _cacheName = CopyString((const char *)&buf[indx]);
	           indx += strlen(_cacheName)+1;		
	        
               // _nativeName
	           delete [] _nativeName;
	           _nativeName = CopyString((const char *)&buf[indx]);
	        
               ret = B_NO_ERROR;  // yay!
            }
         } 
      }
   }
   return ret;
}

ssize_t SHFlavor :: FlattenedSize() const
{
   if (InitCheck() != B_NO_ERROR) return 0;
   return sizeof(_isAddOn) + sizeof(_arch) + sizeof(_size) + sizeof(_modTime) + strlen(_cacheName) + 1 + strlen(_nativeName) + 1;
}

type_code SHFlavor :: TypeCode() const
{
   return SH_FLAVOR_TYPECODE;
}

bool SHFlavor :: IsFixedSize() const
{
   return false;
}

const char * SHFlavor :: GetCacheName() const
{
   return (InitCheck() == B_NO_ERROR) ? _cacheName : "<invalid SHFlavor>";  
}

const char * SHFlavor :: GetNativeName() const
{
   return (InitCheck() == B_NO_ERROR) ? _nativeName : "<invalid SHFlavor>";
}

const char * SHFlavor :: GetSuggestedName() const
{
   return (InitCheck() == B_NO_ERROR) ? (SupportsArchitecture(SHGetArchitectureBits()) ? GetNativeName() : GetCacheName()) : "<invalid SHFlavor>";
}

uint32 SHFlavor :: GetArchitectureBits() const
{
   return _arch;
}

bool SHFlavor :: SupportsArchitecture(uint32 whichArchs) const
{      
   return ((whichArchs & ~_arch) == 0);
}

bool SHFlavor :: IsAddOn() const
{
   return _isAddOn;
}

uint64 SHFlavor :: GetSize() const
{
   return _size;
}

time_t SHFlavor :: GetModificationTime() const
{
   return _modTime;
}

void SHFlavor :: PrintToStream() const
{
   printf("SHFlavor:  _arch=[%lx] _size=[%Li] _modTime=[%li] _cacheName=[%s] _nativeName=[%s]\n", _arch, _size, _modTime, _cacheName, _nativeName);
}

status_t SHFlavor :: InitCheck() const
{
   return ((_cacheName)&&(_nativeName)) ? B_NO_ERROR : B_ERROR;  // it's valid iff the filenames are set!
}

