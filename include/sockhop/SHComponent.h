
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


#ifndef _SHCOMPONENT_H_
#define _SHCOMPONENT_H_

#include <sockhop/SHDistributableObject.h>

class shNode;
class SHWorker;
class shPendingLink;

///////////////////////////////////////////////////////////////////////////
//
// SHComponent
//
// This object represents an add-on that can interact directly with the 
// nodes on the SockHop tree.  It provides a common interface for subclasses 
// of SHWorker and SHSorter to access some of their node's functionality with.
//
///////////////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHComponent;
#else
#pragma export on
#endif 

class SHComponent : public SHDistributableObject
{
public:
   /////////////
   // Lifecycle
   /////////////
   
   SHComponent();
   
   virtual ~SHComponent();

   ///////////////////////////////////
   // SHDistributableObject interface
   ///////////////////////////////////
   
   /////////////////////////
   // BArchivable interface
   /////////////////////////

   SHComponent(BMessage * message);
   // Rehydration constructor
         
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of all subclass's Archive() methods;

   /////////////////////////////
   // SHComponent's new members
   /////////////////////////////
 
   virtual const char * GetName() const=0;
   // Subclasses must override this to return the name of this object.
   // Name strings don't have to be unique, but in general it's better
   // if they are--otherwise you may not be able to refer to them uniquely.

   const char * GetNodeName() const;
   // Returns the name (e.g. "myNode") of the node this component is 
   // running on.  Returns NULL if the component isn't currently attached 
   // to a node.
   
   const char * GetNodePath() const;
   // Returns the full path (e.g. "/path/of/my/Node") of the node this 
   // component is running on.  Returns NULL if the component isn't
   // currently attached to a node.

   BMessenger GetNodeMessenger() const;
   // Returns a BMessenger that can be used to send BMessages
   // to the node this SHComponent is running on.
   // If this SHComponent isn't currently running on a node,
   // then this method will return an invalid BMessenger.
      
private:
   friend class shNode;
   friend class SHWorker;
   friend class shPendingLink;  // sigh...
   
   virtual void setNode(shNode * _node);
      
   shNode * _node;

   /* FBC */
   virtual void _SHComponent1();
   virtual void _SHComponent2();
   virtual void _SHComponent3();
   virtual void _SHComponent4();
   uint32 _SHComponentData[8];
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
