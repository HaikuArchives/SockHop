
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


#ifndef _SHNODE_H_
#define _SHNODE_H_

#include "shConnection.h"
#include "shOperationTagHolder.h"

// BMessage field names for internal use only! 
#define SH_NAME_PATH          "shPath"
#define SH_NAME_FILESPEC      "shFileSpec"
#define SH_NAME_FILE          "shFile"
#define SH_NAME_TAGREF        "shTagRef"
#define SH_NAME_TAGUNREF      "shTagUnref"
#define SH_NAME_DONTSAVEFILE  "shDontSaveFile"
#define SH_NAME_LINKOPID      "shLinkOpID"
#define SH_NAME_NEWLINKID     "shNewLinkID"
#define SH_NAME_LINKTARGET    "shLinkTarget"
#define SH_NAME_GIVEONLYTOCID "shToCID"
#define SH_NAME_WORKERID      "shWorkerId"
#define SH_NAME_CALLBACKSPEC  "shCallback"
#define SH_NAME_POLICY        "shPolicy"

class SHComponent;
class SHSorter;
class SHWorker;
class SHFileSpec;
class shPendingDownload;
class shPendingLink;
class SHAccessPolicy;

class shNode : public shNodeMessageStream
{
public: 
   shNode(const BMessenger & parentAt, int32 parentId, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, bool takeAppWithMe, SHAccessPolicy * nodePolicy);
   virtual ~shNode();

   // returns the name of this node.  ("blah")
   const char * GetNodeName() const;
   
   // returns the full path of this node ("/localhost/sdcc8/blah")
   const char * GetNodePath() const;
   
   // These methods post onSuccess/onFailure/onCompletion messages if (msg) specified to do so.
   // if (optSpecific) is non-NULL, it will be added to the posted message, under the name "shRegarding".
   void PostSystemReplyMessages(const BMessage * regarding, int32 code, const BFlattenable * optSpecific);
   void PostSystemReplyMessagesWithString(const BMessage * regarding, int32 code, const char * optSpecific);
   void PostSystemReplyMessagesForSpec(BMessage * msg, int32 code, BMessage * archiveMessage);
   
   void AddSorter(SHSorter * sorter);
   void RemoveSorter(int index, bool nodeIsDying=false);

   void AddWorker(SHWorker * worker);
   bool RemoveWorker(int index);

   const char * GetChildNodeNameByCID(int cid) const;

   enum {
	SH_OPERATION_FAILED = 0,
	SH_OPERATION_SUCCEEDED,
	SH_OPERATION_DONE
   };

   void printf(const char * fmt, ...);  // for debugging; prints node name too
   
   // Just sets the _isParentLocal flag to true, for SHSorter's information
   void SetParentIsLocal();

   // Will return the SHComponent requested, or NULL if it isn't locally available.
   // Replaced by ::SHCreateDistributableObject() SHComponent * GetNewComponent(const SHFileSpec & nextSpec, BMessage * archive);

   // Returns the name of the SHWildPathSorter we use to send system messages.
   const char * GetSystemSorterName() const;

   static char * TypeToString(uint32 code);

   SHAccessPolicy & GetPolicy();
         
private:   
   void AddChild(shNodeMessageStream * child);
   void RemoveChild(int index, bool okayToSendNotifications);
   void RemoveIncomingLink(int index);
   
   // MessageReceived() calls MessageReceived*() as appropriate
   void MessageReceived(BMessage * msg);
  
   void MessageReceivedFromElsewhere(BMessage * msg, int32 cid);
   void MessageReceivedFromStreamObject(BMessage * msg, int32 cid);
   
   void PostSystemMessage(BMessage * msg);  // adds system-message-specific fields to (msg) and posts it to net

   SHSorter * RelayMessage(BMessage * msg, shOperationTag * optOpTag);
   // Returns NULL if the message is not to affect the local node, or the SHSorter to use
   // to distribute messages to the SHWorkers, if the message is to be applied locally.
      
   // Starts the file-caching process for the given SHFileSpec.
   // Returns true if the files are locally available.  If one or more of the files is
   // not available, it starts a pending-download, sets (*setDownload) to point to it, 
   // and returns false.
   // On error, it returns false, and (*setDownload) is set to NULL. 
   // If (forArch) is not specified, the result bits from SHGetArchitectureBits() will be used.
   bool CacheFile(const SHFileSpec & file, shPendingDownload ** setDownload);
   bool CacheFile(const SHFileSpec & file, shPendingDownload ** setDownload, uint32 forArch);
   
   // Returns the child with the given CID, or NULL if no such child
   // exists.  If (setIndex) is non-NULL, the int it points to is overwritten
   // with the returned child's index in the _children list.
   shNodeMessageStream * GetChildByCID(int cid, int * setIndex = NULL) const;
   shConnection * GetIncomingLinkByCID(int cid, int * setIndex = NULL) const;

   // Will return the SHComponent requested, or start a cache operation (and set *setOnOrder to true) if it isn't locally available.
   SHComponent * InstantiateComponent(BMessage * archive, BMessage * mainMsg, bool * setOnOrder, shOperationTag * optOpTag);
   
   // Finds (and optionally removes) shPendingLinks from our _pendingLinks list,
   // based on the SH_NAME_OP_ID field of the given BMessage.
   // Returns NULL if no such BMessage is found.
   shPendingLink * GetPendingLinkFrom(const BMessage * msg, bool removeFromListToo = false);
   
   // Returns true iff (name) is found and has been made the new default sorter
   bool SetDefaultSorterByName(const char * name);
   
   // Posts the given BMessage to the given child.
   // Makes sure our connectionID is added, if the child is local.
   status_t SendMessageToChild(BMessage * msg, shNodeMessageStream * child);
   
   int32 _opTagIDCounter;
   
   BList _children; // (shNodeMessageStream *)'s
   BList _sorters;  // (SHSorter *)'s
   BList _workers;  // (SHWorker *)'s

   // We can't just use (this) as the shNodeMessageStream, because our
   // parent may be using (this) to keep track of message tags sent to us!
   shOperationTagHolder _holdParentTags;  
   
   BList _pendingDownloads;  // (shPendingDownload *)'s
   
   BList _pendingLinks;  // (shPendingLink *)'s
   BList _links;         // (shConnection *)'s
   
   SHSorter * _defaultSorter; // points to an entry in the _sorters list
   SHSorter * _systemSorter;  // points to the wildpath sorter the shNode uses.
   
   char * _myPath;  // set in the NODEPATH message reception

   int32 _debugLevel;    // If 0, this->printf() is a no-op
   int32 _threadPri;     // priority to launch threads at
   int32 _batchEncoding; // encoding method to use
   
   bool _takeAppWithMe;  // If true, our destructor sends a B_QUIT_REQUESTED to be_app
   bool _waitingToDie;  // set when we are officially defunct but not dead yet
   bool _isParentLocal;
   
   SHAccessPolicy * _policy;
};

// Adds all items in (from)'s field (fieldName) from to (to)'s like-named field
status_t copyFieldTo(const char * fieldName, type_code dataType, const BMessage & from, BMessage & to);

// Add the given data to the given type of reply messages in the given msg.
void AddToReplyMessages(BMessage * msg, int32 code, const char * fieldName, type_code dataType, const void * data, ssize_t dataSize);

// Send and receive a flattened object over a "raw" TCP connection.
// Used in the startup handshaking.
// Either call will fail if the object is larger than (maxBytes) bytes.
status_t SendFlatObject(BDataIO & io, const BFlattenable & obj, int maxBytes);
status_t RecvFlatObject(BDataIO & io,       BFlattenable & obj, int maxBytes);

// Global connection ID counter, thread-safe.
int32 GetNextConnectionID();

// Handy debug checkpoint macro...
#define CP {::printf("CheckPoint: %s:%i (thread %i)\n", __FILE__, __LINE__, find_thread(NULL)); fflush(stdout);}

#endif
