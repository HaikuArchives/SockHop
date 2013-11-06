
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
#include "shOperationTag.h"
#include "shNode.h"

shOperationTag ::
shOperationTag(int32 tagID, const BMessage & onDeletionSend)
  : _tagID(tagID), _refCount(1), _onDeleteSend(onDeletionSend.what)
{
   copyFieldTo(SH_NAME_WHENDONE, B_MESSAGE_TYPE, onDeletionSend, _onDeleteSend);
}

shOperationTag ::
~shOperationTag()
{
    // empty
}

bool
shOperationTag ::
DecReferenceCount(BLooper * optTarget)
{
  if ((--_refCount) <= 0)
  {
     BMessage nextMsg;
     int next = 0;     
     if (optTarget) 
     {
        while(_onDeleteSend.FindMessage(SH_NAME_WHENDONE, next++, &nextMsg) == B_NO_ERROR)
        {
           if (BMessenger(optTarget).SendMessage(&nextMsg) != B_NO_ERROR)
              printf("shOperationTag::DecReferenceCount:  Error sending message!\n");
        }
     }
     delete this;
     return true;
  }
  else return false;
}                   

void
shOperationTag ::
IncReferenceCount()
{
  _refCount++;
}

uint32
shOperationTag ::
GetTagID() const
{
  return _tagID;
}          
