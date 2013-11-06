
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


#include <sockhop/SHAccessPolicy.h>
#include <sockhop/SHDirectConnection.h>

#include "shNode.h"

SHAccessPolicy ::
SHAccessPolicy()
{
   // empty
}

SHAccessPolicy ::
SHAccessPolicy(BMessage * archive)
   : SHComponent(archive)
{
   // empty
}

SHAccessPolicy ::
~SHAccessPolicy()
{
   // empty
}
   
void
SHAccessPolicy ::
OnServerStartup()
{
   // empty
}
   
void 
SHAccessPolicy ::
OnServerShutdown()
{
   // empty
}

void
SHAccessPolicy ::
OnNodeStartup()
{
   // empty
}
   
void 
SHAccessPolicy ::
OnNodeShutdown()
{
   // empty
}

SHAccessPolicy *
SHAccessPolicy ::
MakeNodePolicy()
{
   return NULL;
}

bool
SHAccessPolicy ::
OnSessionAccepted(SHDirectConnection & conn)
{
   return conn.Start();
}

status_t
SHAccessPolicy ::
Archive(BMessage * msg, bool deep) const
{
   return SHComponent::Archive(msg, deep);
}

int 
SHAccessPolicy ::
GetDefaultDebugLevel()
{
   return 0;
}

int32
SHAccessPolicy ::
GetDefaultThreadPriority()
{
   return B_NORMAL_PRIORITY;
}

int32
SHAccessPolicy ::
GetDefaultTransmissionEncoding()
{
   return B_NORMAL_PRIORITY;
}


/* FBC */
void SHAccessPolicy::_SHAccessPolicy1() {}
void SHAccessPolicy::_SHAccessPolicy2() {}
void SHAccessPolicy::_SHAccessPolicy3() {}
void SHAccessPolicy::_SHAccessPolicy4() {}
void SHAccessPolicy::_SHAccessPolicy5() {}
void SHAccessPolicy::_SHAccessPolicy6() {}
void SHAccessPolicy::_SHAccessPolicy7() {}
void SHAccessPolicy::_SHAccessPolicy8() {}
