
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


#include <sockhop/SHComponent.h>
#include "shAddOnTag.h"
#include "shNode.h"

SHComponent::
SHComponent(BMessage * archive)
   : SHDistributableObject(archive), _node(NULL)
{
   // empty
}

SHComponent::
SHComponent()
   : _node(NULL)
{
   // empty
}


SHComponent::
~SHComponent()
{
   // empty
}

const char *
SHComponent::
GetNodeName() const
{
   return(_node ? _node->GetNodeName() : NULL);
}

const char *
SHComponent::
GetNodePath() const
{
   return(_node ? _node->GetNodePath() : NULL);
}

BMessenger
SHComponent::
GetNodeMessenger() const
{
   return(_node ? BMessenger(_node) : BMessenger());
}

status_t
SHComponent::
Archive(BMessage * archive, bool deep) const
{
   return(SHDistributableObject::Archive(archive, deep));
}

void
SHComponent::
setNode(shNode * node)
{
   _node = node;
}

/* FBC */
void SHComponent::_SHComponent1() {}
void SHComponent::_SHComponent2() {}
void SHComponent::_SHComponent3() {}
void SHComponent::_SHComponent4() {}
