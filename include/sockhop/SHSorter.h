
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


#ifndef _SHSORTER_H_
#define _SHSORTER_H_

#include <support/Archivable.h>

#include <sockhop/SHNodeSpec.h>
#include <sockhop/SHComponent.h>

////////////////////////////////////////////////////////////////
//
// SHSorter
//
// This abstract base class represents an object which knows how
// to route BMessages around the SockHop node tree.  The SHWildPathSorter
// subclass of this class is most commonly used, but you may create
// your own SHSorter subclasses if you need different routing
// capabilities.
//
////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHSorter;
#else
#pragma export on
#endif 


class SHSorter : public SHComponent
{
public:
   /////////////
   // Lifecycle
   ///////////// 
   
   SHSorter();
   // Default constructor
   
   virtual ~SHSorter();

   /////////////////////////
   // SHComponent interface
   /////////////////////////
   
   ///////////////////////////////////
   // SHDistributableObject interface
   ///////////////////////////////////
   
   /////////////////////////
   // BArchivable interface
   /////////////////////////
   
   SHSorter(BMessage * archive);
   // Rehydration constructor

   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of all subclass's Archive() methods

   //////////////////////////
   // SHSorter's new members 
   //////////////////////////
           
   virtual bool DoesMessageGoToNode(BMessage & msg, const SHNodeSpec & potentialTarget, uint32 flags)=0;
   // Should be implemented to return true iff (msg) should to be forwarded to the adjacent SockHop
   // node represented by the given (potentialTarget).
   // This method will be called for all nodes (parent, children, and symlinks) that are directly 
   // connected to this SHSorter's node, for each BMessage that is given to this SHSorter.
   // (flags) is a bitchord that is a combination of the following bits of information:
   //  SH_FLAG_IS_PARENT  - Set iff the given target is the current node's parent node.
   //  SH_FLAG_IS_LOCAL   - Set iff the given target is running in the same process space as the current node.
   //  SH_FLAG_IS_SYMLINK - Set iff the given target is a symbolic link, not an actual child.
   
   virtual bool DoesMessageDistributeLocally(BMessage & msg)=0;
   // Should be implemented to return true iff (msg) should be given to the SHWorkers on this node.
   // This method is called once per received BMessage, after all the 
   // DoesMessageGoTo(BMessage,SHNodeSpec,uint32) calls have been done, but before 
   // the beforeMessageRelay() call.

   virtual void BeforeMessageRelay(BMessage & msg);
   // Called after all the doesMessageGoTo(BMessage,SHNodeSpec,uint32) checks are done, and
   // after doesMessageDistributeLocally() has been called, but before the BMessage has actually 
   // been relayed to any child nodes or SHWorkers.
   // Default implementation does nothing, but you may override this if you wish to modify 
   // the BMessage (or whatever) before sending it onwards.

   virtual void BeforeLocalMessageDistribute(BMessage & msg);
   // Called after (msg) has been relayed to other nodes, but before (msg) is distributed
   // to local SHWorkers.  Only called if DoesMessageDistributeLocally() returned
   // true for this BMessage.  Default method does nothing; override if you want.
   
   virtual bool DoesMessageGoToWorker(BMessage & msg, const char * workerName);
   // If you want your sorter to choose which SHWorkers to send the BMessage to,
   // you can override this method to return true iff the message should go
   // to the worker with the given name.
   // By default, this method always returns true, hence the message will go
   // to all interested workers.      
   // Note: This method is only called if DoesMessageDistributeLocally() returned 
   // true for this BMessage.
   
private:
   /* FBC */
   virtual void _SHSorter1();
   virtual void _SHSorter2();
   virtual void _SHSorter3();
   virtual void _SHSorter4();
   uint32 _SHSorterData[8];
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
