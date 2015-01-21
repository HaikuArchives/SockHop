
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


#ifndef SHPENDINGDOWNLOAD_H
#define SHPENDINGDOWNLOAD_H

#include <sockhop/SHFileSpec.h>
#include "shOperationTagHolder.h"

class shPendingDownload;   // forward declaration
class shNode;

// Code to be executed when a download finishes
class shPendingDownloadCallback
{
public:
   shPendingDownloadCallback(const BMessage & msg);
   virtual ~shPendingDownloadCallback();
   
   // This default version just calls PostSuccessMessage()
   // Returns the cid of a child to send the shFile to, or -1 for no child.
   virtual int DoCallback();
   
   // These will post the appropriate success/failure message, if they have it.
   virtual void PostSuccessMessage();
   virtual void PostFailureMessage();
   
   // Subclasses may have read-only access to our contained data.
   shPendingDownload * GetOwner() const;
   BMessage * GetMessage();
    
private:
   friend class shPendingDownload;
   shPendingDownload * _owner;  // set by shPendingDownload::AddCallback(this);
   BMessage _msg;
};

class shInstantiateOnDownloadCallback : public shPendingDownloadCallback
{
public:
   shInstantiateOnDownloadCallback(const BMessage & msg,
                                   const BMessage & archiveMsg);
   ~shInstantiateOnDownloadCallback();
   
   virtual int DoCallback();
   
   // adds the string of the components name to the messages, instead of the SHFileSpec
   virtual void PostSuccessMessageWithString(const char * compName);
   
private:
   BMessage _archive;
};

class shForwardOnDownloadCallback : public shPendingDownloadCallback
{
public:
   shForwardOnDownloadCallback(const BMessage & msg,
                               uint32 cid);
   ~shForwardOnDownloadCallback();
   
   virtual int DoCallback();
   
private:
   uint32 _cid;
};

class shOpTagCallback : public shPendingDownloadCallback
{
public:
   shOpTagCallback(const BMessage & msg, shOperationTagHolder * holder, shOperationTag * tag);
   ~shOpTagCallback();
   
   virtual int DoCallback();
   virtual void PostFailureMessage();
   
private:
   uint32 _tagID;
   shOperationTagHolder * _holder;
};

// Tracks everything associated with a pending download.
class shPendingDownload
{
public: 
   shPendingDownload(shNode * node, const SHFileSpec & file);
   ~shPendingDownload();
   
   void AddCallback(shPendingDownloadCallback * cb);
   
   // Calls DoCallback() on all attached callbacks
   void DoCallbacks(BMessage * file);
   
   // Calls SendFailureMessage() on all attached callbacks
   void PostFailures();
   
   // FileSpec as it was when this shPendingDownload was created
   const SHFileSpec & GetFileSpec() const;
   
   // FileSpec that will be modified as parts of the fileSpec are satisfied.
   SHFileSpec & GetModifiedFileSpec();
   
   shNode * GetNode() const;
   
private:
   shNode * _node;
   const SHFileSpec _fileSpec;
   SHFileSpec _modFileSpec;
   BList _callbackList;
};


#endif