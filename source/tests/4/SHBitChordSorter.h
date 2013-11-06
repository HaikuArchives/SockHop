#ifndef _SHBITCHORDSORTER_H_
#define _SHBITCHORDSORTER_H_

#include <sockhop/SHSorter.h>

#ifdef __INTEL__
_EXPORT class SHBitChordSorter;
#else
#pragma export on
#endif

//Sorts based on BitChords in the shTo field of the BMessage.
//If no shTo field is found in the BMessage, 
//then message goes to everybody except the parent node.
class SHBitChordSorter : public SHSorter 
{
public:
   SHBitChordSorter(int32 filterChord);
   //messages whose ((shTo fields) & filter) != 0) will be passed
   //on to the local node and all children.  Before the message is
   //passed on, all shTo fields will have (filter)'s bits AND'd out.
   
   SHBitChordSorter(BMessage * archive); // for BArchivable compatibility
   ~SHBitChordSorter();
   
   virtual bool DoesMessageGoToNode(BMessage & msg, const SHNodeSpec & child, uint32 flags);
   
   virtual void BeforeMessageRelay(BMessage & msg);
   
   virtual bool DoesMessageDistributeLocally(BMessage & msg);

   virtual const char * GetName() const;
   
   virtual status_t GetAddOnSpec(SHFileSpec & addTo) const;
   
   // BArchivable interface
   static BArchivable * Instantiate(BMessage * archive);
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.
   
private:   
   int32 _filterChord;
   bool _sendValue;
   bool _sendIsCached;
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif
