
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


#include "shPendingDownload.h"
#include "shNode.h"
#include <sockhop/SockHopFunctions.h>
#include <sockhop/SHSorter.h>
#include <sockhop/SHWorker.h>
#include <support/ClassInfo.h>

// ------------- shPendingDownloadCallback ------------------------

shPendingDownloadCallback::
shPendingDownloadCallback(const BMessage & msg)
 : _msg(msg.what), _owner(NULL)
{
   copyFieldTo(SH_NAME_ONSUCCESS, B_MESSAGE_TYPE, msg, _msg);
   copyFieldTo(SH_NAME_ONFAILURE, B_MESSAGE_TYPE, msg, _msg);
}

shPendingDownloadCallback::  
~shPendingDownloadCallback()
{
   // empty
}
   
int 
shPendingDownloadCallback::
DoCallback()
{
   PostSuccessMessage();
   return -1;
}

void
shPendingDownloadCallback::
PostSuccessMessage()
{
   SHFileSpec spec = _owner->GetFileSpec();
   spec.Strip(SHGetArchitectureBits());
   _owner->GetNode()->PostSystemReplyMessages(GetMessage(), shNode::SH_OPERATION_SUCCEEDED, &spec);
}   

void
shPendingDownloadCallback::
PostFailureMessage()
{
   SHFileSpec spec = _owner->GetFileSpec();
   spec.Strip(SHGetArchitectureBits());
   _owner->GetNode()->PostSystemReplyMessages(GetMessage(), shNode::SH_OPERATION_FAILED, &spec);
}   

shPendingDownload *
shPendingDownloadCallback::
GetOwner() const
{
   return _owner;
}

BMessage *
shPendingDownloadCallback::
GetMessage() 
{
   return &_msg;
}


// ------------- shInstantiateOnDownloadCallback ------------------------

shInstantiateOnDownloadCallback::
shInstantiateOnDownloadCallback(const BMessage & msg,
                                const BMessage & archive)
  : shPendingDownloadCallback(msg), _archive(archive)
{
   // empty
}

shInstantiateOnDownloadCallback::
~shInstantiateOnDownloadCallback()
{
   // empty
}

void
shInstantiateOnDownloadCallback::
PostSuccessMessageWithString(const char * name)
{
   GetOwner()->GetNode()->PostSystemReplyMessagesWithString(GetMessage(), shNode::SH_OPERATION_SUCCEEDED, name);
}   
   
int
shInstantiateOnDownloadCallback::
DoCallback()
{
   shPendingDownload * owner = GetOwner();
   shNode * node = owner->GetNode();

   SHDistributableObject * dobj = SHCreateDistributableObject(_archive);
   if (dobj)
   {
      SHComponent * comp = cast_as(dobj, SHComponent);
      if (comp)
      {
         SHSorter * sorter = cast_as(comp, SHSorter);
         if (sorter) 
         {
            PostSuccessMessageWithString(sorter->GetName());
            node->AddSorter(sorter);
            node->printf("shInstantiateOnDownloadCallback:  Successfully added sorter [%s]\n",sorter->GetName());
            return -1;
         }
         else
         {
            SHWorker * worker = cast_as(comp, SHWorker);
            if (worker)
            {
               PostSuccessMessageWithString(worker->GetName());
               node->AddWorker(worker);
               node->printf("shInstantiateOnDownloadCallback:  Successfully added worker [%s]\n",worker->GetName());
               return -1;
            }
            else node->printf("shInstantiateOnDownloadCallback:  Error casting (SHComponent *) to (SHSorter *) or (SHWorker *)\n");
         }
      }
      else node->printf("shInstantiateOnDownloadCallback:  Error, the SHDistributableObject wasn't an SHComponent!\n");

      SHDeleteDistributableObject(dobj);
   }
   else node->printf("shInstantiateOnDownloadCallback:  Error, couldn't reconstitute SHDistributableObject\n");
   
   // If we got here, the function failed.
   PostFailureMessage();
   
   return -1;
}
   

// ------------- shForwardOnDownloadCallback ------------------------

shForwardOnDownloadCallback::
shForwardOnDownloadCallback(const BMessage & msg, uint32 cid)
   : shPendingDownloadCallback(msg), _cid(cid)
{
   // empty
}

shForwardOnDownloadCallback::
~shForwardOnDownloadCallback()
{
   // empty
}

int   
shForwardOnDownloadCallback::
DoCallback()
{
   return _cid;
}


// ------------- shOpTagCallback ------------------------

shOpTagCallback::
shOpTagCallback(const BMessage & msg, shOperationTagHolder * holder, shOperationTag * tag)
   : shPendingDownloadCallback(msg), _tagID(tag->GetTagID()), _holder(holder)
{
   _holder->AddOperationTag(tag);  
}

shOpTagCallback::
~shOpTagCallback()
{
   // empty
}

void
shOpTagCallback::
PostFailureMessage()
{
   if (GetOwner()) _holder->RemoveOperationTag(_tagID, GetOwner()->GetNode());
}   

int   
shOpTagCallback::
DoCallback()
{
   if (GetOwner()) _holder->RemoveOperationTag(_tagID, GetOwner()->GetNode());
   return -1;
}   

// ------------- shPendingDownload ------------------------
   
shPendingDownload::
shPendingDownload(shNode * node, const SHFileSpec & fileSpec)
  : _node(node), _fileSpec(fileSpec), _modFileSpec(fileSpec)
{
   // empty 
}

shPendingDownload::
~shPendingDownload()
{
   int num = _callbackList.CountItems();
   for (int i=0; i<num; i++) delete ((shPendingDownloadCallback *)(_callbackList.ItemAt(i)));
}
   
void 
shPendingDownload::
AddCallback(shPendingDownloadCallback * cb)
{
   cb->_owner = this;
   _callbackList.AddItem(cb);
}
   
void
shPendingDownload::
DoCallbacks(BMessage * sendMsg)
{
   int num = _callbackList.CountItems();
   for (int i=0; i<num; i++) 
   {
      int nextCID = ((shPendingDownloadCallback *)(_callbackList.ItemAt(i)))->DoCallback();
      const char * to;
      if ((nextCID != -1)&&((to = GetNode()->GetChildNodeNameByCID(nextCID)) != NULL)) sendMsg->AddString(SH_NAME_TO, to);
   }
}   

void 
shPendingDownload::
PostFailures()
{
   int num = _callbackList.CountItems();
   for (int i=0; i<num; i++) ((shPendingDownloadCallback *)(_callbackList.ItemAt(i)))->PostFailureMessage();
}   

const SHFileSpec &
shPendingDownload::
GetFileSpec() const
{
   return _fileSpec;
}

SHFileSpec &
shPendingDownload::
GetModifiedFileSpec()
{
   return _modFileSpec;
}

shNode *
shPendingDownload::
GetNode() const
{
   return _node;
}
