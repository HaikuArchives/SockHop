
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


#ifndef _SHCONNECTION_H_
#define _SHCONNECTION_H_

#include <kernel/OS.h>
#include <sockhop/SockHopConstants.h>
#include "shNodeMessageStream.h"
#include <sockhop/SHNodeSpec.h>

#define AUTORETRY(x) {while((x) == B_INTERRUPTED) {}}

class shSockIO;
class shSharedConnectionImp;

// Internal class to communicate over network streams.  Note that most of the work is done
// in the shSharedConnectionImp object that is passed in to this class... that's where the threads are based from.
class shConnection : public shNodeMessageStream
{
public:
   shConnection(shSharedConnectionImp * info, const BMessenger & replyTarget, int32 parentID, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, int32 threadPri, int32 batchEncodeMethod);
   virtual ~shConnection();
 
   virtual bool StartThreads();
   // Call this to start the connection threads, etc.
   // Returns true if the threads were started successfully, false on error.
  
   virtual void MessageReceived(BMessage * msg);
   // Passes (msg) to the sending thread.

   void SetTagMessage(const char * fieldName, const BMessage & msg);
   // If set, (msg) will be added to all BMessages sent to (replyTarget) by this shConnection,
   // under the field name (fieldName).  If (fieldName) is NULL, tagging will be turned off.

   void SetReplyTarget(const BMessenger & replyTarget);
   // In case you change your mind about where BMessages should be forwarded to.
   // Safe to call any time.
  
private:  
   shSharedConnectionImp * _info;
   int32 _threadPri;
   int32 _batchEncodeMethod;
};


// This little nugget is ref-count owned by the shConnection, the send thread,
// and the receive thread.  It is deleted in the Unref() method when the count reaches zero.
class shSharedConnectionImp
{
public:
   shSharedConnectionImp();
   ~shSharedConnectionImp();

   // Some utility methods
   static int HackBind(int s, struct sockaddr * addr, int size);

   virtual void PostNextMessage(BMessage * msg);
   // To be called by user threads when they have a message that needs to be sent over the
   // wire.  (msg) will be given to the send thread to send.   (msg) becomes property of
   // the send thread, so it should not be deleted by the caller of this method.

protected:
   virtual int SetupConnection() = 0;
   // Called by the send thread.  Should not return till the connection is completely set up,
   // or the setup has failed.  Return a socket fd on success, or -1 on failure.
 
   virtual bool OnMessageSend(BMessage & msgToSend);
   // Called by standard send loop, just prior to sending the BMessage.
   // Return true to send the BMessage, or false to drop it.
   // Default implementation always returns true.
  
   virtual bool OnMessageReceived(BMessage & msgToRecv);
   // Called by standard Receive Loop, just after receiving a BMessage.
   // Return true to send the BMessage to _replyTarget, or false to drop it.
   // Default implementation always returns true.

   virtual void OnConnectionOpened();
   // Called when the connection is fully functional.
   // Default method sends an SH_CODE_CONNECTIONOPEN message to (replyTarget).
  
   virtual void OnConnectionClosed();
   // Called when the connection is closed.
   // Default method sends an SH_CODE_CONNECTIONCLOSED message to (replyTarget).

   virtual void SetTagMessage(const char * fieldName, const BMessage & msg);
   // If set, (msg) will be added to all BMessages sent to (replyTarget) by this shConnection,
   // under the field name (fieldName).  If (fieldName) is NULL, tagging will be turned off.

   virtual void SetReplyTarget(const BMessenger & replyTarget);
   // In case you change your mind about where BMessages should be forwarded to.
   // Safe to call any time.  
  
   virtual bool StartThreads(int32 threadPri, int32 batchEncodeMethod);
   // Call this to start the connection threads, etc.
   // Returns true if the threads were started successfully, false on error.

   SHNodeSpec GetNodeSpec() const;
   // Returns the SHNodeSpec given to our shConnection's constructor.
      
private:
   void setNaglesAlgorithmEnabled(bool enabled);
   status_t ForwardMessageToTarget(BMessage & msg);  // Sends BMessage to _replyTarget, with ID tag, thread safe  

   void BatchEncodeMessages(BList & messageList);  // Called with 0 or more messages to be reduced (possibly)
   bool BatchDecodeMessage(BMessage * msg, BList & outList); // Should decode (msg) into many (that are new-allocated) and return true, or return false to indicate "just send the msg itself".
   
   friend class shConnection;

   bool Unref();
   // Decrements the reference count.  If the ref count reaches zero, deletes this object.
   // Returns true iff the object was deleted.
   
   void Ref();
   // Adds to the reference count.

   BLocker _lock;          // general lock for settings synchronization

   int _refCount;     

   BList * _sendList;      // Double-buffered list pointer (swap mechanism for efficiency)
   sem_id _sendSem;        // released by PostMessage() whenever a new message is given to send, by GetMoreSendMessages()

   char * _tagFieldName;   // Optional ID "tag" message, can be added to all outgoing BMessages.
   BMessage _tagMessage;

   BMessenger _replyTarget;  // Use this to send messages back to the shConnection!--set by parent
   int32 _id;                // Same as our parent shConnection's id--set by parent
   SHNodeSpec _spec;         // Same as our parent shConnection's SHNodeSpec--set by parent
   
   int _sockfd;              // set by the SetupConnection() method

   int32 _threadPri;         // Set by the StartThread() method
   int32 _batchEncodeMethod; // Set by the StartThread() method
      
   static long sendLoopStub(void * data);
   long sendLoop();
   // Standard sending loop:  Gets BMessages from the _sendList, and
   // pushes them out over the TCP connection.
   // This thread is started by StartThreads().
  
   static long recvLoopStub(void * data);
   long recvLoop();
   // Standard receiving loop:  Gets BMessages from the TCP connection,
   // add any ID messages to them, and send them to the replyTarget. 
   // This thread is started by Sending Thread.
};


#endif
