#ifndef _SHBITCHORDSORTER_H_
#define _SHBITCHORDSORTER_H_

#include <sockhop/SHSorter.h>

#ifdef __INTEL__
_EXPORT class SHBitChordSorter;
#else
#pragma export on
#endif

// An example custom SHSorter.  This sorter will forward a
// BMessage to all children and give the BMessage to the current
// node's workers, if the message's bits are a subset of 
// current node's bits.   Otherwise it will just let the message drop
// into the void.
//
// All BMessages sent to this SHSorter are expected to have an
// int32 field with the name SHBC_NAME_BITS.

#define SHBC_NAME_BITS "shbcBits"

class SHBitChordSorter : public SHSorter 
{
public:
   SHBitChordSorter(int32 filterChord);
   // Messages whose ((shTo field bits) & filterChord) != 0) will be passed
   // on to the local workers and all children.  Before the message is
   // passed on, all shTo field will have (~filterChord) AND'd out.
   
   ~SHBitChordSorter();
   
   virtual bool DoesMessageGoToNode(BMessage & msg, const SHNodeSpec & child, uint32 flags);
   // Returns true iff (msg) is to be forwarded to (child).
   // In this case, the sorter does an all-or-nothing broadcast,
   // so this method is implemented by just calling DoesMessageDistributeLocally(),
   // which does the actual calculation of whether the message
   // should be broadcast or dropped.
      
   virtual void BeforeMessageRelay(BMessage & msg);
   // Strips any bits that weren't in (filterChord) out of 
   // the SHBC_NAME_BITS bitChords in (msg).
   
   virtual bool DoesMessageDistributeLocally(BMessage & msg);
   // Implements the bit-filtering message-forwarding algorithm.
   
   virtual const char * GetName() const;
   // Returns the string literal "bitchord".
   
   virtual status_t GetAddOnSpec(SHFileSpec & spec) const;
   // Puts references to our add-on file(s) into (spec).
   
   // BArchivable interface
   SHBitChordSorter(BMessage * archive);    
   static BArchivable * Instantiate(BMessage * archive);
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   
private:   
   int32 _filterChord;  // Our filtering chord.
   bool _sendValue;     // Cached value, if (_sendIsCached) is true.
   bool _sendIsCached;  // For efficiency, since we only broadcast or drop BMessages.
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif
