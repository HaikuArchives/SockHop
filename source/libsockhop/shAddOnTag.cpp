
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


#include "shAddOnTag.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <support/Locker.h>
#include <support/List.h>
#include <support/Autolock.h>
#include <support/ClassInfo.h>

#include <sockhop/SHFlavor.h>
#include <sockhop/SockHopFunctions.h>

static BLocker _addOnTableLock;
static BList   _addOnList;

// Class that controls loading and unloading of add-on files via reference counting
class shAddOn
{
public:
   // Refs or creates an add-on object for the given file, or returns NULL on failure.
   static shAddOn * GetAddOn(const char * name)
   { 
      BAutolock lock(&_addOnTableLock);

      // figure out the full path name of the add-on
      char buffer[B_PATH_NAME_LENGTH*2];
      if (name[0] != '/') 
      {
         // add-on has relative path name; concatenate it to our
         // current absolute path.  This is so that we don't have to
         // rely on the current directory being in the ADDON_PATH
         // environment variable.
         getcwd(buffer, sizeof(buffer));
         strncat(buffer, "/", sizeof(buffer));
         strncat(buffer, name, sizeof(buffer));
         name = buffer;
      }

      // First, search the table to see if we have one
      int numTags = _addOnList.CountItems();
      for (int i=0; i<numTags; i++)
      {
         shAddOn * nextTag = (shAddOn *) _addOnList.ItemAt(i);
         if (strcmp(nextTag->_fileName, name) == 0)
         {
            nextTag->_refCount++;
            return nextTag;
         }
      }
      
      image_id addOnImage = load_add_on(name);
      if (addOnImage >= 0)
      {
//printf("succesfully loaded add image from file [%s]\n",name);
         shAddOn * newShAddOn = new shAddOn(name, addOnImage);
         _addOnList.AddItem(newShAddOn);
         return newShAddOn;
      }
      return NULL;
   }

   // To be called when you are no longer using (addOnFile)'s code image.
   void Release()
   {
      BAutolock lock(&_addOnTableLock);
      if (--_refCount == 0)
      {
         (void)_addOnList.RemoveItem(this);
         delete this;
      }
   }

   // Creates an SHDistributableObject of fomr class (className) in from our add-on file
   SHDistributableObject * InstantiateObject(const char * className, const BMessage & archive) const
   {
      char mangledName[500] = "";
 
#if __INTEL__
/* R3:    sprintf(mangledName, "?Instantiate@%s@@SAPAVBArchivable@@PAVBMessage@@@Z", className);  */
/* R4: */ sprintf(mangledName, "Instantiate__%li%sP8BMessage", strlen(className), className);
#elif __POWERPC__
          sprintf(mangledName, "Instantiate__%li%sFP8BMessage", strlen(className), className);
#else
#error "Did you get BeOS to run on the Alpha, already?"
#endif

      if (*mangledName == '\0') return NULL;

      if (_image != -1)
      {
         // Search our image_id for the Instantiate method...
         void * location;
         if (get_image_symbol(_image, mangledName, B_SYMBOL_TYPE_TEXT, &location) == B_NO_ERROR)
         {
             instantiation_func f = (instantiation_func) location;
             BArchivable * ret = f((BMessage *)(&archive));
             if (ret)
             {
                SHDistributableObject * obj = cast_as(ret, SHDistributableObject);
                if (obj) 
                {  
                   //printf("shAddOn::InstantiateObject:  Created object [%s] from file [%s]!\n", className, _fileName);
                   return obj;
                }
                else 
                {
                   // Oops, it wasn't an SHDistributableObject.  Can't have random BArchivables
                   // wandering around loose, now can we?
                   printf("shAddOn::InstantiateObject:  Object wasn't an SHDistributableObject!\n");
                   delete ret;
                   ret = NULL;
                }
             }
         }
         else printf("shAddOn::InstantiateObject:  Couldn't find function [%s] in add-on file [%s].\n", mangledName, _fileName);
      }
     return NULL;
   }
  
private:
   // nobody else gets to use these; they have
   // to use the public methods listed above!
   shAddOn(const char * file, image_id id)
      : _image(id), _refCount(1)
   {
      char * temp = new char[strlen(file)+1];
      strcpy(temp, file);
      _fileName = temp;
   }
  
   ~shAddOn()
   {
//printf("Unloading add-on image for file [%s]\n", _fileName);
      if (_image != -1) unload_add_on(_image);
      delete [] _fileName;
   }
  
   int _refCount;
   const char * _fileName;
   image_id _image;   // the image id we are referencing
};


shAddOnTag * shAddOnTag :: GetTag(const SHFileSpec & spec)
{
   SHFlavor flav;
   int numFlavs = spec.CountFlavors();
   shAddOnTag * ret = new shAddOnTag;
   for (int i=0; i<numFlavs; i++)
   {
      if (spec.GetFlavorAt(i, flav) == B_NO_ERROR)
      {
         if ((flav.SupportsArchitecture(SHGetArchitectureBits())) && (flav.IsAddOn()))
         {
            shAddOn * newAddOn = shAddOn::GetAddOn(flav.GetSuggestedName());
            if (newAddOn) ret->_addOnRefs.AddItem(newAddOn);
            else
            {
               // Oops, one of our add-on files doesn't exist!  Run screaming...
               printf("shAddOnTag:Error, couldn't load add-on image from file [%s]\n", flav.GetSuggestedName());
               delete ret;
               return NULL;
            }
         }
      }
      else printf("shAddOnTag::Error getting flavor %i/%i!\n", i, numFlavs);
   }
   return ret;
}

shAddOnTag :: ~shAddOnTag()
{
   int num = _addOnRefs.CountItems();
   for (int i=0; i<num; i++) ((shAddOn *)_addOnRefs.ItemAt(i))->Release();   
}

SHDistributableObject *
shAddOnTag::
InstantiateObject(const BMessage & archive) const
{
   // Find the last classname in the (archive)...
   type_code typeCode = B_STRING_TYPE;
   int32 numEntries;
   if (archive.GetInfo("class", &typeCode, &numEntries) == B_NO_ERROR)
   {
      // Note:  for now, we only attempt to load the lowest subclass.
      const char * className;
      if (archive.FindString("class", numEntries-1, &className) == B_NO_ERROR)
      {
         // Try to instantiate in each of our add-on files, starting with the last.
         if (_addOnRefs.CountItems() > 0)
         {
            for (int i=_addOnRefs.CountItems()-1; i >= 0; i--)
            {
               shAddOn * next = (shAddOn *) _addOnRefs.ItemAt(i);
               SHDistributableObject * obj = next->InstantiateObject(className, archive);
               if (obj) return obj;
            }
         }
         else
         {
            // No add-ons specified?  Then the user must be relying on just shared libraries
            // to instantiate his object (SHWildPathSorter does this for example).  Let's give that a shot.
            if (validate_instantiation((BMessage *) &archive, className))
            {
               BArchivable * arc = instantiate_object((BMessage *)&archive);  // I'm trusting that this won't modify (archive)!
               if (arc)
               {
                  SHDistributableObject * obj = cast_as(arc, SHDistributableObject);
                  if (obj) return obj;
                      else delete arc;             
               }
            }         
         }
      }      
   }
   else printf("shAddOn::InstantiateObject:  no class entry found in archive (are you sure your Archive() called <superclass>::Archive?)\n");

   return NULL;
}
