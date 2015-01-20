
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


#ifndef _SH_OPERATIONTAGHOLDER_H_
#define _SH_OPERATIONTAGHOLDER_H_

#include "shOperationTag.h"

// A container for shOperationTags
class shOperationTagHolder 
{
public:
  shOperationTagHolder();
  ~shOperationTagHolder();
  
  void AddOperationTag(shOperationTag * tag);
  // Adds (tag) to our list, and calls (tag)'s IncReferenceCount() method.
  
  void RemoveOperationTag(uint32 tagID, BLooper * optMessagesTo);
  // Finds the first (tag) in our list with the id (tagID),
  // DecReferenceCount()'s it, and removes the entry from the list.
    
  void RemoveAllOperationTags(BLooper * optMessagesTo);
  // Calls every owned tag's DecReferenceCount(), and
  // clears the tag list.
  // If (optmessagesTo) is non-NULL, any resulting BMessages will
  // be sent to it.
  
protected:
  BList _operationTagList;
};

#endif
