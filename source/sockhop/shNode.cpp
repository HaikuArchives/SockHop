
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


#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <support/Autolock.h>
#include <app/Application.h>

#include "SockHopInternalConstants.h"
#include <sockhop/SockHopFunctions.h>
#include "shNode.h"
#include <sockhop/SHWildPathSorter.h>
#include <sockhop/SHWorker.h>
#include <sockhop/SHFileSpec.h>
#include <sockhop/SHDefaultAccessPolicy.h>
#include <sockhop/SHStringMatcher.h>
#include "shFile.h"
#include "shPendingDownload.h"
#include "shPendingLink.h"
#include "shAddOnTag.h"
#include "shCallbackConnection.h"
#include "shAcceptConnection.h"
#include "shStraightConnection.h"
#include <errno.h>

#include <stdarg.h>

#define SH_IAMROOT ((_myPath[0] == '/')&&(_myPath[1] == '\0'))


void 
shNode::
printf(const char* fmt, ...)
{
  if (_debugLevel != 0)
  {
     va_list argp;
     ::printf("[%s] ",GetNodePath());
     va_start(argp, fmt);
     vprintf(fmt, argp);
     va_end(argp);
     fflush(stdout);
  }
}          


shNode::
shNode(const BMessenger & m, int32 parentId, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, bool takeAppWithMe, SHAccessPolicy * policy)
  : shNodeMessageStream(m, parentId, id, startupMessage, spec, policy->GetDefaultThreadPriority()), _myPath(NULL), _opTagIDCounter(0), _waitingToDie(false), 
    _takeAppWithMe(takeAppWithMe), _debugLevel(policy->GetDefaultDebugLevel()), _threadPri(policy->GetDefaultThreadPriority()), _batchEncoding(policy->GetDefaultTransmissionEncoding()),
    _isParentLocal(false), _policy(policy)
{
   AddSorter(_defaultSorter = _systemSorter = new SHWildPathSorter);
   
   _policy->OnNodeStartup();
}

shNode::
~shNode()
{
   int i;

   // kill all the workers.
   // must do this before anything else, as they assume a valid shNode to send messages to, etc, when
   // they shut down.
   int numWorkers = _workers.CountItems();
   for (i = numWorkers-1; i >= 0; i--)
   {
      // first, soften them all up by sending them a B_QUIT_REQUESTED.  That way, if any one of them takes
      // a long time to quit, at least the other ones will be quitting in the meantime (i.e. they'll quit
      // in parallel, rather than in serial)
      SHWorker * worker = (SHWorker *) _workers.ItemAt(i);
      BMessage quit(B_QUIT_REQUESTED);
      (void)worker->GetMessenger().SendMessage(&quit);
   }
   for (i = numWorkers-1; i >= 0; i--) RemoveWorker(i);  // Then, we get serious.
   
   // delete all pending download requests
   int numReqs = _pendingDownloads.CountItems();
   for (i = numReqs-1; i >= 0; i--) delete ((shPendingDownload *)_pendingDownloads.ItemAt(i));

   // delete all outgoing-symlnk acceptors
   int numPendingLinks = _pendingLinks.CountItems();
   for (i = numPendingLinks-1; i >= 0; i--) delete ((shPendingLink *)_pendingLinks.ItemAt(i));
   
   // kill all current incoming lnks  (note: these are lnks TO our node.  Links FROM our node look just like children to us.)
   int numLinks = _links.CountItems();
   for (i = numLinks-1; i >=0; i--) RemoveIncomingLink(i);
     
   // kill all the children
   int numChildren = _children.CountItems();
   for (i = numChildren-1; i >=0; i--) RemoveChild(i, false);

   // delete all the sorters
   int numSorters = _sorters.CountItems();
   for (i = numSorters-1; i >=0; i--) RemoveSorter(i, true);

   printf("(Node thread %i) I am terminating%s.\n", Thread(), _takeAppWithMe ? " (and taking my node team with me)" : "");

   _policy->OnNodeShutdown();

   delete []_myPath;

   if (_takeAppWithMe) 
   {
      if (BMessenger(be_app).SendMessage(B_QUIT_REQUESTED) != B_NO_ERROR) 
         printf("shNode::~shNode:  Couldn't tell team to quit!\n");
   }
   
   SHDeleteDistributableObject(_policy);
}

SHAccessPolicy & 
shNode :: GetPolicy()
{
   return *_policy;
}

void
shNode::
SetParentIsLocal()
{
   _isParentLocal = true;
}

void
shNode::
AddWorker(SHWorker * worker)
{
   worker->setNode(this);
   _workers.AddItem(worker);
   worker->Start();  // start him off!
}

bool
shNode::
RemoveWorker(int indx)
{
   SHWorker * worker = (SHWorker *)_workers.ItemAt(indx);
   if (worker)
   {
      SHDeleteDistributableObject(worker);      
      _workers.RemoveItem(indx);
      return true;
   }
   else 
   {
      printf("RemoveWorker: ERROR, no worker at position %i!\n",indx);   
      return false;
   }
}

void
shNode::
AddSorter(SHSorter * sorter)
{
   sorter->setNode(this);
   _sorters.AddItem(sorter);
}

const char *
shNode::
GetSystemSorterName() const
{
   return _systemSorter->GetName();
}

void
shNode::
RemoveSorter(int indx, bool nodeIsDying)
{
   SHSorter * sorter = (SHSorter *)_sorters.ItemAt(indx);
   if (sorter)
   {
      if ((sorter == _systemSorter)&&(nodeIsDying == false)) 
      {
         printf("ERROR, ATTEMPTED TO DELETE SYSTEM SORTER!\n");
         return;
      }
      if (sorter == _defaultSorter) _defaultSorter = _systemSorter;
      SHDeleteDistributableObject(sorter);      
      _sorters.RemoveItem(indx);
   }
   else printf("ERROR, no sorter at position %i!\n",indx);   
}

void 
shNode::
AddChild(shNodeMessageStream * child)
{
   _children.AddItem(child);
}

void
shNode::
RemoveChild(int indx, bool okayToSendNotifications)
{
   shNodeMessageStream * child = (shNodeMessageStream *) _children.ItemAt(indx);
   if (child)
   {
      // It's okay to post these before killing the child, since they will be handled
      // in this thread, and thus they won't be handled till after this method returns
      // (and the child has been removed and deleted)
      if (okayToSendNotifications) 
      {
         PostSystemReplyMessages(&child->GetStartupMessage(), SH_OPERATION_FAILED, &child->GetNodeSpec());
      }

      child->RemoveAllOperationTags(okayToSendNotifications ? this : NULL);

      // Remove the child itself
	  if (child->Lock()) 
	  {
	     (void)_children.RemoveItem(indx);

	     child->Quit();
      }
      else printf("RemoveChild: Warning, couldn't lock child!\n");
   }
   else printf("ERROR!  No child at position %i!\n",indx);
}

void
shNode::
RemoveIncomingLink(int indx)
{
   shConnection * lnk = (shConnection *) _links.ItemAt(indx);
   if (lnk)
   {
      lnk->RemoveAllOperationTags(NULL);
      
      // Remove the lnk itself
	  if (lnk->Lock()) 
	  {
	     (void)_links.RemoveItem(indx);
	     lnk->Quit();
      }
      else printf("RemoveIncomingLink: Warning, couldn't lock lnk!\n");
   }
   else printf("ERROR!  No lnk at position %i!\n",indx);
}


char *
shNode ::
TypeToString(uint32 what)
{
   switch(what)
   {
      case SH_COMMAND_BASE:           return "SH_COMMAND_BASE";
      case SH_COMMAND_ADDCOMPONENTS:  return "SH_COMMAND_ADDCOMPONENTS";
	  case SH_COMMAND_REMOVECOMPONENTS: return "SH_COMMAND_REMOVECOMPONENTS";
      case SH_COMMAND_SETPARAMETERS:  return "SH_COMMAND_SETPARAMETERS";
      case SH_COMMAND_GETPARAMETERS:  return "SH_COMMAND_GETPARAMETERS";
      case SH_COMMAND_QUIT:           return "SH_COMMAND_QUIT";
      case SH_CODE_CONNECTIONOPEN:    return "SH_CODE_CONNECTIONOPEN";
	  case SH_CODE_CONNECTIONCLOSED:  return "SH_CODE_CONNECTIONCLOSED";
	  case SH_ENCODING_BATCH:         return "SH_ENCODING_BATCH";
	  case SH_ENCODING_ZLIBLIGHT:     return "SH_ENCODING_ZLIBLIGHT";
	  case SH_ENCODING_ZLIBMEDIUM:    return "SH_ENCODING_ZLIBMEDIUM";
	  case SH_ENCODING_ZLIBHEAVY:     return "SH_ENCODING_ZLIBHEAVY";
	  case SH_ENCODING_NONE:          return "SH_ENCODING_NONE";
	  case SH_INTERNAL_BASE:          return "SH_INTERNAL_BASE";
	  case SH_INTERNAL_NODEPATH:      return "SH_INTERNAL_NODEPATH";
	  case SH_INTERNAL_KILLME:        return "SH_INTERNAL_KILLME";
	  case SH_INTERNAL_NEEDFILE:      return "SH_INTERNAL_NEEDFILE";
	  case SH_INTERNAL_HEREISFILE:    return "SH_INTERNAL_HEREISFILE";
	  case SH_INTERNAL_TAGUNREF:      return "SH_INTERNAL_TAG_UNREF";
	  case SH_INTERNAL_NOOP:          return "SH_INTERNAL_NOOP";
	  case SH_INTERNAL_LINKREQUEST:   return "SH_INTERNAL_LINKREQUEST";
	  case SH_INTERNAL_LINKFAILED:     return "SH_INTERNAL_LINKREPLY";
	  case SH_INTERNAL_LINKREQUESTCOMPLETE:   return "SH_INTERNAL_LINKREQUESTCOMPLETE";
	  case SH_INTERNAL_NEWLINK:       return "SH_INTERNAL_NEWLINK";
	  case SH_INTERNAL_REMOVEWORKER:  return "SH_INTERNAL_REMOVEWORKER";
	  case SH_INTERNAL_LARGEST:       return "SH_INTERNAL_LARGEST";
      default:                        return "User Message?";
   }
}



void
shNode::
MessageReceived(BMessage * msg)
{
   int32 cid;   // id of the messageStream that sent us this message  

   if (msg->what == SH_INTERNAL_NOOP) return;
   if ((_waitingToDie)&&(msg->what != SH_CODE_CONNECTIONCLOSED)) return;  // If we are just waiting to die, then no sense processing messages!

   if (msg->FindInt32(SH_NAME_CONNECTIONID, &cid) == B_NO_ERROR)
   {
       (void)msg->RemoveName(SH_NAME_CONNECTIONID);  // so we can guarantee it doesn't exist later == no extra search

       // Now handle stuff that is specific to where the message is from!
       if ((msg->what == SH_CODE_CONNECTIONOPEN)||(msg->what == SH_CODE_CONNECTIONCLOSED))
          MessageReceivedFromStreamObject(msg, cid);
       else 
          MessageReceivedFromElsewhere(msg, cid);
   }
   else
   {
       // message not from a node--must be from local sorter or worker
       MessageReceivedFromElsewhere(msg, -1);
   }
}

// This method handles messages that originated with the shNodeMessageStream's in our own process space.
void
shNode::
MessageReceivedFromStreamObject(BMessage * msg, int32 cid)
{
//     printf("MsgFromStreamObject: got a msg %p, from %i, type %x:[%s]\n",msg,cid,msg->what,TypeToString(msg->what));
     switch(msg->what)
     {
       case SH_CODE_CONNECTIONCLOSED:
	      if (cid == GetParentID())
	      {
	         printf("Parent closed, so I'm quitting!  Bye bye...\n");
	         
	         if (BMessenger(this).SendMessage(B_QUIT_REQUESTED) != B_NO_ERROR)
	            printf("shNode::MRFSO: Error sending B_QUIT_REQUESTED to myself???\n");
	      }
		  else 
		  {
		     int indx;
		     
		     if (GetChildByCID(cid, &indx)) 
		     {
		        printf("The connection for child cid=%i has been severed, removing it.\n", cid);
		        RemoveChild(indx, true);
		     }
		     else if (GetIncomingLinkByCID(cid, &indx)) 
		     {
		        printf("The incoming sym-lnk cid=%i has been severed, removing it.\n", cid);
		        RemoveIncomingLink(indx);
		     }
		     
		     // Note:  we don't have to send the connectTag operationTag here, is it is done automatically
		     // by RemoveChild()!  Viva la France!
		  }
	    break;
     
        case SH_CODE_CONNECTIONOPEN:
          if (cid == GetParentID())
          {
              //printf("Parent connection open... but we knew that?\n");
          }
          else 
          {
             int indx;
             shNodeMessageStream * child = GetChildByCID(cid, &indx);

             if (child == NULL) child = GetIncomingLinkByCID(cid, &indx);
             if (child) 
             {
                PostSystemReplyMessages(&child->GetStartupMessage(), SH_OPERATION_SUCCEEDED, &child->GetNodeSpec());
                int connectTagID = child->GetAndClearConnectTagID();
                if (connectTagID != -1) child->RemoveOperationTag(connectTagID, this);
             }
             else printf("ERROR:  SH_INTERNAL_CONNECTION_OPEN couldn't find child OR incoming lnk object for cid=%i (parentID = %i)\n",cid, GetParentID());
          }
        break;
        
	    default:
	      printf("MessageReceivedFromStreamObject: unknown message type %i from child cid=%i\n", msg->what,cid);
	    break;
	 }
}


shNodeMessageStream *
shNode::
GetChildByCID(int cid, int * setIndex) const
{
   shNodeMessageStream * child = NULL;
   
   int numChildren = _children.CountItems();
   for (int i = 0; i < numChildren; i++)
   {
      shNodeMessageStream * nextChild = (shNodeMessageStream *)_children.ItemAt(i);
      if (nextChild->GetID() == cid) 
      {
          child = nextChild;
          if (setIndex) *setIndex = i;
          break;
      }
   }
   return(child);
}

shConnection *
shNode::
GetIncomingLinkByCID(int cid, int * setIndex) const
{
   shConnection * lnk = NULL;
   
   int numLinks = _links.CountItems();
   for (int i = 0; i < numLinks; i++)
   {
      shConnection * nextLink = (shConnection *)_links.ItemAt(i);
      if (nextLink->GetID() == cid) 
      {
          lnk = nextLink;
          if (setIndex) *setIndex = i;
          break;
      }
   }
   return(lnk);
}


status_t 
shNode :: 
SendMessageToChild(BMessage * msg, shNodeMessageStream * child)
{            
   if (child->GetFlags() & SH_FLAG_IS_LOCAL) 
   {
      // Since we're sending directly, we have to tell the child who we are--
      // no convenient shConnections in between to do it for us! 
      msg->RemoveName(SH_NAME_CONNECTIONID);
      msg->AddInt32(SH_NAME_CONNECTIONID, GetID());
   }
   return BMessenger(child).SendMessage(msg);
}

SHSorter *
shNode::
RelayMessage(BMessage * msg, shOperationTag * optOpTag)
{
   // first find out which sorter we should use
   SHSorter * sorter = NULL;
   const char * sorterName;
   
   // Hack for when we really really need the message to be forwarded to just one child...
   int32 onlyCID;
   if (msg->FindInt32(SH_NAME_GIVEONLYTOCID, &onlyCID) == B_NO_ERROR)
   {
      (void)msg->RemoveName(SH_NAME_GIVEONLYTOCID);

      if (onlyCID == GetParentID())
      {
         PostReplyMessage(msg);
      }
      else
      {
         shNodeMessageStream * child = GetChildByCID(onlyCID);
         if (child == NULL) child = GetIncomingLinkByCID(onlyCID);
         if (child)
         {
            if (SendMessageToChild(msg, child) != B_NO_ERROR)
                printf("shNode::RelayMessage: Error posting onlycid=%i msg!\n",onlyCID);
         }
      }   
      return NULL;
   }
   
   if (msg->FindString(SH_NAME_WHICHSORTER, &sorterName) == B_NO_ERROR)
   {
     int numSorters = _sorters.CountItems();
     for (int i=0; i < numSorters; i++)
     {
        SHSorter * nextSorter = (SHSorter *)_sorters.ItemAt(i);
        if (strcmp(sorterName, nextSorter->GetName()) == 0)
        {
           sorter = nextSorter;
           break;
        }
     }
   }
   else sorter = _defaultSorter;
   
   if (sorter)
   {
      int i;
      
      // first figure out who to send to...
      bool parentActivated = sorter->DoesMessageGoToNode(*msg, GetNodeSpec(), SH_FLAG_IS_PARENT | (_isParentLocal ? SH_FLAG_IS_LOCAL : 0));
      
      int numChildren = _children.CountItems();
      for (i=0; i<numChildren; i++) 
      {
         shNodeMessageStream * nextChild = (shNodeMessageStream *) _children.ItemAt(i);
         if (sorter->DoesMessageGoToNode(*msg, nextChild->GetNodeSpec(), nextChild->GetFlags())) nextChild->SetActivated();
      }
            
      bool distLocally = sorter->DoesMessageDistributeLocally(*msg);
      
      // any last-minute changes to the msg?
      sorter->BeforeMessageRelay(*msg);
      
      // then do the actual distribution
      if (parentActivated) 
      {
         PostReplyMessage(msg);  // Adds an shCID to (msg), so....
         if ((optOpTag)&&(!SH_IAMROOT)) _holdParentTags.AddOperationTag(optOpTag);
         if (msg->ReplaceInt32(SH_NAME_CONNECTIONID, GetParentID()) != B_NO_ERROR) (void)msg->AddInt32(SH_NAME_CONNECTIONID, GetParentID());
      }
      else (void)msg->AddInt32(SH_NAME_CONNECTIONID, GetParentID());

      for (i=0; i<numChildren; i++)
      {
         if (((shNodeMessageStream *) _children.ItemAt(i))->GetAndClearActivated())
         {
           shNodeMessageStream * nextChild = (shNodeMessageStream *)_children.ItemAt(i);
           if (SendMessageToChild(msg, nextChild) != B_NO_ERROR)
              printf("shNode::RelayMessage: Error sending message to child %i\n",i);
           if (optOpTag) nextChild->AddOperationTag(optOpTag);
         }
      }
      return(distLocally ? sorter : NULL);
   }
   else
   {
      PostSystemReplyMessages(msg, SH_OPERATION_FAILED, NULL);
      return NULL;
   }
}

const char *
shNode::
GetNodePath() const
{
   return _myPath;
}

const char *
shNode::
GetNodeName() const
{
   return(strrchr(_myPath, '/')+1);
}



// Will load from RAM, local disk, or send out cache request(s)...
SHComponent *
shNode::
InstantiateComponent(BMessage * archive, BMessage * mainMsg, bool * setOnOrder, shOperationTag * optOpTag)
{
   *setOnOrder = false;  // default
   SHFileSpec spec;
    
   shPendingDownload * pendingDownload = NULL;
   if ((archive->FindFlat(SH_NAME_ADDONSPEC, &spec) == B_NO_ERROR)&&(CacheFile(spec, &pendingDownload)))
   {
      if (_policy->OkayToInstantiateObject(*archive))
      {
         // Yay, file is available locally!
         SHDistributableObject * dobj = SHCreateDistributableObject(*archive);
         if (dobj)
         {
            SHComponent * alreadyCached = cast_as(dobj, SHComponent);
            if (alreadyCached) return(alreadyCached);
            else 
           {
               printf("shNode::InstantiateComponent:  The SHDistributableObject wasn't an SHComponent!\n");
               SHDeleteDistributableObject(dobj);
            }
         }
      }
      else printf("shNode::InstantiateComponent:  My SHAccessPolicy won't allow me to instantiate this one!\n");
   }
   else
   {
      if (pendingDownload)
      {
         pendingDownload->AddCallback(new shInstantiateOnDownloadCallback(*mainMsg, *archive));  
         if (optOpTag) pendingDownload->AddCallback(new shOpTagCallback(BMessage(SH_INTERNAL_NOOP), &_holdParentTags, optOpTag));
         *setOnOrder = true;
      }
      else printf("shNode::InstantiateComponent:  No SH_NAME_ADDONSPEC field in archive, or CacheFile() failed.\n");
   }
   return(NULL);
}

// Front end to PostSystemReplyMessages, but lookes in (archiveMessage)/SH_NAME_ADDONSPEC for an
// SHFileSpec to add (stripped) as the SH_NAME_REGARDING field.
void 
shNode::
PostSystemReplyMessagesForSpec(BMessage * msg, int32 code, BMessage * archiveMessage)
{
   SHFileSpec spec;
   SHFileSpec * specToUse = NULL;
   
   if (archiveMessage->FindFlat(SH_NAME_ADDONSPEC, &spec) == B_NO_ERROR)
   {
       spec.Strip(SHGetArchitectureBits());
       specToUse = &spec;
   }
   
   PostSystemReplyMessages(msg, code, specToUse);
}


void
shNode::
MessageReceivedFromElsewhere(BMessage * msg, int32 cid)
{
	//printf("MsgFromElsewhere: got a msg %p, from %i, type %x:[%s]\n",msg,cid,msg->what,TypeToString(msg->what));
    shOperationTag * opTag = NULL;

    if (cid == -1)
    {
       // It came from user code!  Check for SH_NAME_WHENDONE.  If there is one, record it in our
       // opTag and replace it in (msg) with an SH_NAME_TAG_REF.
       if (msg->HasMessage(SH_NAME_WHENDONE))
       {
          opTag = new shOperationTag(_opTagIDCounter++, *msg);
          (void)msg->RemoveName(SH_NAME_WHENDONE);  // We don't propogate this, just a reference to it
          (void)msg->AddInt32(SH_NAME_TAGREF, opTag->GetTagID());
       }
    }
    else
    {
       // It was relayed from an adjacent node!  Check for SH_NAME_TAG_REF, and if there is one,
       // create an opTag that will return the reference to the sender when we finish with this message...
       int32 refID;
       if (msg->FindInt32(SH_NAME_TAGREF, &refID) == B_NO_ERROR)
       {
           // (cid) wants us to remember his tag, and return it to him when we are done with this message!
           // This message will be launched when our children send back all replies to this message.
           BMessage tagMsg(SH_INTERNAL_TAGUNREF);
           tagMsg.AddString(SH_NAME_TO, "");   // so it won't get broadcast when it gets to its target node!
           tagMsg.AddInt32(SH_NAME_GIVEONLYTOCID, cid);
           tagMsg.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
           tagMsg.AddInt32(SH_NAME_TAGUNREF, refID);
              
           BMessage wrapMsg; 
           wrapMsg.AddMessage(SH_NAME_WHENDONE, &tagMsg);
              
           opTag = new shOperationTag(_opTagIDCounter++, wrapMsg);
              
           // Replace our parent's tag ID with our own...
           int32 myTagID = opTag->GetTagID();
           (void) msg->ReplaceInt32(SH_NAME_TAGREF, myTagID);
       }
    }

    // RelayMessage does all the SHSorting and forwarding, and returns true iff 
	// we should process the BMessage locally too.
	SHSorter * sorter = RelayMessage(msg, opTag);
	if (sorter)
	{
	    if (_myPath) printf("Message for Me from cid=%i... [%i]:[%s]\n",cid, msg->what, TypeToString(msg->what));

		switch(msg->what) 
		{
		   case SH_INTERNAL_TAGUNREF:
		   {	 
		       int32 refID;

		       if (msg->FindInt32(SH_NAME_TAGUNREF, &refID) == B_NO_ERROR)
		       {
		           // (cid) is telling us he is done with the tag for refID, and we can delete it.
		           // Check our cid's list for any corresponding shOpCompletionTag, and fire it
		           if (cid == GetParentID()) 
		           {
		              _holdParentTags.RemoveOperationTag(refID, this);
		           }
		           else
		           {
		              shNodeMessageStream * tagFrom = GetChildByCID(cid);
		              if (tagFrom == NULL) tagFrom = GetIncomingLinkByCID(cid);
		              if (tagFrom)
		              {
                          tagFrom->RemoveOperationTag(refID, this);
		              }
		              else
		              {
		                  printf("CRITICAL ERROR:  Couldn't set tagFrom(cid=%i)!\n",cid);
		              } 
		           }
		       }
		       else printf("Error, couldn't find the unref tag!\n");
		   }
           break;
           
		   case SH_COMMAND_ADDCOMPONENTS:
		     {
		        // Add any requested sorters (SH_NAME_SORTERS : BArchivable/SHDistributableObject/SHComponent/SHSorter/...)
			    {
	 	            BMessage nextSorterArchive;
			        long nextSorterIndex = 0;
			        while(msg->FindMessage(SH_NAME_SORTERS, nextSorterIndex++, &nextSorterArchive) == B_NO_ERROR)
			        {
			           bool onOrder;
			           
			           SHComponent * comp = InstantiateComponent(&nextSorterArchive, msg, &onOrder, opTag);
			           if (comp)
			           {
			              SHSorter * srt = cast_as(comp, SHSorter);
			              if (srt)
			              {
			                 PostSystemReplyMessagesWithString(msg, SH_OPERATION_SUCCEEDED, srt->GetName());
			                 AddSorter(srt);
			                 printf("Successfully added sorter [%s]\n",srt->GetName());
			              }
                          else
			              {
			                 printf("SH_COMMAND_ADDCOMPONENTS:  Error, archived SHComponent in SH_NAME_SORTERS field wasn't an SHSorter!\n");
 			                 PostSystemReplyMessagesForSpec(msg, SH_OPERATION_FAILED, &nextSorterArchive);
			              }
			           }
			           else
				       {
				          if (onOrder)
				          {
				             printf("SH_COMMAND_ADDCOMPONENTS:  SHSorter add-on file is on order from parent node...\n");
				          }
				          else
				          {
  			                 printf("SH_COMMAND_ADDCOMPONENTS:  Error unarchiving SHSorter!\n");
  			                 PostSystemReplyMessagesForSpec(msg, SH_OPERATION_FAILED, &nextSorterArchive);
				          }
				       }
			        }
		        }

		        // Add any requested workers (SH_NAME_WORKERS : BArchivable/SHDistributableObject/SHComponent/SHWorker/...)
			    {
	 	            BMessage nextWorkerArchive;
			        long nextWorkerIndex = 0;
			        while(msg->FindMessage(SH_NAME_WORKERS, nextWorkerIndex++, &nextWorkerArchive) == B_NO_ERROR)
			        {
			           bool onOrder;
			           
			           SHComponent * comp = InstantiateComponent(&nextWorkerArchive, msg, &onOrder, opTag);
			           if (comp)
			           {
                           SHWorker * worker = cast_as(comp, SHWorker);
			               if (worker)
			               {
 			                  PostSystemReplyMessagesWithString(msg, SH_OPERATION_SUCCEEDED, worker->GetName());
  			                  AddWorker(worker);
			                  printf("SH_COMMAND_ADDCOMPONENTS:  Successfully added worker [%s]\n",worker->GetName());
			               }
			               else
			               {
			                  printf("SH_COMMAND_ADDCOMPONENTS:  Error, archived SHComponent in SH_NAME_WORKERS field wasn't an SHWorker!\n");
 			                  PostSystemReplyMessagesForSpec(msg, SH_OPERATION_FAILED, &nextWorkerArchive);
			               }
			           }
			           else
				       {
				          if (onOrder)
				          {
				             printf("SH_COMMAND_ADDCOMPONENTS:  SHWorker add-on file is on order from parent node...\n");
				          }
				          else
				          {
  			                 printf("SH_COMMAND_ADDCOMPONENTS:  Error unarchiving SHWorker!\n");
  			                 PostSystemReplyMessagesForSpec(msg, SH_OPERATION_FAILED, &nextWorkerArchive);
				          }
				       }
			        }
		        }
		        
		        // connect to any requested children  (SH_NAME_CHILDREN : BFlattenable/SHNodeSpec)
			    {
			        SHNodeSpec nextChild;
			        long nextChildIndex = 0;
			        while(msg->FindFlat(SH_NAME_CHILDREN, nextChildIndex++, &nextChild) == B_NO_ERROR)
			        {
			           printf("Adding %s child node: node=[%s] host=[%s] port=[%i]\n",
			              nextChild.GetHostName()[0] ? "remote" : "local",
			              nextChild.GetNodeName(), nextChild.GetHostName(), nextChild.GetPortNumber());
	
                       shNodeMessageStream * newChild = NULL;
	                   if (nextChild.IsNodeNameValid())
	                   {
				           if (nextChild.GetHostName()[0]) 
				           {
				              // child node is to be created on another host, by connecting to a SockHopServer
				              newChild = new shConnection(new shCallbackConnection, BMessenger(this), GetID(), GetNextConnectionID(), *msg, nextChild, _threadPri, _batchEncoding);
				              if (((shConnection *)newChild)->StartThreads())
				              {
				                 if (opTag) 
				                 {
				                    newChild->AddOperationTag(opTag);
                                    newChild->SetConnectTagID(opTag->GetTagID());
				                 }
				              }
				              else
				              {
				                 printf("Error, couldn't start new child's threads!\n");
				                 delete newChild;
				                 newChild = NULL;   // cause error report, below 
				              }
				           }
				           else
				           {
				              SHAccessPolicy * kidsPolicy = _policy->MakeNodePolicy();  // Does our policy specify the kid's policy?
				              if (kidsPolicy == NULL)
				              {
				                 // Use a clone of our own policy, then
				                 BMessage reArchive;
				                 if (_policy->Archive(&reArchive) == B_NO_ERROR)
				                 {
                                    if (_policy->OkayToInstantiateObject(reArchive))
                                    {
				                       SHFileSpec spec;
				                       if (reArchive.FindFlat(SH_NAME_ADDONSPEC, &spec) == B_NO_ERROR)
				                       {
				                          if (spec.CountFlavors() > 0)
				                          {
				                             SHDistributableObject * obj = SHCreateDistributableObject(&reArchive);
				                             if (obj)
				                             {
				                                if (cast_as(obj, SHAccessPolicy))
				                                {
				                                   kidsPolicy = (SHAccessPolicy *) obj;
				                                }
 				                                else
                                                {
				                                   SHDeleteDistributableObject(obj);
				                                   printf("NewLocalNode:  Clone of policy wasn't an SHAccessPolicy???\n");
				                                }
                                             }
				                             else printf("NewLocalNode:  Couldn't reinstantiate the policy for the new kid!\n");
				                          }
				                          else kidsPolicy = new SHDefaultAccessPolicy(&reArchive);  // no items in spec: must be the default!
				                       }
				                       else printf("NewLocalNode:  Couldn't find SH_NAME_ADDONSPEC in the reArchive of the policy!\n");
				                    }
				                    else printf("NewLocalNode:  My SHAccessPolicy won't allow me to reinstantiate it, to give the new kid a copy!\n");
				                 }
				                 else printf("NewLocalNode:  Couldn't Archive() my policy to give to the kid!\n");
				              }     
				                 
				              // create the child node right here in our process
				              if (kidsPolicy)
				              {
				                 newChild = new shNode(BMessenger(this), GetID(), GetNextConnectionID(), *msg, nextChild, false, kidsPolicy);
				                 newChild->AddFlag(SH_FLAG_IS_LOCAL);  // for us to see that he is local
				                 ((shNode *)newChild)->SetParentIsLocal();  // for him to see that we are local

                                 // This operation succeeds straight away :)
                                 PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, &nextChild);
				              }
                              else printf("NewLocalNode:  Couldn't create local child, his SHAccessPolicy creation failed.\n");
				           }
				           
				           if (newChild)
				           {
				              AddChild(newChild);
			               
			                  // Tell our baby his pathname...	           
                              BMessage whoareyou(SH_INTERNAL_NODEPATH);
				              char * temp = new char[strlen(GetNodePath()) + sizeof('/') + strlen(nextChild.GetNodeName()) + 1];
                              strcpy(temp, GetNodePath());
                              if (strcmp(GetNodePath(), "/")) strcat(temp, "/");
                              strcat(temp, nextChild.GetNodeName());
                              whoareyou.AddString(SH_NAME_PATH, temp);
                              delete []temp;
                              whoareyou.AddString(SH_NAME_TO, "");   // so it only goes to that one node.
                              whoareyou.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());  // make sure it uses our built-in sorter!
                              if (BMessenger(newChild).SendMessage(&whoareyou) != B_NO_ERROR)  // gotta send it direct so we know it'll get there first!!!
                              printf("Error sending whoareyou message to child!\n");
		                      
                              // let him fly
                              newChild->Run();
                           }
				       }
				       
				       // Cause failure report if we didn't actually add a child here
				       if (newChild == NULL) PostSystemReplyMessages(msg, SH_OPERATION_FAILED, &nextChild);
			        }
			     }
			    
			     // Create any requested symlnks (at least, start the process, anyway!)
			     {
                    if (msg->HasString(SH_NAME_SYMLINKS)) 
                    {
                       // Record that we are waiting for the lnk process to complete...
			           shPendingLink * lnk = new shPendingLink(this, msg, opTag);
			           if (lnk->Start()) 
			           {
                          // A BMessage asking the node(s) to allow us to connect to them...
                          BMessage requestMessage(SH_INTERNAL_LINKREQUEST);
                          SHNodeSpec acceptSpec = lnk->GetAcceptSpec();
                          requestMessage.AddFlat(SH_NAME_LINKTARGET, &acceptSpec);
		                
                          long nextLinkIndex = 0;
                          const char * nextLink;
                          while(msg->FindString(SH_NAME_SYMLINKS, nextLinkIndex++, &nextLink) == B_NO_ERROR)
                               requestMessage.AddString(SH_NAME_TO, nextLink);
			               
			              // If a target node can't connect to us, he will send this BMessage back to us,
			              // which will cause us to send a failure BMessage from our pendingLink back to the user.
			              // We don't need to do this for an onSuccess message, because we will be able
			              // to communicate directly with the lnking node then.
			              BMessage onFailure(SH_INTERNAL_LINKFAILED);
			                onFailure.AddString(SH_NAME_TO, GetNodePath());
			                onFailure.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
			                onFailure.AddPointer(SH_NAME_LINKOPID, lnk);
			              requestMessage.AddMessage(SH_NAME_ONFAILURE, &onFailure);
			                 
                          // (whenDone) will be sent to us after all their replies have been received.
                          // That's how we'll know when to shut down and delete our shPendingLink.
	                      BMessage whenDone(SH_INTERNAL_LINKREQUESTCOMPLETE);
			                whenDone.AddString(SH_NAME_TO, "");  // This will be posted from our node, so just address it to "current node".
			                whenDone.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
                            whenDone.AddPointer(SH_NAME_LINKOPID, lnk);
			              requestMessage.AddMessage(SH_NAME_WHENDONE, &whenDone);
			              
			              // And away it goes!  
			              PostSystemMessage(&requestMessage);
			              _pendingLinks.AddItem(lnk);
			           }
			           else
			           {
			              printf("Error, couldn't start up acceptor for incoming symlnk connections!\n");
                          PostSystemReplyMessages(msg, SH_OPERATION_FAILED, NULL);
			              delete lnk;
			           }
			        }
			     }
			     
			     // Cache any requested files  (SH_NAME_FILES : BFlattenable/SHFileSpec)
				 {
	               SHFileSpec nextSpec;
	               long next = 0;
	                 
			       while(msg->FindFlat(SH_NAME_FILES, next++, &nextSpec) == B_NO_ERROR) 
			       {
			          shPendingDownload * pendingDownload;
			           
			          if (CacheFile(nextSpec, &pendingDownload))
			          {
			             nextSpec.Strip(SHGetArchitectureBits());
			             PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, &nextSpec);
			          }
			          else
			          {  
			              if (pendingDownload)
			              {
			                 // This callback will make sure success/failure messages are
			                 // sent as appropriate when the download completes.
			                 pendingDownload->AddCallback(new shPendingDownloadCallback(msg));
	                         if (opTag) pendingDownload->AddCallback(new shOpTagCallback(BMessage(SH_INTERNAL_NOOP), &_holdParentTags, opTag));
			              }
			              else
			              {
			                 nextSpec.Strip(SHGetArchitectureBits());
			                 PostSystemReplyMessages(msg, SH_OPERATION_FAILED, &nextSpec);
			              }
			           }
			        }
	             }		      
		      }
		      break;

		   case SH_COMMAND_REMOVECOMPONENTS:
		     {
		        SHStringMatcher matcher;
		        
		        // Remove any specified sorters (SH_NAME_SORTERS : String)
			    {
	 	            long nextIndex = 0;
			        const char * name;
			        while(msg->FindString(SH_NAME_SORTERS, nextIndex++, &name) == B_NO_ERROR)
			        {
			            // Check sorters for names matching (name)
			            matcher.SetSimpleExpression(name);
			            
			            // sorters
			            int num = _sorters.CountItems();
			            for (int i=num-1; i>=0; i--)
			            {
			               SHSorter * nextSorter = (SHSorter *) _sorters.ItemAt(i);
			               if (nextSorter != _systemSorter)  // NEVER delete the system sorter, we need it!
			               {
			                  if (matcher.Match(nextSorter->GetName()))
			                  {
                                 PostSystemReplyMessagesWithString(msg, SH_OPERATION_SUCCEEDED, name);
			                     RemoveSorter(i, false);
			                  }
			               }
			            }
                    }
                }

		        // Remove any specified workers (SH_NAME_WORKERS : String)
                {
	 	            long nextIndex = 0;
			        const char * name;

			        while(msg->FindString(SH_NAME_WORKERS, nextIndex++, &name) == B_NO_ERROR)
			        {
			            // Check workers for names matching (name)
			            matcher.SetSimpleExpression(name);
			            
			            // workers
			            int num = _workers.CountItems();
			            for (int j=num-1; j>=0; j--)
			            {
			               SHWorker * nextWorker = (SHWorker *) _workers.ItemAt(j);
			               if (matcher.Match(nextWorker->GetName())) 
			               { 
                              PostSystemReplyMessagesWithString(msg, SH_OPERATION_SUCCEEDED, name);
                              RemoveWorker(j);
			               }
			            }
			        }
		        }

		        // Remove any specified children (SH_NAME_CHILDREN and SH_NAME_SYMLINKS)
			    {
	 	            const char * typeFields[] = {SH_NAME_CHILDREN, SH_NAME_SYMLINKS};
			        
			        for (int childType = 0; childType < (sizeof(typeFields)/sizeof(const char *)); childType++)
			        {
			           long nextIndex = 0;
			           const char * name;
			           
			           while(msg->FindString(typeFields[childType], nextIndex++, &name) == B_NO_ERROR)
			           {
			              matcher.SetSimpleExpression(name);
			              
			              for (int i=0; i<_children.CountItems(); i++)
			              {
			                 shNodeMessageStream * nextChild = (shNodeMessageStream *) _children.ItemAt(i);
			                 
			                 // the SH_NAME_SYMLINKS fields should only remove symlnks...
			                 if ((childType == 1)&&((nextChild->GetFlags() & SH_FLAG_IS_SYMLINK) == 0)) continue;
			                 
			                 if (matcher.Match(nextChild->GetNodeSpec().GetNodeName()))
			                 {
                                 PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, &nextChild->GetNodeSpec());
                                 RemoveChild(i, true);
                                 i--;
                             }
                          }  
			           }
			        }
		        }
			    
                // Remove all specified files (SH_NAME_FILES)
				{
	               SHFileSpec nextSpec;
	               SHFlavor flav;
	               long next = 0;
	                 
			       while(msg->FindFlat(SH_NAME_FILES, next++, &nextSpec) == B_NO_ERROR) 
			       {
			           for (int i=0; nextSpec.GetFlavorAt(i, flav); i++) 
			           {
			              /*if (flav.SupportsArchitecture(SHGetArchitectureBits())) -I don't see why it should only work for native files? -jaf*/
			              unlink(flav.GetSuggestedName());
			           }
                       PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, &nextSpec);
                   }
	             }		      
		      }
		      break;

           case SH_COMMAND_QUIT:
              {
                 // If I am the root node, I can't ask dad to kill me;  rather I can just up and kill myself!
                 // If my parent isn't local, then I can kill myself without warning, and TCP will inform dad.
                 if ((SH_IAMROOT)||(_isParentLocal == false))
                 {
                    printf("Root node got a SH_COMMAND_QUIT, bye bye!\n"); 
                    if (BMessenger(this).SendMessage(B_QUIT_REQUESTED) != B_NO_ERROR)
                       printf("SH_COMMAND_QUIT:  Error sending quit request to myself???\n");
                    fflush(stdout);
                 }
                 else
                 {
                    printf("Non-root node got a request to quit--I will tell my parent to kill me!\n"); fflush(stdout);
                    BMessage killMe(SH_INTERNAL_KILLME);
                    killMe.AddString(SH_NAME_TO, "");
                    killMe.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
                    if (PostReplyMessage(&killMe) != B_NO_ERROR) printf("Error, PostReplyMessage failed! (42)\n");
			        _waitingToDie = true;
			     }
              }
              break;

           case SH_INTERNAL_NODEPATH:
              {
                 const char * newPath;
                 if (msg->FindString(SH_NAME_PATH, &newPath) == B_NO_ERROR)
                 {
                    if (_myPath) printf("CRITICAL ERROR: NODEPATH is getting set twice!!!\n");
                    else
                    {
                       _myPath = new char[strlen(newPath) + 1];
                       strcpy(_myPath, newPath);
                       printf("I have been Christened!\n");
                    }
                 }
                 else printf("Error, couldn't find my shPath!\n");
              }
              break;		 

           case SH_INTERNAL_KILLME:
              {
                 int indx;
		         if (GetChildByCID(cid, &indx)) RemoveChild(indx, true);
		                             else printf("Warning2:  Couldn't find child to kill! (cid=%i)!\n",cid);
              }
              break;

           case SH_INTERNAL_LINKREQUEST:
              {
                  // We get this message from other nodes who want to establish a symlnk to our
                  // node.  In response, we attempt to connect to the SHNodeSpec included in the
                  // SH_INTERNAL_LINKTARGET field of their message.
                  bool success = false;

                  SHNodeSpec lnkTarget;
                  if (msg->FindFlat(SH_NAME_LINKTARGET, &lnkTarget) == B_NO_ERROR)
                  {
                     lnkTarget.GetSpecMessage().AddString(SH_NAME_SYMLINKS, GetNodePath());

                     // On failure, we need to send the failure messages from (msg).
                     shConnection * newIncomingLink = new shConnection(new shStraightConnection, BMessenger(this), -1, GetNextConnectionID(), *msg, lnkTarget, _threadPri, _batchEncoding);
                     newIncomingLink->AddFlag(SH_FLAG_IS_SYMLINK);

                     if (newIncomingLink->StartThreads())
                     { 
                        if (opTag) 
                        {
                           newIncomingLink->AddOperationTag(opTag);  // We're not done til this guy connects or fails!
                           newIncomingLink->SetConnectTagID(opTag->GetTagID());
                        }
                        newIncomingLink->Run();
                        _links.AddItem(newIncomingLink);
                        success = true;
		             }
		             else
		             {
		                printf("SH_INTERNAL_LINKREQUEST:  Couldn't start threads!\n");
		                if (newIncomingLink->Lock()) newIncomingLink->Quit();
		             }
	              }
	              else printf("SH_INTERNAL_LINKREQUEST:  No SH_NAME_LINKTARGET!\n");
	              
	              if (success == false) PostSystemReplyMessages(msg, SH_OPERATION_FAILED, &lnkTarget);
              }
              break;
           
           case SH_INTERNAL_LINKFAILED:
              {
                  shPendingLink * pendingLink = GetPendingLinkFrom(msg, false);
                  
                  if (pendingLink) 
                  {
                      const char * from = NULL;
                      (void)msg->FindString(SH_NAME_SYMLINKS, &from);
                      pendingLink->PostFailureMessage(from);  // This will go back to the user and tell him a lnk failed.
                  }
                  else printf("SH_INTERNAL_LINKREQUESTFAILED:  couldn't find pendingLink!\n");
              }
              break;

           case SH_INTERNAL_NEWLINK:
              {
                 const char * name;
                 if (msg->FindString(SH_NAME_SYMLINKS, &name) == B_NO_ERROR)
                 {
                     shPendingLink * pendingLink = GetPendingLinkFrom(msg, false);
                     
                     if (pendingLink) 
                     {
                         int32 cid;
                         
                         if (msg->FindInt32(SH_NAME_NEWLINKID, &cid) == B_NO_ERROR)
                         {
                             SHDirectConnection * dc = pendingLink->DetachSession(cid);
                             if (dc) 
                             {
                                shConnection * conn = dc->_connection;

                                dc->_connection = NULL;  // Rip out his still-beating heart!
                                delete dc;  // and throw his lifeless corpse down the temple steps

                                conn->AddFlag(SH_FLAG_IS_SYMLINK);
                                AddChild(conn);
                                pendingLink->PostSuccessMessage(name);
                             }
                             else printf("SH_INTERNAL_NEWLINK:  Couldn't DetachSession %i!\n", cid);
                         }
                         else printf("SH_INTERNAL_NEWLINK:  cid not found!\n");
                     }
                     else printf("SH_INTERNAL_NEWLINK:  pendingLink not found!\n");
                 }
                 else printf("SH_INTERNAL_NEWLINK:  Error, no lnk name!\n");
              }
              break;
              
           case SH_INTERNAL_LINKREQUESTCOMPLETE:
              {
                  shPendingLink * pendingLink = GetPendingLinkFrom(msg, true);

                  if (pendingLink) delete pendingLink;
                              else printf("SH_INTERNAL_LINKREQUESTCOMPLETE:  couldn't find pendingLink!\n");
              }
              break;
                       
           case SH_INTERNAL_HEREISFILE:
              {
	              if (cid != GetParentID()) printf("HEREISFILE:  Error, it didn't come from daddy?\n");
	              
	              SHFileSpec fileSpec;
	              if (msg->FindFlat(SH_NAME_FILESPEC, &fileSpec) == B_NO_ERROR)
	              {
	                 bool saved = false;
	                 BMessage file;
                     SHFlavor flav;                     

	               	 if (msg->FindMessage(SH_NAME_FILE, &file) == B_NO_ERROR)
	                 {
	                     if (fileSpec.GetFlavorAt(0, flav) == B_NO_ERROR)
	                     {
                            const char * name = flav.GetSuggestedName();
                            bool save = (msg->FindString(SH_NAME_DONTSAVEFILE) == NULL);
                            printf("HEREISFILE:  Got file [%s] from daddy, %s\n", name, save ? "saving it!" : "don't need to save it!");
	                        saved = save ? ((_policy->OkayToWriteFile(name))&&(shMessageToFile(name, &file, flav.GetModificationTime()))) : true;
                            if (saved == false) printf("HEREISFILE:  Error saving file [%s]\n", name);
	                     }
	                 }
		             
	                 int num = _pendingDownloads.CountItems();
	                 for (int i=num-1; i>=0; i--)
	                 {
	                    shPendingDownload * nextDL = (shPendingDownload *) _pendingDownloads.ItemAt(i);
	                    
	                    // pending download is complete if removing the newly-downloaded flavor makes it empty.
	                    if ((nextDL->GetModifiedFileSpec().RemoveFlavor(flav) == B_NO_ERROR)&&(nextDL->GetModifiedFileSpec().CountFlavors() == 0))
	                    {
	                       (void) _pendingDownloads.RemoveItem(i);
	                       if (saved) 
	                       {
	                       	   BMessage forwardMsg(SH_INTERNAL_HEREISFILE);

	                           forwardMsg.AddFlat(SH_NAME_FILESPEC, &fileSpec);
                               forwardMsg.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
	                           forwardMsg.AddMessage(SH_NAME_FILE, &file);

	                           nextDL->DoCallbacks(&forwardMsg);

                               if (forwardMsg.FindString(SH_NAME_TO)) 
                               {
                                  if (BMessenger(this).SendMessage(&forwardMsg) != B_NO_ERROR)
                                     printf("SH_INTERNAL_HEREISFILE:  Error sending HEREISFILE message to myself?\n");
                               }
	                       }
	                       else nextDL->PostFailures();
	                       delete nextDL;
	                    }
	                 }
		          }
		          else printf("HEREISFILE:  Error, daddy gave me no fileSpec!\n");
	          }
              break;
              
           case SH_INTERNAL_NEEDFILE:
              {
                 // A request from our kid saying he needs a file!
                 if ((cid >= 0)&&(cid != GetParentID()))
                 {
                    // we check exactly one FileSpec per NEEDFILE request.
                    SHFileSpec filespec;
                    shNodeMessageStream * child = GetChildByCID(cid);
                    bool sendMsg = true;
                    BMessage file;
                    bool fileValid = false;
                    
	                if (child)
	                {
	                   SHFlavor flav;
	                   
	                   if ((msg->FindFlat(SH_NAME_FILESPEC, &filespec) == B_NO_ERROR)&&(filespec.GetFlavorAt(0, flav) == B_NO_ERROR))
			           {
			              shPendingDownload * pendingDownload;
			              
			              if (CacheFile(filespec, &pendingDownload, flav.GetArchitectureBits()))
		                  {
                              fileValid = shFileToMessage(flav.GetSuggestedName(), &file);
			              }
			              else
			              {
			                  if (pendingDownload)
			                  {
			                     printf("NEEDFILE:  Request for file [%s] is pending parent...\n", flav.GetSuggestedName());
			                     pendingDownload->AddCallback(new shForwardOnDownloadCallback(*msg, cid));
 	                             if (opTag) pendingDownload->AddCallback(new shOpTagCallback(BMessage(SH_INTERNAL_NOOP), &_holdParentTags, opTag));
			                     sendMsg = false;   // will send message later, so not now
			                  }
			                  else
			                  {
			                     // do nothing, message will be sent by default
			                     printf("NEEDFILE:  CacheFile failed for file [%s]!\n", flav.GetSuggestedName());
			                  }
			              }
			            }
			            else printf("NEEDFILE1:  Couldn't get FileSpec!\n");
			            
			            if (sendMsg)
			            {
			               BMessage fileMsg(SH_INTERNAL_HEREISFILE);
			               fileMsg.AddString(SH_NAME_TO, "");
	                       fileMsg.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
	                       fileMsg.AddFlat(SH_NAME_FILESPEC, &filespec);
	                       if (fileValid) 
	                       {
	                          if (is_instance_of(child, shNode)) fileMsg.AddString(SH_NAME_DONTSAVEFILE, "");  // If we know child node is in our process, then we know it has the same current directory, hence a 2nd save is unnecessary
	                          fileMsg.AddMessage(SH_NAME_FILE, &file);
	                       }
	                       if (BMessenger(child).SendMessage(&fileMsg) != B_NO_ERROR)
	                          printf("NEEDFILE:  Error sending message to child!\n");
			            }
		            }
                    else printf("NEEDFILE:  Couldn't find child???\n");		            
                 }
                 else printf("Error, SH_INTERNAL_NEEDFILE with cid=%i (parentID=%i)???\n",cid, GetParentID());
              }
              break;
           
           case SH_INTERNAL_REMOVEWORKER:
           {
              int32 id;
              
              if (msg->FindInt32(SH_NAME_WORKERID, &id) == B_NO_ERROR)
              {
                 int num = _workers.CountItems();
                 for (int i=0; i<num; i++)
                 {
                    SHWorker * nextWorker = (SHWorker *) _workers.ItemAt(i);
                    if (nextWorker->GetId() == id)
                    {
                       RemoveWorker(i);
                       break;
                    }
                 }
              }
              else printf("SH_INERNAL_REMOVEWORKER:  Couldn't find SH_NAME_WORKERID!\n");
           }
           break;
                             
           case SH_COMMAND_SETPARAMETERS:
           {
               int numSucceeded = 0;
               int numFailed = 0;
        
               const char * stringData;
               int32 intData;
               
               if (msg->FindString(SH_PARAMNAME_DEFAULTSORTER, &stringData) == B_NO_ERROR) 
               {
                   int sorterListLen = _sorters.CountItems();
                   bool foundSorter = false;
                   
                   for (int i=0; i<sorterListLen; i++)
                   {
                      SHSorter * nextSorter = (SHSorter *) _sorters.ItemAt(i);
                      if (strcmp(stringData, nextSorter->GetName()) == 0)
                      {
                          _defaultSorter = nextSorter;
                          printf("Default sorter is now [%s]\n",_defaultSorter->GetName());
                          foundSorter = true;
                          break;
                      }
                   }
                   
                   if (foundSorter) numSucceeded++; else numFailed++;
                   
                   AddToReplyMessages(msg, 
                      foundSorter ? SH_OPERATION_SUCCEEDED : SH_OPERATION_FAILED, 
                      SH_PARAMNAME_DEFAULTSORTER, B_STRING_TYPE, stringData, strlen(stringData)+1);
			   }
               if (msg->FindInt32(SH_PARAMNAME_DEBUG, &intData) == B_NO_ERROR)
               {
                  if (intData == 0) printf("Debugging output off.\n");
                  _debugLevel = intData;
                  if (intData != 0) printf("Debugging output on.\n");

                  AddToReplyMessages(msg, SH_OPERATION_SUCCEEDED, SH_PARAMNAME_DEBUG, B_INT32_TYPE, &intData, sizeof(intData));
                  numSucceeded++;
               }
               if (msg->FindInt32(SH_PARAMNAME_THREADPRIORITY, &intData) == B_NO_ERROR)
               {
                  printf("Changing thread priority to %li.  (Affects newly spawned threads only)\n", intData);
                  _threadPri = intData;
                  numSucceeded++;
               }
               if (msg->FindInt32(SH_PARAMNAME_TRANSMISSIONENCODING, &intData) == B_NO_ERROR)
               {
                  printf("Changing transmission encoding to %li.  (Affects newly spawned threads only)\n", intData);
                  _batchEncoding = intData;
                  numSucceeded++;
               }               
               
               if (numFailed > 0)    PostSystemReplyMessages(msg, SH_OPERATION_FAILED, NULL);
               if (numSucceeded > 0) PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, NULL);
           }
           break;
         
           case SH_COMMAND_GETPARAMETERS:
           {
               int numSucceeded = 0;
               int numFailed = 0;
               
               // actual parameters get parsed here!
               if (msg->HasData(SH_PARAMNAME_DEFAULTSORTER, B_ANY_TYPE)) 
               {
                  AddToReplyMessages(msg, SH_OPERATION_SUCCEEDED, SH_PARAMNAME_DEFAULTSORTER, 
                                     B_STRING_TYPE, _defaultSorter->GetName(), strlen(_defaultSorter->GetName())+1);
                  numSucceeded++;
               }                      
               if (msg->HasData(SH_PARAMNAME_DEBUG, B_ANY_TYPE))
               {
                  AddToReplyMessages(msg, SH_OPERATION_SUCCEEDED, SH_PARAMNAME_DEBUG,
                                     B_INT32_TYPE, &_debugLevel, sizeof(_debugLevel));
                  numSucceeded++;
               }
               if (msg->HasData(SH_PARAMNAME_THREADPRIORITY, B_ANY_TYPE))
               {
                  AddToReplyMessages(msg, SH_OPERATION_SUCCEEDED, SH_PARAMNAME_DEBUG,
                                     B_INT32_TYPE, &_threadPri, sizeof(_threadPri));
                  numSucceeded++;
               }
               if (msg->HasData(SH_PARAMNAME_TRANSMISSIONENCODING, B_ANY_TYPE))
               {
                  AddToReplyMessages(msg, SH_OPERATION_SUCCEEDED, SH_PARAMNAME_DEBUG,
                                     B_INT32_TYPE, &_batchEncoding, sizeof(_batchEncoding));
                  numSucceeded++;
               }
               if (numFailed > 0) PostSystemReplyMessages(msg, SH_OPERATION_FAILED, NULL);
               if (numSucceeded > 0) PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, NULL);
           }
		   break;
           
		   default:
		      {
		         sorter->BeforeLocalMessageDistribute(*msg);
		         int num = _workers.CountItems();
		         for (int i=0; i<num; i++)
		         {
		            SHWorker * nextWorker = (SHWorker *) _workers.ItemAt(i);
		            if ((nextWorker->IsInterestedIn(msg))&&(sorter->DoesMessageGoToWorker(*msg, nextWorker->GetName())))
		            {
		               // SendMessage() may return an error here, if the worker's
		               // BLooper has already gone away, but we haven't removed the SHSorter
		               // yet.  But that's okay.
		               (void)nextWorker->GetMessenger().SendMessage(msg);
		            }
		         }
		         PostSystemReplyMessages(msg, SH_OPERATION_SUCCEEDED, NULL);
		      }
		      break;
		}
	}

	if (opTag) (void) opTag->DecReferenceCount(this);
}


void
shNode::
PostSystemMessage(BMessage * msg)
{
   msg->AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());  // make sure it uses our built-in sorter!
   if (BMessenger(this).SendMessage(msg) != B_NO_ERROR)   // send it to ourself.  The normal mechanism will handle it from there
      printf("PostSystemMessage:  Couldn't post message to myself?\n");
}

void
shNode::
PostSystemReplyMessages(const BMessage * regarding, int32 code, const BFlattenable * optSpecific)
{
   char * replyTag = NULL;

   switch(code)
   {
      case SH_OPERATION_SUCCEEDED: replyTag = SH_NAME_ONSUCCESS;  break;
      case SH_OPERATION_FAILED:    replyTag = SH_NAME_ONFAILURE;  break;
   }

   if (replyTag)
   {
      BMessage replyMsg;
      
      int next = 0;

      while(regarding->FindMessage(replyTag, next++, &replyMsg) == B_NO_ERROR)
      {
         if (optSpecific) replyMsg.AddFlat(SH_NAME_REGARDING, ((BFlattenable *)optSpecific)); // note: shouldn't AddFlat() take a (const BFlattenable *)?
         replyMsg.AddString(SH_NAME_FROM, GetNodePath());
         if (BMessenger(this).SendMessage(&replyMsg) != B_NO_ERROR)
            printf("PostSystemReplyMessages:  Error sending message to myself?\n");
      }
   }
}

void
shNode::
PostSystemReplyMessagesWithString(const BMessage * regarding, int32 code, const char * optSpecific)
{
   char * replyTag = NULL;

   switch(code)
   {
      case SH_OPERATION_SUCCEEDED: replyTag = SH_NAME_ONSUCCESS;  break;
      case SH_OPERATION_FAILED:    replyTag = SH_NAME_ONFAILURE;  break;
   }
   
   if (replyTag)
   {
      BMessage replyMsg;
      
      int next = 0;

      while(regarding->FindMessage(replyTag, next++, &replyMsg) == B_NO_ERROR)
      {
         if (optSpecific) replyMsg.AddString(SH_NAME_REGARDING, optSpecific); 
         replyMsg.AddString(SH_NAME_FROM, GetNodePath());
         if (BMessenger(this).SendMessage(&replyMsg) != B_NO_ERROR)
            printf("PostSystemReplyMessagesWithString:  Error sending message to myself?\n");
      }
   }
}

bool
shNode::
CacheFile(const SHFileSpec & spec, shPendingDownload ** setPendingDownload)
{
   return CacheFile(spec, setPendingDownload, SHGetArchitectureBits());
}

// Returns true if all files are available immediately, (false&&(pendingDownload != NULL))
// if one or more files have been put on order, (false&&(pendingDownload == NULL)) if an error occurred.
bool
shNode::
CacheFile(const SHFileSpec & specRef, shPendingDownload ** setPendingDownload, uint32 forArchitecture)
{
   *setPendingDownload = NULL;   // default = no download pending due to this method call

   SHFileSpec spec(specRef);     // make a local copy that we can modify (mutilate)
   spec.Strip(forArchitecture);  // and strip it to just the stuff for the specified architecture.
   if (spec.CountFlavors() == 0)
   {
      printf("CacheFile:  Error, no flavors are usable by me!\n");
      return false;
   }
   
   SHFlavor flav;  // temp storage for SHFlavors at various places below

   // First check to see if the all the files are available and up-to-date on our local drive.
   for (int h=spec.CountFlavors()-1; h >= 0; h--)  // backwards since we may be removing items as we go
   {
      if (spec.GetFlavorAt(h, flav) == B_NO_ERROR)
      {
         BEntry file(flav.GetSuggestedName());
         if (file.Exists())
         {
            off_t localSize = -1;
            if ((file.GetSize(&localSize) == B_NO_ERROR)&&(localSize == flav.GetSize()))
            {
               time_t localTime = 0;
               if ((file.GetModificationTime(&localTime) == B_NO_ERROR)&&(localTime == flav.GetModificationTime()))
               {
                   printf("CacheFile:  Looks like local file [%s] is up-to-date!\n", flav.GetSuggestedName());
                   spec.RemoveFlavorAt(h);
               }
               else printf("CacheFile:  Local time isn't the same as parent's time! %i vs %i\n", localTime, flav.GetModificationTime());
            } 
            else printf("CacheFile:  Local size isn't the same as parent's size! %Li vs %Li\n", localSize, flav.GetSize());
         }
         else printf("CacheFile:  File [%s] doesn't exist locally...\n", flav.GetSuggestedName());
      }
      else printf("CacheFile:  Error 1 in GetFlavorAt(%i/%i)\n", h, spec.CountFlavors());
   }
             
   // If spec is empty, then we already have up-to-date versions of the all the files required!
   if (spec.CountFlavors() == 0) return true;

   // Otherwise, we need to request the files.
   if (SH_IAMROOT) return false;   // root node can NEVER cache files, since he has nobody above him to get them from.
      
   // Remove any files that are already on order by other components.
   // (this is important, otherwise we might end up downloading a file more than once => waste of bandwidth)
   SHFileSpec allNeededFiles(spec);  // keep an unmodified copy so we can track all the files as they come in
   int numPendingDownloads = _pendingDownloads.CountItems();
   for (int i=spec.CountFlavors()-1; i >= 0; i--)
   {
      if (spec.GetFlavorAt(i, flav) == B_NO_ERROR)
      {      
         for (int j=0; j<numPendingDownloads; j++)
         {
            shPendingDownload * nextDL = (shPendingDownload *) _pendingDownloads.ItemAt(j);
            if (nextDL->GetModifiedFileSpec().IndexOf(flav) >= 0)
            {
               printf("File [%s] is already on order, I'll just wait for it to be sent!\n", flav.GetSuggestedName());
               spec.RemoveFlavorAt(i);
               break;
            }
         }
      }
      else printf("CacheFile:  Error 2 in GetFlavorAt(%i/%i)\n", i, spec.CountFlavors());
   }
 
   // If we got here, then we need to ask daddy for the file(s)
   // Protocol demands that we ask for each file in its own separate SHFileSpec (should probably be sent as an SHFlavor but oh well)
   // File a request form for each file we don't have, that isn't on order... 
   int numNotOnOrder = spec.CountFlavors();
   for (int j=0; j<numNotOnOrder; j++) 
   {
      if (spec.GetFlavorAt(j, flav) == B_NO_ERROR)
      {
         SHFileSpec neededFile;
         neededFile.AddFlavor(flav);
                
         printf("Requesting a copy of file [%s] from daddy...\n", flav.GetSuggestedName());
         BMessage requestDownload(SH_INTERNAL_NEEDFILE);
         requestDownload.AddFlat(SH_NAME_FILESPEC, &neededFile);
         requestDownload.AddString(SH_NAME_TO, "");
         requestDownload.AddString(SH_NAME_WHICHSORTER, _systemSorter->GetName());
         if (PostReplyMessage(&requestDownload) != B_NO_ERROR) printf("Warning, SH_INTERNAL_NEEDFILE message post failed\n");
      }
      else printf("CacheFile:  Error 3 in GetFlavorAt(%i/%i)\n", j, numNotOnOrder);
   }
   _pendingDownloads.AddItem(*setPendingDownload = new shPendingDownload(this, allNeededFiles));
   return false;
}

const char *
shNode::
GetChildNodeNameByCID(int cid) const
{
   int indx;
   shNodeMessageStream * child = GetChildByCID(cid, &indx);  
   if (child == NULL) child = GetIncomingLinkByCID(cid, &indx);
   return (child ? child->GetNodeSpec().GetNodeName() : "<error>");
}


shPendingLink *
shNode::
GetPendingLinkFrom(const BMessage * msg, bool removeToo)
{
   const shPendingLink * lnk;
   
   if (msg->FindPointer(SH_NAME_LINKOPID, (void **) &lnk) == B_NO_ERROR)
   {
      int len = _pendingLinks.CountItems();
      for (int i=0; i<len; i++)
      {
         shPendingLink * nextLink = (shPendingLink *) _pendingLinks.ItemAt(i);
         if (lnk == nextLink) 
         {
            if (removeToo) (void)_pendingLinks.RemoveItem(i);
            return nextLink;
         }
      }
      printf("GetPendingLinkFrom:  Warning, couldn't find shPendingLink!\n");
   }
   else printf("GetPendingLinkFrom:  Warning, no SH_NAME_OP_ID in message!\n");
   
   return NULL;
}

BLooper * SHCreateRootNode(const BMessenger & replyTarget, SHAccessPolicy * policy)
{
   shNode * newNode = new shNode(replyTarget, GetNextConnectionID(), GetNextConnectionID(), BMessage(), SHNodeSpec("<user code>"), false, policy ? policy : new SHDefaultAccessPolicy);
   newNode->SetParentIsLocal();
   
   // Tell the node that he is a root node
   BMessage whoareyou(SH_INTERNAL_NODEPATH);
   whoareyou.AddString(SH_NAME_PATH, "/");
   whoareyou.AddString(SH_NAME_TO, "");   // so it only goes to that one node.  Probably not necessary since we know there's no subtree yet, but good form!
   if (BMessenger(newNode).SendMessage(&whoareyou) != B_NO_ERROR)
   {
      newNode->Quit();
      return NULL;
   }
   return newNode;
}

status_t copyFieldTo(const char * fieldName, type_code dataType, const BMessage & from, BMessage & to)
{
   const void * data;
   ssize_t numBytes;
   int next = 0;
   
   while(from.FindData(fieldName, dataType, next++, &data, &numBytes) == B_NO_ERROR)
   {
      status_t ret = to.AddData(fieldName, dataType, data, numBytes, false);
      if (ret != B_NO_ERROR) return ret;
   }
   return B_NO_ERROR;
}

void
AddToReplyMessages(BMessage * msg, int32 code, const char * fieldName, type_code dataType, const void * data, ssize_t dataSize)
{
   int next = 0;
   BMessage temp;
   const char * codeField;
   
   switch(code)
   {
      case shNode::SH_OPERATION_SUCCEEDED:   codeField = SH_NAME_ONSUCCESS;  break;
      case shNode::SH_OPERATION_FAILED:      codeField = SH_NAME_ONFAILURE;  break;
      default:  printf("AddToReplyMessages:  bad code %li!\n", code); return;
   }
   
   while(msg->FindMessage(codeField, next, &temp) == B_NO_ERROR)
   {
      temp.AddData(fieldName, dataType, data, dataSize);
      msg->ReplaceMessage(codeField, next, &temp);
      next++;
   }
}

// Only does loading from RAM or from local disk!
SHDistributableObject *
SHCreateDistributableObject(const BMessage & archive)
{
   SHFileSpec spec;
   
   if (archive.FindFlat(SH_NAME_ADDONSPEC, &spec) == B_NO_ERROR)
   {
      shAddOnTag * tag = shAddOnTag::GetTag(spec);
      if (tag)
      {
         SHDistributableObject * obj = tag->InstantiateObject(archive);
         if (obj) 
         {
            obj->_addOnTags.AddItem(tag);
            return obj;
         }
         else printf("SHCreateDistributableObject:  ERROR, couldn't instantiate SHDistributableObject!\n");

         delete tag;
      }
      else printf("SHCreateDistributableObject: ERROR, couldn't instantiate object (no add-on file available?)\n");
   }
   else printf("SHCreateDistributableObject: ERROR, couldn't find SH_NAME_ADDONSPEC (\"%s\") field in archive msg!\n", SH_NAME_ADDONSPEC);
   
   return NULL;    // failure
}

void SHDeleteDistributableObject(SHDistributableObject * obj)
{      
   if (obj)
   {
      BList addOnTagList = obj->_addOnTags;
      obj->_addOnTags.MakeEmpty();
      delete obj;
      
      int num = addOnTagList.CountItems();
      for (int i=0; i<num; i++) delete ((shAddOnTag *)addOnTagList.ItemAt(i));
   }
}

uint32
SHGetArchitectureBits()
{
#if __INTEL__
   return SH_ARCH_BEOS_X86;
#elif __POWERPC__
   return SH_ARCH_BEOS_PPC;
#else
#error "Just what kind of computer IS this, anyway??"
#endif
}

status_t RecvFlatObject(BDataIO & io, BFlattenable & obj, int maxBytes)
{
   status_t ret = B_ERROR;  // default
   int32 numBytes;
 
   int sret;

   if ((sret = io.Read(&numBytes, sizeof(numBytes))) == sizeof(numBytes))
   {
      type_code typeCode;
      
      numBytes = ntohl(numBytes);
      
      int xBytes = 0;
      if ((numBytes > 0)&&(numBytes < maxBytes)&&((xBytes = io.Read(&typeCode, sizeof(typeCode))) == sizeof(typeCode)))
      {
         typeCode = ntohl(typeCode);
         char * bytes = new char[numBytes];
         int readBytes = io.Read(bytes, numBytes);
         if (readBytes == numBytes)
         {
             ret = obj.Unflatten(typeCode, bytes, numBytes);
             if (ret != B_NO_ERROR) printf("UnflattenRet Error!  (ret = %li)\n",ret);
         }
         else printf("readBytes 2 failed (ret=%i) (%s)\n", readBytes, strerror(errno));
         
         delete [] bytes;
      }
      else printf("ERROR: xBytes = %i, numBytes=%li (%s)\n", xBytes, numBytes, strerror(errno));
    } 
   else printf("RecvFlatObject:  size read failed! %i (%s)\n", sret, strerror(errno));
     
   return ret;
}

status_t SendFlatObject(BDataIO & io, const BFlattenable & obj, int maxBytes)
{
   int numBytes = obj.FlattenedSize();
   if ((numBytes <= 0)||(numBytes > maxBytes)) return B_ERROR;

   char * bytes = new char[numBytes];
   status_t ret = obj.Flatten(bytes, numBytes);

   if (ret == B_NO_ERROR)
   {
      // First send the size of the object.
      int32 netBytes = htonl(numBytes);  // to canonical net order
      if (io.Write(&netBytes, sizeof(netBytes)) == sizeof(netBytes))
      {
         // Then send its type_code (we assume type_code is a uint32!)
         uint32 typeCode = htonl(obj.TypeCode());

         if (io.Write(&typeCode, sizeof(typeCode)) == sizeof(typeCode))         
         {
            // Now send the flattened object itself.
            ret = (io.Write(bytes, numBytes) == numBytes) ? B_NO_ERROR : B_ERROR;
         }
         else ret = B_ERROR;
      }
      else ret = B_ERROR;
   }

   delete [] bytes; 
   return ret;
}


int32
GetNextConnectionID()
{
   static BLocker connectCountLock;
   static int32 count = 10;
   
   BAutolock m(connectCountLock);
   return(count++);
}


