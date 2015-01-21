
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


#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "shPendingLink.h"
#include "SockHopInternalConstants.h"
#include <sockhop/SHDefaultAccessPolicy.h>

class shAcceptLinkPolicy : public SHDefaultAccessPolicy {
public:
   shAcceptLinkPolicy(shPendingLink * link, SHAccessPolicy & copyFrom);
   virtual ~shAcceptLinkPolicy();
   
   virtual SHNodeSpec GetListeningLocation();
   // Always a system-chosen address
   
   virtual bool OkayToAcceptConnection(SHNodeSpec & conn);
   
   virtual bool OnSessionAccepted(SHDirectConnection & conn);
   // Notifies the node of our new arrival.
   
   virtual int32 GetDefaultThreadPriority();
   
   virtual int32 GetDefaultTransmissionEncoding();
      
private:
   int32 _threadPri;
   int32 _batchEncoding;
   BMessage _connSpecMsg;
   shPendingLink * _link;
   char _password[50];
};

shAcceptLinkPolicy ::
shAcceptLinkPolicy(shPendingLink * l, SHAccessPolicy & copyFrom)
   : _link(l), _threadPri(copyFrom.GetDefaultThreadPriority()), _batchEncoding(copyFrom.GetDefaultTransmissionEncoding())
{
   // Generate a random password.
   srand(time(NULL));
   for (int i=0; i<sizeof(_password); i++) _password[i] = (rand() % 255) + 1;
   _password[sizeof(_password)-1] = '\0';
}

shAcceptLinkPolicy ::
~shAcceptLinkPolicy()
{
   // empty
}

SHNodeSpec 
shAcceptLinkPolicy ::
GetListeningLocation()
{
   BMessage pwdMsg;
   pwdMsg.AddString("password", _password);
   pwdMsg.AddPointer(SH_NAME_LINKOPID, _link);   
   return(SHNodeSpec("<shAcceptLinkPolicy>", "", 0, pwdMsg));
}

bool
shAcceptLinkPolicy ::
OkayToAcceptConnection(SHNodeSpec & conn)
{
   const char * from;
   
   if (conn.GetSpecMessage().FindString(SH_NAME_SYMLINKS, &from) == B_NO_ERROR)
   {
      char * lastSlash = strrchr(from, '/');
      if (lastSlash)
      {
         conn.SetNodeName(lastSlash+1);
         _connSpecMsg = conn.GetSpecMessage();  // make a local copy of the connect info...
         conn.GetSpecMessage().MakeEmpty();     // and delete it from the official copy so it won't look ugly later
         return(SHDefaultAccessPolicy::OkayToAcceptConnection(conn));
      }
   }
   
   return false;
}

bool
shAcceptLinkPolicy ::
OnSessionAccepted(SHDirectConnection & conn)
{
   // Send a BMessage to our node, telling him that (conn) is ready, with name taken
   // from _prevSpecMsg.
   const char * name;
   if (_connSpecMsg.FindString(SH_NAME_SYMLINKS, &name) == B_NO_ERROR)
   {
      void * opID;
      if (_connSpecMsg.FindPointer(SH_NAME_LINKOPID, &opID) == B_NO_ERROR)
      {
         conn.SetTarget(GetNodeMessenger());
         BMessage informNode(SH_INTERNAL_NEWLINK);
         informNode.AddString(SH_NAME_TO, "");
         informNode.AddString(SH_NAME_WHICHSORTER, "wildpath");
         informNode.AddString(SH_NAME_SYMLINKS, name);
         informNode.AddPointer(SH_NAME_LINKOPID, opID);
         informNode.AddInt32(SH_NAME_NEWLINKID, conn.GetId());

         if (GetNodeMessenger().SendMessage(&informNode) == B_NO_ERROR) return true;
      }
   }
   return false;   // Not properly set up, is it?
}

int32 shAcceptLinkPolicy :: GetDefaultThreadPriority()
{
   return _threadPri;
}
   
int32 shAcceptLinkPolicy :: GetDefaultTransmissionEncoding()
{
   return _batchEncoding;
}
      

shPendingLink::
shPendingLink(shNode * node, const BMessage & msg, shOperationTag * optOpTag)
  : SHSessionAcceptor(BMessenger(), _policy = new shAcceptLinkPolicy(this, node->GetPolicy()), false), 
    _node(node), _msg(msg.what), _opTag(optOpTag)
{
   _policy->_node = node;
   
   copyFieldTo(SH_NAME_ONSUCCESS, B_MESSAGE_TYPE, msg, _msg);
   copyFieldTo(SH_NAME_ONFAILURE, B_MESSAGE_TYPE, msg, _msg);
   
   if (_opTag) _opTag->IncReferenceCount();
}

shPendingLink::
~shPendingLink()
{
   if (_opTag) _opTag->DecReferenceCount(_node);
}

const BMessage &
shPendingLink::
GetMessage() const
{
   return _msg;
}

shOperationTag *
shPendingLink::
GetOpTag() const
{
   return _opTag;
}

void
shPendingLink ::
PostFailureMessage(const char * optFrom)
{
   _node->PostSystemReplyMessagesWithString(&_msg, shNode::SH_OPERATION_FAILED, optFrom);
}

void
shPendingLink ::
PostSuccessMessage(const char * optFrom)
{
   _node->PostSystemReplyMessagesWithString(&_msg, shNode::SH_OPERATION_SUCCEEDED, optFrom);
}

shNode *
shPendingLink ::
GetNode() const
{
   return _node;
}
