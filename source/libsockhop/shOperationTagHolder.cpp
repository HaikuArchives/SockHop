
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
#include "shOperationTagHolder.h"

shOperationTagHolder::
shOperationTagHolder()
{
   // empty
}

shOperationTagHolder::
~shOperationTagHolder()
{
   int num = _operationTagList.CountItems();
   if (num > 0) 
   {
       printf("WARNING:  shOperationTagHolder: there are still %i tags in the tagList, dropping them!\n", num);
       RemoveAllOperationTags(NULL);
   }
}

void
shOperationTagHolder::
AddOperationTag(shOperationTag * tag)
{
   tag->IncReferenceCount();
   _operationTagList.AddItem(tag);
}

void
shOperationTagHolder::
RemoveAllOperationTags(BLooper * optMessageTarget)
{
   int num = _operationTagList.CountItems();
   for (int i=0; i<num; i++) (void)((shOperationTag *)_operationTagList.ItemAt(i))->DecReferenceCount(optMessageTarget);
   _operationTagList.MakeEmpty();
}

void
shOperationTagHolder::
RemoveOperationTag(uint32 tagID, BLooper * optMessageTarget)
{
   int num = _operationTagList.CountItems();
   for (int i=0; i<num; i++)
   {
      shOperationTag * nextTag = (shOperationTag *) _operationTagList.ItemAt(i);

      if (nextTag->GetTagID() == tagID)
      {
         (void) nextTag->DecReferenceCount(optMessageTarget);
         (void) _operationTagList.RemoveItem(i);
         break;
      }      
   }
}
