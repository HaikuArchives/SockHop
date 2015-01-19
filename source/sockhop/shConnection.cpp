
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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "socket.h"
#include "shConnection.h"
#include "shSockIO.h"
#include "shNode.h"
#include "SockHopInternalConstants.h"
#include <sockhop/SHDirectConnection.h>
#include <errno.h>
#include <support/Autolock.h>
#include <zlib.h>

static BLocker _exitLock;
static int _threadCount = 0;  // how many threads are currently active
static sem_id _wakeupExitWatcher = B_ERROR;  // will be demand-allocated
static BList _lingeringConnections;  // each send thread will add its shSharedConnectionImp here

// This method is called automatically when the main thread is about to exit, via the
// atexit() method.  It won't return until all the threads spawned with SpawnTrackedThread()
// have exited.  This avoids the problem of those threads crashing when the heap is pulled
// out from under them!
static void waitForLingeringThreads();
static void waitForLingeringThreads()
{
// HACK:  this code crashes under PPC.  No heap?
#ifdef __INTEL__
   // Tell any registered send threads that are still around that it's time to go!
   {
      BAutolock m(_exitLock);
      int num = _lingeringConnections.CountItems();
      for (int i=0; i<num; i++) ((shSharedConnectionImp *) _lingeringConnections.ItemAt(i))->PostNextMessage(NULL);
      _lingeringConnections.MakeEmpty();
   }
   
   while(1)
   {
      // check the semaphore & threadcount...
      {
         BAutolock m(_exitLock);
         if ((_threadCount == 0)||(_wakeupExitWatcher < B_NO_ERROR)) break;
      }
      
      // Safe to access _wakeupExitWatcher here because once it is set to a valid semaphore, 
      // it will never be modified again... and we verified in the critical section above that
      // it was set (if it wasn't, we wouldn't get here)
      acquire_sem(_wakeupExitWatcher);
   }      
#endif   
}

// Spawns a thread and increments the thread count to note that it is active.
static thread_id SpawnTrackedThread(thread_func func, const char * tname, int32 pri, void * arg, shSharedConnectionImp * optImp)
{
   BAutolock m(_exitLock);
   if (_wakeupExitWatcher < B_NO_ERROR)
   {
      // demand-allocate the watcher semaphore
      _wakeupExitWatcher = create_sem(0, NULL);
      if (_wakeupExitWatcher >= B_NO_ERROR) atexit(waitForLingeringThreads);
                                       else printf("ERROR:  Couldn't create _wakeupExitWatcher!!!\n");
   }   
   thread_id ret = spawn_thread(func, tname, pri, arg);
   if (ret >= B_NO_ERROR) 
   {
      _threadCount++;
      if (optImp) _lingeringConnections.AddItem(optImp);
   }
   return ret;   
}

// called by a TrackedThread when it exits.  Decrements the threadCount and kicks
// the semaphore so that waitForLingeringThreads() will wakeup and re-check the value.
static void TrackedThreadExiting(shSharedConnectionImp * optImp)
{
   BAutolock m(_exitLock);
   _threadCount--;
   if (_wakeupExitWatcher >= B_NO_ERROR) release_sem(_wakeupExitWatcher);  // signal him to check threadCount again
   if (optImp) _lingeringConnections.RemoveItem(optImp);
}

shConnection::
shConnection(shSharedConnectionImp * info, const BMessenger & replyTarget, int32 parentId, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, int32 threadPri, int32 batchEncodeMethod) 
  : _info(info), shNodeMessageStream(replyTarget, parentId, id, startupMessage, spec, threadPri), _threadPri(threadPri), _batchEncodeMethod(batchEncodeMethod)
{
   _info->_replyTarget = replyTarget;
   _info->_id          = _id;
   _info->_spec        = GetNodeSpec();
}

shConnection ::
~shConnection()
{   
   _info->PostNextMessage(NULL);   // tell the send thread to go away
   _info->Unref();  // remove our reference from _info so it will delete itself when the other threads exit
}

bool
shConnection ::
StartThreads()
{
   return _info->StartThreads(_threadPri, _batchEncodeMethod);
}

void
shConnection::
MessageReceived(BMessage * msg)
{ 
   BMessage * detachedMsg = DetachCurrentMessage();
   if (detachedMsg == msg) _info->PostNextMessage(detachedMsg);
   else 
   {
      // this code should never be executed, but just in case
      printf("shConnection:  ERROR: DetachCurrentMessage returned wrong message???\n");     
      delete detachedMsg;
   }
}

void
shConnection::
SetReplyTarget(const BMessenger & rT)
{
   _info->SetReplyTarget(rT);
}

void
shConnection ::
SetTagMessage(const char * fieldName, const BMessage & msg)
{
   _info->SetTagMessage(fieldName, msg);
}




/////////////////////////////////////////////////////////
// begin basic shared connection implementation code!
/////////////////////////////////////////////////////////

shSharedConnectionImp :: shSharedConnectionImp()
  : _refCount(1), _sendList(new BList), _sendSem(-1), _tagFieldName(NULL), _sockfd(-1)
    // _threadPri will be set by the StartThreads() method.
{
   // empty
}

// should only be called by Unref(), by which time all threads are guaranteed gone!
shSharedConnectionImp :: ~shSharedConnectionImp()
{
   delete [] _tagFieldName;
   if (_sendSem >= B_NO_ERROR) delete_sem(_sendSem);
   if (_sockfd >= B_NO_ERROR) closesocket(_sockfd);
   for (int i=_sendList->CountItems()-1; i>=0; i--) delete (BMessage *) _sendList->ItemAt(i);
   delete _sendList;
}

void 
shSharedConnectionImp ::
Ref()
{
   BAutolock m(_lock);
   _refCount++;
}

bool
shSharedConnectionImp ::
Unref()
{
   bool deleteMyself = false;
   {
      BAutolock m(_lock);
      deleteMyself = (--_refCount <= 0);
   }
   if (deleteMyself) delete this;
   return deleteMyself;
}

SHNodeSpec 
shSharedConnectionImp ::
GetNodeSpec() const
{
   return _spec;
}

bool 
shSharedConnectionImp :: 
StartThreads(int32 threadPri, int32 batchEncodeMethod)
{
   _threadPri = threadPri;
   _batchEncodeMethod = batchEncodeMethod;
   
   if ((_sendSem = create_sem(0, NULL)) >= B_NO_ERROR)
   {
      thread_id sendThread = SpawnTrackedThread(sendLoopStub, "SockHop TCP Send", _threadPri, this, this);
      if (sendThread >= B_NO_ERROR)
      {
         Ref();  // one for the send thread!
         if (resume_thread(sendThread) == B_NO_ERROR) 
         {
            return true;
         }
         else printf("shConnection:  ERROR resuming send thread!\n");
         kill_thread(sendThread);
         Unref();  // oops, nevermind
      }
      else printf("shConnection:  ERROR spawning send thread!\n");
   }
   else printf("shConnection: ERROR, couldn't allocate _sendSem!\n");
   return false;
}


void
shSharedConnectionImp ::
PostNextMessage(BMessage * msg)
{
   BAutolock lock(_lock);
   _sendList->AddItem(msg);
   release_sem(_sendSem);
}

long
shSharedConnectionImp::
sendLoopStub(void * data)
{
   long ret = ((shSharedConnectionImp *)data)->sendLoop();
   TrackedThreadExiting((shSharedConnectionImp *)data);
   return ret;
}

void shSharedConnectionImp :: setNaglesAlgorithmEnabled(bool enabled)
{               
#ifdef TCP_NODELAY_SUPPORTED  // come on Be, gimme a little love here!
   int delay = enabled ? 0 : 1;  
   setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &delay, sizeof(int));
#endif   
}

long
shSharedConnectionImp ::
sendLoop()
{
   _sockfd = SetupConnection();
   if (_sockfd >= 0)
   {
      thread_id recvThread = SpawnTrackedThread(recvLoopStub, "SockHop TCP Recv", _threadPri, this, NULL);  // recvThread will die when send thread dies so it doesn't need to register the shSharedConnectionImp
      if (recvThread >= 0)
      {
         Ref();  // one for the receive thread!
         if (resume_thread(recvThread) == B_NO_ERROR)
         {
            OnConnectionOpened();
   
            shSockIO sockIO(_sockfd);
            BList * nextBatch = new BList;     
            bool ok = true;
	        while(ok)
            {
               nextBatch->MakeEmpty();  // clear out dangling pointers from the last trip through this loop

 	           // The shConnection BLooper will release _sendSem when we have new messages to send.  We may sleep here till it does.
 	           acquire_sem(_sendSem);

               {
                  // grab the next batch o' messages
                  BAutolock lock(_lock);
                  BList * temp = _sendList;  // swap lists!  This way the lock need only be held briefly.  (double buffering!)
                  _sendList = nextBatch;
                  nextBatch = temp;
	           }

               BatchEncodeMessages(*nextBatch);  // here's where we can compress if we want to
               
               setNaglesAlgorithmEnabled(true);  // send the following messages in one packet if possible...
               int numMessages = nextBatch->CountItems();
               for (int i=0; i < numMessages; i++)
	           {
                  BMessage * msg = (BMessage *) nextBatch->ItemAt(i);
		
                  if (msg)
                  {
                     if ((ok)&&(OnMessageSend(*msg)))
                     {
                        long setMsgSize;
                        status_t result = (msg->what == SH_INTERNAL_RECEIVETHREADERROR) ? B_ERROR : msg->Flatten((BDataIO *)(&sockIO), &setMsgSize);
                        if (result != B_NO_ERROR) ok = false;  // This will cause us to delete the rest of the BMessages without sending them.
                     }                      
                     delete msg;  // BUG:  This crashes if the main thread has exited already?   How to fix this?
                  }
                  else ok = false;  // NULL message is our signal to leave--but we want to delete the rest of nextBatch's messages first.
               }    
               setNaglesAlgorithmEnabled(false);  // this will flush the send buffer so that everything gets sent right away.
            }      
            
            delete nextBatch;
         }
         else 
         {
            Unref();  // never mind, receive thread can't unref now so we'll have to do it
            printf("shSharedConnectionImp:  couldn't start receive thread!\n");
         }
      }
      else printf("shSharedConnectionImp:  couldn't spawn receive thread!\n");
   }
   else printf("shSharedConnectionImp:  SetupConnection failed!\n");

   OnConnectionClosed();          
   
   Unref();  // signal that we're done
   return 0;
}


long
shSharedConnectionImp::
recvLoopStub(void * data)
{
   long ret = ((shSharedConnectionImp *)data)->recvLoop();
   TrackedThreadExiting(NULL);
   return ret;
}

// If there is more than one BMessage in (list), replaces those 
// BMessages with a single SH_ENCODING_BATCH BMessage that contains all of them.
// NULLs in the list are handled correctly.
static void CoalesceMessages(BList & list);
static void CoalesceMessages(BList & list)
{
   BMessage * newMessage = NULL;  // demand-allocated
   bool foundNULL = false;  // set true if/when we find a NULL in the list

   int num = list.CountItems();
   for (int i=0; i<num; i++) 
   {
      BMessage * next = (BMessage *) list.ItemAt(i);
      if (next)
      {        
         if (foundNULL == false) 
         {
            if (newMessage == NULL) newMessage = new BMessage(SH_ENCODING_BATCH);  // demand-allocate!
            newMessage->AddMessage("m", next);
         }
         delete next;
      }
      else foundNULL = true;
   }
   list.MakeEmpty();
   if (newMessage) list.AddItem(newMessage);
   if (foundNULL) list.AddItem(NULL);  // add this back in so send loop will see it and quit
}


// Creates and returns a new BMessage that contains (message) in compressed form,
// and deletes (message).  May just return (message) itself if the compressed form
// would actually be larger than the uncompressed form!
static BMessage * MakeZlibCompressedMessage(BMessage * message, int level);
static BMessage * MakeZlibCompressedMessage(BMessage * message, int level)
{
   BMessage * ret = message;  // default:  compression doesn't work and we stick with the original
   ssize_t flatSize = message->FlattenedSize();
   
   char * buf = new char[flatSize];
   if (message->Flatten(buf, flatSize) == B_NO_ERROR)
   {
      ssize_t compressedDataSize = flatSize + ((int)((flatSize+12) * 0.01f)) + 12;  // "must be at least 0.1% larger than sourceLen plus 12 bytes" -- nice ambiguous sentence structure, eh?
      char * compressedBuf = new char[compressedDataSize];
      
      int zRet;
      if ((zRet = compress2((unsigned char *)compressedBuf, (unsigned long *) &compressedDataSize, (unsigned char *)buf, flatSize, level)) == Z_OK)
      {
         BMessage * zMessage = new BMessage(SH_ENCODING_ZLIBLIGHT);  // all ZLIB modes are transmitted with the ZLIBLIGHT tag.
         zMessage->AddData("z", B_ANY_TYPE, compressedBuf, compressedDataSize);
         zMessage->AddInt32("s", flatSize);
      
         ssize_t compSize = zMessage->FlattenedSize();
         if (compSize >= flatSize)
         {
            // printf("Oops, our compression expanded the message from %li to %li!  Using uncompressed message...\n", flatSize, compSize);
            delete zMessage;
         }
         else
         {
            // printf("Compressed message from %li to %li.  Using compressed message.\n", flatSize, compSize);
            delete message;
            ret = zMessage;
         }
      }
      else printf("MakeZlibCompressedMessage: compress() failed, errorcode = %i\n",zRet);
            
      delete [] compressedBuf;
   }   
   else printf("MakeZlibCompressedMessage:  couldn't Flatten() message!\n");
   
   delete [] buf;   
   return ret;
}

// Creates and returns a new BMessage based on the "z" compressed-data-field of (msg),
// which must be a BMessage that was created previously by MakeZlibCompressedMessage.
// Returns NULL on failure.  
// Note that unlike MakeZlibCompressedMessage(), this method does NOT delete the BMessage you pass in to it.
static BMessage * MakeZlibUncompressedMessage(BMessage * msg);
static BMessage * MakeZlibUncompressedMessage(BMessage * msg)
{
   BMessage * ret = NULL;  // default is failure
   
   if (msg->what == SH_ENCODING_ZLIBLIGHT)  // all zlib-compressed messages are transmitted with the ZLIBLIGHT tag
   {
      const void * data;
      ssize_t numBytes;
      ssize_t uncompressedSize;
            
      if ((msg->FindData("z", B_ANY_TYPE, &data, &numBytes) == B_NO_ERROR)&&
          (msg->FindInt32("s", &uncompressedSize) == B_NO_ERROR))
      {
         char * buf = new char[uncompressedSize];
         
         int zRet;
//printf("uncompress:  uncompressedSize = %li, compressedSize = %li\n", uncompressedSize, numBytes);
         if ((zRet = uncompress((unsigned char *) buf, (unsigned long *) &uncompressedSize, (unsigned char *)data, (unsigned long) numBytes)) == Z_OK)
         {         
            BMessage * unflattened = new BMessage;
            if (unflattened->Unflatten(buf) == B_NO_ERROR) ret = unflattened;  // success!
            else 
            {
               printf("MakeZlibUncompressedMessage:  Couldn't Unflatten compressed data!\n");
               delete unflattened;
            }
         }
         else printf("MakeZlibUncompressedMessage:  uncompress() failed, ret=%i\n",zRet);
         
         delete [] buf;   
      }
   }
   return ret;
}

// A chance to compact the messages in (messageList) before we send them.
// All messages in (messageList) were allocated with "new", and may be deleted by this method.
void shSharedConnectionImp :: BatchEncodeMessages(BList & messageList)
{
   int numMessages = messageList.CountItems();
   
   switch(_batchEncodeMethod)
   {
      case SH_ENCODING_BATCH:
         if (numMessages > 1) CoalesceMessages(messageList);
      break;

      case SH_ENCODING_ZLIBLIGHT:
      case SH_ENCODING_ZLIBMEDIUM:
      case SH_ENCODING_ZLIBHEAVY:
      {
         int level = 1;
         switch(_batchEncodeMethod) 
         {
            case SH_ENCODING_ZLIBLIGHT:  level = 1;  break;
            case SH_ENCODING_ZLIBMEDIUM: level = 6;  break;
            case SH_ENCODING_ZLIBHEAVY:  level = 9;  break;
         }
                  
         // Batch all the messages up into one big message!         
         if (numMessages > 1) 
         {
            // Batch the messages together, then compress them
            CoalesceMessages(messageList);
            BMessage * firstItem = (BMessage *) messageList.ItemAt(0);
            if (firstItem) messageList.ReplaceItem(0, MakeZlibCompressedMessage(firstItem, level));
         }
         else if (numMessages == 1)
         {
            // Just compress the message directly
            BMessage * firstItem = (BMessage *) messageList.ItemAt(0);
            if (firstItem) messageList.ReplaceItem(0, MakeZlibCompressedMessage(firstItem, level));
         }         
      }
      break;
      
      case SH_ENCODING_NONE:
         // do nothing!
      break;
            
      default:
         printf("shSharedConnectionImp:  Error, unknown encoding method %li\n", _batchEncodeMethod);         
      break;
   }   
}

// A chance to split (msg) into a list of BMessages.
// Should return (true) if any new BMessages were put into (outList) to use in lieu of (msg),
// or (false) if (msg) should be used as-is.
bool shSharedConnectionImp :: BatchDecodeMessage(BMessage * msg, BList & outList)
{   
   switch(msg->what)
   {
      case SH_ENCODING_ZLIBLIGHT:   // all message are transmitted with the ZLIBLIGHT tag
      {
         BMessage * uncompressedMessage = MakeZlibUncompressedMessage(msg);
         if (uncompressedMessage)
         {
            if (BatchDecodeMessage(uncompressedMessage, outList)) 
            {
               // Messages have been put in (outList)--our uncompressedMessage was an intermediate form.
               delete uncompressedMessage;
            }
            else
            {
               // (uncompressedMessage) is the real thing... use it!
               outList.AddItem(uncompressedMessage);
            }
         }
         else printf("BatchDecodeMessage:SH_ENCODING_ZLIB:  Error uncompressing message!\n");
      }
      return true;
      
      case SH_ENCODING_BATCH:
      {
         int32 count;
         type_code type;
         
         if ((msg->GetInfo("m", &type, &count) == B_NO_ERROR)&&
             (type == B_MESSAGE_TYPE))
         {
            for (int32 i=0; i<count; i++)
            {
               BMessage * next = new BMessage;
               if (msg->FindMessage("m", i, next) == B_NO_ERROR) outList.AddItem(next);
               else
               {
                  printf("BatchDecodeMessage:SH_ENCODING_BATCH:  Error unflattening batch msg %li/%li!\n", i, count);
                  delete next;
               }               
            }
         }
         else printf("BatchDecodeMessage:SH_ENCODING_BATCH:  Error, didn't find any submessages!\n");
      }
      return true;
      
      case SH_ENCODING_NONE: 
         printf("shSharedConnectionImp:  Error, got SH_ENCODING_NONE BMessage????  That shouldn't happen!\n");
      // fall-thru
      default:
         return false;  // means "don't use outList, just send (msg) out"
   }
}

long
shSharedConnectionImp::
recvLoop()
{
   shSockIO io(_sockfd, &_refCount, 3, 1);  // Tell the receive code to poll the _refCount every 1 second, and make sure it's still 3.  It'll error out if it's not.
   BMessage nextMsg;
   BList messageList;
   
   while(1)
   {
      messageList.MakeEmpty();      
      if (nextMsg.Unflatten(&io) != B_NO_ERROR) break;
      
      bool useBatchList = BatchDecodeMessage(&nextMsg, messageList);  // Was that one message holding others?
      if (useBatchList)
      {
         bool breakWhenDone = false;
         
         // If so, send each one, then delete it.
         int num = messageList.CountItems();
         for (int i=0; i<num; i++)
         {
            BMessage * nextInBatch = (BMessage *) messageList.ItemAt(i);
            if ((breakWhenDone == false)&&(OnMessageReceived(*nextInBatch))) 
            {
               BAutolock m(_lock);
               if (ForwardMessageToTarget(*nextInBatch) != B_NO_ERROR) breakWhenDone = true;
            }
            delete nextInBatch;         
         }    
         if (breakWhenDone) break;
      }
      else
      {      
         // Just send the one message directly
         if (OnMessageReceived(nextMsg)) 
         {
            BAutolock m(_lock);
            if (ForwardMessageToTarget(nextMsg) != B_NO_ERROR) break;
         }
      }
   }
   PostNextMessage(NULL);  // This will ensure the send thread exits as well--and he'll send the CONNECTION_CLOSED message.
   Unref();
   return 0;
}


status_t
shSharedConnectionImp ::
ForwardMessageToTarget(BMessage & msg)
{            
   (void)msg.RemoveName(SH_NAME_CONNECTIONID);  // to avoid spoofing

   BAutolock m(_lock);
   if (_id != -1) msg.AddInt32(SH_NAME_CONNECTIONID, _id);   
   if (_tagFieldName) msg.AddMessage(_tagFieldName, &_tagMessage);
   return _replyTarget.SendMessage(&msg);
}            


bool
shSharedConnectionImp ::
OnMessageReceived(BMessage &)
{      
   return true;
}

bool
shSharedConnectionImp ::
OnMessageSend(BMessage & msg)
{      
   (void)msg.RemoveName(SH_NAME_CONNECTIONID);  // unnecessary to transmit this, it will be stripped on receive anyway
   return true;
}

void
shSharedConnectionImp::
OnConnectionClosed()
{
   BMessage msg(SH_CODE_CONNECTIONCLOSED);   
   ForwardMessageToTarget(msg);   
}

void
shSharedConnectionImp::
OnConnectionOpened()
{
   BMessage msg(SH_CODE_CONNECTIONOPEN);   
   ForwardMessageToTarget(msg);
}

void
shSharedConnectionImp::
SetReplyTarget(const BMessenger & rT)
{
   BAutolock m(_lock);
   _replyTarget = rT; 
}

void
shSharedConnectionImp ::
SetTagMessage(const char * fieldName, const BMessage & msg)
{
   BAutolock m(_lock);
   if (_tagFieldName) ::free(_tagFieldName);
   _tagFieldName = NULL;
   if (fieldName) 
   {
      _tagFieldName = new char[strlen(fieldName)+1];
      strcpy(_tagFieldName, fieldName);
   }
   _tagMessage = msg;
}

#define BIND_HACK 1

// Use this till I figure out why ::bind(INADDR_ANY) doesn't work
int
shSharedConnectionImp ::
HackBind(int s, struct sockaddr * addr, int size)
{
   // This call lets us bind even to sockets that have been used quite recently.
   int option = 1;
   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof(option)); 
    
#ifdef BIND_HACK
   const int MIN_HACK_RANDOM_PORT_NUMBER = 20000;
   const int MAX_HACK_RANDOM_PORT_NUMBER = 25000;

   if (((struct sockaddr_in *)addr)->sin_port == htons(0))
   {
      for (int i= MIN_HACK_RANDOM_PORT_NUMBER; i <= MAX_HACK_RANDOM_PORT_NUMBER; i++)
      {
         ((struct sockaddr_in *)addr)->sin_port = htons(i);
         if (bind(s, addr, size) == B_NO_ERROR) return(B_NO_ERROR);
      } 
      printf("Couldn't hackBind a port!\n");
      return(B_ERROR);
   }
   else 
#endif
   return bind(s, addr, size);
}

