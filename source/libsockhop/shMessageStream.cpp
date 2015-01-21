
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


#include <support/Autolock.h>
#include "shMessageStream.h"
#include "shNode.h"

shMessageStream::
shMessageStream(const BMessenger & r, int32 threadPri)
: BLooper(NULL, threadPri), _replyTarget(r)
{
   // empty
}


shMessageStream::
~shMessageStream()
{
   // empty
}

void
shMessageStream::
SetReplyTarget(const BMessenger & newTarget)
{
   BAutolock m(_synchTarget);
   _replyTarget = newTarget;
}

status_t
shMessageStream::
PostReplyMessage(BMessage * msg)
{
   BAutolock m(_synchTarget);
   return(_replyTarget.SendMessage(msg));
}

