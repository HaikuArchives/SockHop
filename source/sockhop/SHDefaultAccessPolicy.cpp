
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
#include <stdlib.h>
#include <sockhop/SHDefaultAccessPolicy.h>

#include "shNode.h"

SHDefaultAccessPolicy ::
SHDefaultAccessPolicy(int port, int debugLevel, const char * password, int32 threadPri, int32 batchEncoding)
   : _port(port), _debugLevel(debugLevel), _password(password ? strdup(password) : NULL), _threadPri(threadPri), _batchEncoding(batchEncoding)
{ 
   // empty
}
   
SHDefaultAccessPolicy ::
SHDefaultAccessPolicy(BMessage * archive)
   : SHAccessPolicy(archive), _port(SH_DEFAULT_PORT), _debugLevel(0), _password(NULL), _threadPri(B_NORMAL_PRIORITY), _batchEncoding(SH_ENCODING_NONE)
{ 
   const char * temp;

   if (archive->FindString("debug", &temp) == B_NO_ERROR)         
   {
      // A little undocumented hackery to allow "true" input as well, since I find it handy...
      if (strcmp(temp, "true") == 0) _debugLevel = 1;
                                else _debugLevel = atoi(temp);
   }
   
   if (archive->FindString("port", &temp) == B_NO_ERROR)          _port       = atoi(temp);
   if (archive->FindString("password", &_password) == B_NO_ERROR) _password   = strdup(_password);
   if (archive->FindString("priority", &temp) == B_NO_ERROR)      _threadPri  = atoi(temp);
   if (archive->FindString("encoding", &temp) == B_NO_ERROR)
   {
           if (strcmp(temp, "none")       == 0) _batchEncoding = SH_ENCODING_NONE;
      else if (strcmp(temp, "batch")      == 0) _batchEncoding = SH_ENCODING_BATCH;
      else if (strcmp(temp, "zlib")       == 0) _batchEncoding = SH_ENCODING_ZLIBLIGHT;
      else if (strcmp(temp, "zliblight")  == 0) _batchEncoding = SH_ENCODING_ZLIBLIGHT;
      else if (strcmp(temp, "zlibmedium") == 0) _batchEncoding = SH_ENCODING_ZLIBMEDIUM;
      else if (strcmp(temp, "zlibheavy")  == 0) _batchEncoding = SH_ENCODING_ZLIBHEAVY;
      else printf("SHDefaultAccessPolicy:  Warning, unknown encoding name [%s]\n", temp);
   }
}
   
SHDefaultAccessPolicy ::
~SHDefaultAccessPolicy()
{ 
   if (_password) ::free((void *)_password);
}
   
SHNodeSpec
SHDefaultAccessPolicy ::
GetListeningLocation()
{ 
   BMessage pwd;
   if (_password) pwd.AddString("password", _password);
   return(SHNodeSpec("", "", _port, pwd));
}
   
bool 
SHDefaultAccessPolicy ::
OkayToAcceptConnection(SHNodeSpec & connectingSpec)
{ 
   if (_password)
   {
      const char * theirPassword;
      if (connectingSpec.GetConstSpecMessage().FindString("password", &theirPassword) == B_NO_ERROR)
      {
         return(strcmp(_password, theirPassword) == 0);
      }
      else return false;
   }
   else return true;  // No password == accept anybody
}
 
bool
SHDefaultAccessPolicy ::
OkayToInstantiateObject(const BMessage &)
{
   return true;
}
   
bool    
SHDefaultAccessPolicy ::
OkayToWriteFile(const char *)
{ 
   return true;
}
   
bool 
SHDefaultAccessPolicy ::
OkayToReadFile(const char *)
{ 
   return true;
}

// SHComponent interface
const char *
SHDefaultAccessPolicy ::
GetName() const
{
   return "default";
}

// BArchivable interface   
BArchivable *
SHDefaultAccessPolicy ::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "SHDefaultAccessPolicy")) return NULL;
   return new SHDefaultAccessPolicy(archive);
}

   
status_t   
SHDefaultAccessPolicy ::
Archive(BMessage * archive, bool deep) const
{
   char temp[100];
   
   status_t ret = SHComponent::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
 
   // Note:  Store ALL data as strings to be compatible with the the 
   // BMessages that were created from the command line arguments!
     
   sprintf(temp, "%li", _port);
   archive->AddString("port", temp);
   
   sprintf(temp, "%li", _debugLevel);
   archive->AddString("debug", temp);
   
   if (_password) archive->AddString("password", _password);
   
   sprintf(temp, "%li", _threadPri);
   archive->AddString("priority", temp);

   const char * encodeName = "<error>";
   switch(_batchEncoding)
   {
      case SH_ENCODING_NONE:       encodeName = "none";         break;
      case SH_ENCODING_BATCH:      encodeName = "batch";        break;
      case SH_ENCODING_ZLIBLIGHT:  encodeName = "zliblight";    break;
      case SH_ENCODING_ZLIBMEDIUM: encodeName = "zlibmedium";   break;
      case SH_ENCODING_ZLIBHEAVY:  encodeName = "zlibheavy";    break;
   }   
   archive->AddString("encoding", encodeName);

   return ret;
}

int 
SHDefaultAccessPolicy ::
GetDefaultDebugLevel()
{
   return _debugLevel;
}

int32
SHDefaultAccessPolicy ::
GetDefaultThreadPriority()
{
   return _threadPri;
}

int32
SHDefaultAccessPolicy ::
GetDefaultTransmissionEncoding()
{
   return _batchEncoding;
}

void
SHDefaultAccessPolicy ::
OnServerStartup()
{
   if (_password == NULL) 
   {
      printf("\n= Warning, you didn't specify a password.  That means\n"
             "  ANYBODY could connect to your machine and do mean things\n"
             "  like delete all your files!  If you are not on an isolated,\n"
             "  trusted network, you might want to restart this server with\n"
             "  the password=<whatever> command line argument.\n\n");
   }
}

/* FBC */
void SHDefaultAccessPolicy::_SHDefaultAccessPolicy1() {}
void SHDefaultAccessPolicy::_SHDefaultAccessPolicy2() {}
void SHDefaultAccessPolicy::_SHDefaultAccessPolicy3() {}
void SHDefaultAccessPolicy::_SHDefaultAccessPolicy4() {}