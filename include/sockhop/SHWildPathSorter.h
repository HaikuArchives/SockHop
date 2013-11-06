
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


#ifndef _SHWILDPATHSORTER_H_
#define _SHWILDPATHSORTER_H_

#include <sockhop/SHSorter.h>

/////////////////////////////////////////////////////////////
//
// SHWildPathSorter
//
// This SHSorter subclass is the standard, default SHSorter
// used by SockHop to route BMessages around the node tree.
// It sorts based on the contents of the SH_NAME_TO and
// SH_NAME_TOWORKERS field in the BMessages.
//
// Examples of possible SH_NAME_TO values and their effects:
//
// "/"    - Send BMessage to root node only
// "/Joe" - Send BMessage to node Joe, located under root node
// "Joe"  - Send BMessage to node Joe, located under current node
// ".."   - Send BMessage to the parent node of the current node
// "."    - Send BMessage to the current node
// "/.."  - Send BMessage to the originating user code (above the root node)
// "/*"   - Send BMessage to all nodes directly under root node
// "/Joe/J*" - Send BMessage to all nodes under Joe whose node names
//             begin with the letter 'J'
//
// Note that there may be no SH_NAME_TO strings in the BMessage,
// in which case it will be broadcast to all nodes in the subtree
// including and below the posting node.
//
// Also, there may be more than one SH_NAME_TO string in the BMessage,
// in which case the BMessage will go to the union of the nodes specified
// in the array of strings.
//
// By default, the BMessage will be sent to every SHWorker on each
// node specified above.  To modify this, you may add an SH_NAME_TOWORKERS
// field to the BMessage, which specifies the name(s) of SHWorkers to
// give the BMessage to:
//
// "Worker17" - Give BMessage to workers named "Worker17" only
// "Worker*"  - Give BMessage to workers whose name starts with "Worker"
//
// This field may also contain an array of strings, in which case the
// BMessage is sent to the union of SHWorkers specified in all strings.
//
//////////////////////////////////////////////////////////////////////

#ifdef __INTEL__
_EXPORT class SHWildPathSorter;
#else
#pragma export on
#endif 

class SHStringMatcher;

class SHWildPathSorter : public SHSorter 
{
public:
   SHWildPathSorter();
   // Default constructor.
   // NOTE:  You don't usually have to construct an SHWildPathSorter
   // object explicitely:  every node added to the SockHop node tree
   // will automatically be given an SHWildPathSorter when it is created!
   
   SHWildPathSorter(BMessage * archive); 
   // BArchivable rehydration constructor
   
   virtual ~SHWildPathSorter();
   
   virtual bool DoesMessageGoToNode(BMessage & msg, const SHNodeSpec & potentialTarget, uint32 flags);
   // Implements "wildpath" BMessage forwarding algorithm, based on the
   // SH_NAME_TO field(s) in (msg).
   
   virtual bool DoesMessageGoToWorker(BMessage & msg, const char * workerName);
   // Implements wildcarding BMessage distribution algorithm, based ont
   // the SH_NAME_TOWORKERS field in (msg).
   
   virtual void BeforeMessageRelay(BMessage & msg);
   // Implemented to remove first part of the SH_NAME_TO strings
   // before transmission.
   
   virtual bool DoesMessageDistributeLocally(BMessage & msg);
   // Returns true iff any SH_NAME_TO fields in (msg) contain the
   // empty string ("") or the current node indicator string (".").
   
   virtual const char * GetName() const;
   // Returns "wildpath".
   
   static BArchivable * Instantiate(BMessage * archive);
   // Needed to be an instantiatable BArchivable object.
   
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.
   
private:
   bool ParsePath(const char * shTo, const SHNodeSpec & child, bool nodeIsParent);
   void GetFirstPartOfPath(const char * from, char * to, int toSize) const;
   bool DoesWildcardMatch(const char * wildcard, const char * matchTo);
 
   SHStringMatcher * _matcher;
   
   /* FBC */
   virtual void _SHWildPathSorter1();
   virtual void _SHWildPathSorter2();
   virtual void _SHWildPathSorter3();
   virtual void _SHWildPathSorter4();
   uint32 _SHWildPathSorterData[8];
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
