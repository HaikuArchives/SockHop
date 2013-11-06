
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
#include <string.h>
#include <malloc.h>
#include <sockhop/SHNodeSpec.h>
#include "shNode.h"

//:Describes a host node to be connected to.
SHNodeSpec::
SHNodeSpec()
{
   clear(false);
}  

SHNodeSpec::
SHNodeSpec(const SHNodeSpec & copyMe)
{
   clear(false);
   *this = copyMe;
}

SHNodeSpec::
SHNodeSpec(const char * nodeName)
{
   clear(false);
   SetNodeName(nodeName);
}

SHNodeSpec::
SHNodeSpec(const char * nodeName, const BMessage & specMessage)
{
   clear(false);
   SetNodeName(nodeName);
   _specMessage = specMessage;
}

SHNodeSpec::
SHNodeSpec(const char * nodeName, const char * hostName)
{
   clear(false);
   SetNodeName(nodeName);
   SetHostName(hostName);
}

SHNodeSpec::
SHNodeSpec(const char * nodeName, const char * hostName, const BMessage & specMessage)
{
   clear(false);
   SetNodeName(nodeName);
   SetHostName(hostName);
   _specMessage = specMessage;
}

SHNodeSpec::
SHNodeSpec(const char * nodeName, const char * hostName, int portNum)
{
   clear(false);
   SetNodeName(nodeName);
   SetHostName(hostName);
   SetPortNumber(portNum);
}

SHNodeSpec::
SHNodeSpec(const char * nodeName, const char * hostName, int portNum, const BMessage & specMessage)
{
   clear(false);
   SetNodeName(nodeName);
   SetHostName(hostName);
   SetPortNumber(portNum);
   _specMessage = specMessage;
}

 
SHNodeSpec::
~SHNodeSpec()
{
   clear(true);
}
 
bool
SHNodeSpec::
IsNodeNameValid() const
{
   const char * n = GetNodeName();
   int len = strlen(n);
   
   if ((len == 0)||(len >= SH_MAX_NODENAME_LENGTH)) return(false);
   if (strcmp(n, ".") == 0) return(false);
   if (strcmp(n, "..") == 0) return(false);
   if (strchr(n, '/')) return(false);
   return(true);
}


type_code
SHNodeSpec::
TypeCode() const
{
   return SH_NODESPEC_TYPECODE;
}

void
SHNodeSpec ::
MakeFlatMessage(BMessage & msg) const
{
   msg.MakeEmpty();
   msg.what = TypeCode();
   if (_nodeName) msg.AddString("shNodeName", _nodeName);
   if (_hostName) 
   {
      msg.AddString("shHostName", _hostName);
      msg.AddInt32("shPortNum", _portNum);
   }
   msg.AddMessage("shSpecMessage", &_specMessage);
}

status_t 
SHNodeSpec::
Flatten(void * b, ssize_t numBytes) const
{
   BMessage msg;
   MakeFlatMessage(msg);
   return(msg.Flatten(((char *)b), numBytes));
}

bool
SHNodeSpec::
IsFixedSize() const
{
   return false;
}

status_t 
SHNodeSpec::
Unflatten(type_code code, const void * b, ssize_t)
{
   if (code != TypeCode()) return(B_ERROR);
   
   BMessage msg;
   status_t ret = msg.Unflatten((const char *)b);
   if (ret != B_NO_ERROR) return ret;
   
   clear(true);
   const char * str;
   if (msg.FindString("shNodeName", &str) == B_NO_ERROR) SetNodeName(str);
   if (msg.FindString("shHostName", &str) == B_NO_ERROR)
   {
      SetHostName(str);
      int32 port;
      if (msg.FindInt32("shPortNum", &port) == B_NO_ERROR)
      {
         SetPortNumber(port);
      }
   }
   (void)msg.FindMessage("shSpecMessage", &_specMessage);
   return B_NO_ERROR;
}

// Format is:  1 byte that is non-zero if string is NULL, else zero.
// If byte is non-zero, then string follows, else next byte follows.
// _specMessage BMessage at the end.
ssize_t
SHNodeSpec::
FlattenedSize() const
{
   BMessage msg;
   MakeFlatMessage(msg);
   return msg.FlattenedSize();
}

void 
SHNodeSpec::
SetHostName(const char * newHostName)
{
   if (_hostName) free(_hostName);
   _hostName = strdup(newHostName);
}

const char * 
SHNodeSpec::
GetHostName() const
{
   return _hostName ? _hostName : "";
}

void 
SHNodeSpec::
SetNodeName(const char * newNodeName)
{
   if (_nodeName) free(_nodeName);
   _nodeName = strdup(newNodeName);
}

const char * 
SHNodeSpec::
GetNodeName() const
{
   return _nodeName ? _nodeName : GetHostName(); 
}

void 
SHNodeSpec::
SetPortNumber(uint32 newPortNum)
{
   _portNum = newPortNum;
}

uint32 
SHNodeSpec::
GetPortNumber() const
{
   return (_hostName ? _portNum : 0);
}

void
SHNodeSpec::
clear(bool freeMem)
{
   if (freeMem)
   {
      if (_hostName) free(_hostName);
      if (_nodeName) free(_nodeName);
   }
   _hostName = NULL;
   _nodeName = NULL;
   _portNum  = SH_DEFAULT_PORT;
   _specMessage.MakeEmpty();
}

SHNodeSpec &
SHNodeSpec ::
operator =(const SHNodeSpec & copyMe)
{
   clear(true);
   SetHostName(copyMe._hostName);
   SetNodeName(copyMe._nodeName);
   SetPortNumber(copyMe._portNum);
   _specMessage = copyMe._specMessage;
   return(*this);  
}

bool
SHNodeSpec ::
operator ==(const SHNodeSpec & compareMe) const
{
   if (GetPortNumber() != compareMe.GetPortNumber())   return false;
   if (strcmp(GetHostName(), compareMe.GetHostName())) return false;
   if (strcmp(GetNodeName(), compareMe.GetNodeName())) return false;
   // Note that we DON'T compare _specMessage's here!
   return true;
}

bool
SHNodeSpec ::
operator !=(const SHNodeSpec & compareMe) const
{
   return(!(*this == compareMe));
}

void
SHNodeSpec ::
PrintToStream() const
{
   printf("SHNodeSpec:  nodename=[%s], hostname=[%s], portNum=[%li]\n",
          GetNodeName(), GetHostName(), GetPortNumber());
   printf("SHNodeSpec:  SpecMessage is:\n");
   _specMessage.PrintToStream();
}

BMessage &
SHNodeSpec ::
GetSpecMessage()
{
   return _specMessage;
}


const BMessage &
SHNodeSpec ::
GetConstSpecMessage() const
{
   return _specMessage;
}