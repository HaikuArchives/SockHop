
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


#ifndef _SHPENDINGLINK_H_
#define _SHPENDINGLINK_H_

#include "shNode.h"
#include <sockhop/SHSessionAcceptor.h>

class shPendingLink : public SHSessionAcceptor
{
public:
   shPendingLink(shNode * node, const BMessage & msg, shOperationTag * optOpTag);
   //If (optOpTag) is non-NULL, references it once.
   
   ~shPendingLink();
   //If (optOpTag) is non-NULL, unreferences it once.
   
   const BMessage & GetMessage() const;
   //Returns a read-only reference to our BMessage.

   shOperationTag * GetOpTag() const;
   //Returns a pointer to our opTag.
   
   void PostFailureMessage(const char * optFrom);
   // Causes us to post _msg's failure message from _node.
   // If (optFrom) is non-NULL, it will be added to the SH_NAME_REGARDING field.

   void PostSuccessMessage(const char * optFrom);
   // Causes us to post _msg's success message from _node.
   // If (optFrom) is non-NULL, it will be added to the SH_NAME_REGARDING field.

   shNode * GetNode() const;
   // Used by link acceptor policy...
   
private:
   friend class SHSessionAcceptor;
   
   SHAccessPolicy * _policy;
   shNode * _node;
   BMessage _msg;
   shOperationTag * _opTag;
   BMessage _prevSpecMsg;  // holds SpecMessage from last connection attempt.
};

#endif