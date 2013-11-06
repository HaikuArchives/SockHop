
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


#ifndef SHADDONTAG_H
#define SHADDONTAG_H

#include <sockhop/SHDistributableObject.h>

class shAddOnTag
{
public:
   // Call to load in the code images for all of this architecture's
   // add-on SHFlavors in (key).  Code images are guaranteed valid 
   // until all users of its tags have called Release().
   // Returns NULL on failure, or a new shAddOnTag on success.
   static shAddOnTag * GetTag(const SHFileSpec & key);

   ~shAddOnTag();
   
   SHDistributableObject * InstantiateObject(const BMessage & archive) const;

private:
   BList _addOnRefs;  // images this add-on tag is referencing  
};


/*
class shAddOnTag
{
public:
  static shAddOnTag * GetTag(const SHFileSpec & key);

  // To be called when you are no longer using (key)'s code image.
  void Release();

  // Release a whole list of (shAddOnTag *)'s
  static void ReleaseList(const BList & list);
  
  SHDistributableObject * InstantiateObject(const BMessage & archive) const;
  
private:
  // nobody else gets to use these; they have
  // to use the public methods listed above!
  shAddOnTag(const SHFileSpec & key, image_id imageid);
  ~shAddOnTag();
  
  int _refCount;
  SHFileSpec _key;   // the file that we load (stripped)
  image_id _image;   // the image id we are referencing
};
*/

#endif