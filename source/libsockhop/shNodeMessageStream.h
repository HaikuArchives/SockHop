
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


#ifndef _SHNODEMESSAGESTREAM_H_
#define _SHNODEMESSAGESTREAM_H_

#include "shMessageStream.h"
#include <sockhop/SHNodeSpec.h>
#include "shOperationTagHolder.h"

//A type of shMessageStream with extra data that shNode and
//friends can use on the "server side".
class shNodeMessageStream : public shMessageStream, public shOperationTagHolder
{
public:
  shNodeMessageStream(const BMessenger & replyTarget, int32 parentId, int32 id, const BMessage & startupMessage, const SHNodeSpec & spec, int32 threadPri);
  virtual ~shNodeMessageStream();
  
  int32 GetID() const;
  int32 GetParentID() const;
  
  const BMessage & GetStartupMessage();

  const SHNodeSpec & GetNodeSpec();  
  
  bool GetAndClearActivated();
  void SetActivated();
  
  int GetAndClearConnectTagID();
  void SetConnectTagID(int id);
  
  void AddFlag(uint32 flag);
  void RemoveFlag(uint32 flag);
  uint32 GetFlags() const;
  
protected:
  virtual status_t PostReplyMessage(BMessage * msg);
  
  int32 _connectTagID;  // so we can fire an shOperationTag on connect
  const int32 _id;
  const int32 _parentId;
  BMessage _startupMessage;
  const SHNodeSpec _nodeSpec;
  uint32 _flags;
  bool _activated;
};

#endif
