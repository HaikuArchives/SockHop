
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


#ifndef _SHOPERATIONTAG_H_
#define _SHOPERATIONTAG_H_

#include <app/Looper.h>
#include <app/Message.h>

class shOperationTag
{
public:
   shOperationTag(int32 tagID, const BMessage & fromMsg);
   // (fromMsg) is assumed to have an entry SH_NAME_WHENDONE
   // that contains a list of BMessages to be sent.  A copy
   // of (fromMsg) will be made, and all other entries stripped out of it.
   // The SH_NAME_WHENDONE messages will be sent by DecReferenceCount()
   // when appropriate.
   // On return, the reference count is 1 (so the first
   // DecReferenceCount() with no intervening IncReferenceCount()'s
   // will delete this)

   bool DecReferenceCount(BLooper * sendMessagesTo);
   // Decrements the refCount.  If the refCount <= 0,
   // it will send off all BMessages in its BList to
   // (sendMessagesTo), delete the BList and the BMessages,
   // then delete itself.

   void IncReferenceCount();

   uint32 GetTagID() const;

private:
   ~shOperationTag();  // private -- use DecReferenceCount() to delete!  

   int32 _tagID;
   uint32 _refCount;
   BMessage _onDeleteSend;
};

#endif 