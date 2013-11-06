#include <stdio.h>
#include "SHBitChordSorter.h"

#define SHBCS_NAME_FILTER "filter"

SHBitChordSorter::
SHBitChordSorter(int32 filter) 
   : _filterChord(filter), _sendIsCached(false)
{
   // empty
}

SHBitChordSorter::
SHBitChordSorter(BMessage * arch)
   : SHSorter(arch), _sendIsCached(false)
{
   if (arch->FindInt32(SHBCS_NAME_FILTER, &_filterChord) != B_NO_ERROR)
       printf("SHBitChordSorter:  Error, couldn't find filterChord in archive!\n");

printf("SHBitChordSorter:  my chord is %lx\n",_filterChord);
}

SHBitChordSorter::
~SHBitChordSorter()
{
printf("SHBitChordSorter:  bye bye!\n");
   // empty
}

status_t
SHBitChordSorter::GetAddOnSpec(SHFileSpec & spec) const
{
   (void)spec.AddFlavor(SHFlavor("add-ons/x86/SHBitChordSorter", SH_ARCH_BEOS_X86, true));
   (void)spec.AddFlavor(SHFlavor("add-ons/ppc/SHBitChordSorter", SH_ARCH_BEOS_PPC, true));
   return B_NO_ERROR;
}

const char *
SHBitChordSorter::
GetName() const
{
   return "bitchord";  // Note:   hardcoded name means we are assuming you
                       //         will put no more than one SHBitChordSorter on each node...
}

// This method tell the shNode to send the message to all children (if there
// is an shTo in the message that matches our _filterChord), or to no-one (if there isn't).
bool 
SHBitChordSorter::
DoesMessageGoToNode(BMessage & msg, const SHNodeSpec &, uint32 flags)
{
   return((((flags & SH_FLAG_IS_PARENT) == 0))&&(DoesMessageDistributeLocally(msg))); 
}


bool
SHBitChordSorter::
DoesMessageDistributeLocally(BMessage & msg)
{
   if (_sendIsCached == false)
   {
      _sendValue = false;   // default
      if (_filterChord != 0)
      { 
         int next = 0;
         int32 chord;
         while((msg.FindInt32(SH_NAME_TO, next++, &chord)) == B_NO_ERROR) 
         {
            if ((chord & _filterChord) != 0) 
            {
               _sendValue = true;
               break;
            }
         }
      }
      _sendIsCached = true;
   }
   return _sendValue;
}


void
SHBitChordSorter::
BeforeMessageRelay(BMessage & msg)
{
   // first read all shTo's into a BList...
   BList tempList;
   int next = 0;
   int32 chord;
   while((msg.FindInt32(SH_NAME_TO, next++, &chord)) == B_NO_ERROR)
   {
      chord &= ~(_filterChord);
      if (chord != 0) tempList.AddItem((void *)chord);
   }
   
   // Now replace the current shTo's with the new list
   if (tempList.CountItems() > 0) 
        (void)msg.ReplaceData(SH_NAME_TO, B_INT32_TYPE, tempList.Items(), sizeof(int32)*tempList.CountItems());
   else (void)msg.RemoveData(SH_NAME_TO);
   
   _sendIsCached = false;  // so that we will recalculate _sendValue for the next message.
}

    
BArchivable *
SHBitChordSorter::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "SHBitChordSorter")) return NULL;
   return new SHBitChordSorter(archive);
}


status_t
SHBitChordSorter::
Archive(BMessage * archive, bool deep) const
{
   status_t ret = SHSorter::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
   archive->AddInt32(SHBCS_NAME_FILTER, _filterChord); 
   return B_NO_ERROR; 
}
   

