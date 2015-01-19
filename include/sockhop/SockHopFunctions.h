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


#ifndef _SOCKHOPFUNCTIONS_H_
#define _SOCKHOPFUNCTIONS_H_

#include <app/Message.h>
#include <app/Messenger.h>
#include <sockhop/SHDistributableObject.h>
#include <sockhop/SHAccessPolicy.h>

#ifndef __INTEL__
#pragma export on
#endif 

// This header file lists the C-style functions that are exported by libsockhop.so.

// Returns a BLooper which represents the root node of the SockHop tree.  All normal BLooper 
// rules apply (e.g. you should call Run() on it to start it, then call Lock() and Quit() to stop it)
// If (optPolicy) is specified, it should be a freshly allocated SHAccessPolicy object
// that the created root node will assume ownership of and use for policy decisions.
// If (optPolicy) is NULL or not specified, an SHDefaultAccessPolicy object will be used.
_EXPORT BLooper * SHCreateRootNode(const BMessenger & repliesTo, SHAccessPolicy * optPolicy = NULL);

// This call is only necessary if you want to link SockHop add-on files into your code at 
// run time.  Since SockHop loads and unloads add-on images on demand, you have to use this 
// method instead of load_add_on(), and SHDeleteDistributableObject() instead of unload_add_on(), 
// so that you and SockHop won't unload each other's images at inopportune times.  The objects 
// will be created based on the BMessage you supply (it works the same as passing (archive) to 
// the Instantiate() method of the class named in the "class" field of (archive)).
// This function will look in the field SH_NAME_ADDONSPEC to determine which add-on file to load, 
// if necessary.  This function will not attempt to download the add-on file from the parent node, 
// however.
_EXPORT SHDistributableObject * SHCreateDistributableObject(const BMessage & archive);

// Every SHDistributableObject you create with SHCreateDistributableObject()
// should be later destroyed with SHDeleteDistributableObject().
// Don't use the delete operator on these objects (if you do, your add-ons won't get unloaded)
// On the other hand, it's okay to use SHDeleteDistributableObject() on SHDistributableObjects
// that were created with the new operator.
_EXPORT void SHDeleteDistributableObject(SHDistributableObject * deleteMe);

// Returns the SH_ARCH_* constant bitchord for the architecture that
// the code is currently running on.
_EXPORT uint32 SHGetArchitectureBits();

#ifndef __INTEL__
#pragma reset
#endif

#endif
