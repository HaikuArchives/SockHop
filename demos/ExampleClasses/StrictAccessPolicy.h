#ifndef STRICTACCESSPOLICY_H
#define STRICTACCESSPOLICY_H

#include <sockhop/SockHopConstants.h>
#include <sockhop/SHAccessPolicy.h>

#ifdef __INTEL__
_EXPORT class StrictAccessPolicy;
#else
#pragma export on
#endif

// This example class implements a very simple, but very
// strict access control policy.  Only hosts whose hostnames
// contain the string specified in the constructor are allowed
// to connect to a server that uses this string!
// Furthermore, this access policy prevents reading and writing
// of files via absolute file paths.
class StrictAccessPolicy : public SHAccessPolicy
{
public:
   StrictAccessPolicy(const char * acceptableHostnameSubstring, int port = SH_DEFAULT_PORT);
   // Only hosts whose IP names contain (acceptableHostnameSubstring) may connect!
   
   virtual ~StrictAccessPolicy();
 
   virtual SHNodeSpec GetListeningLocation();
   // Returns an SHNodeSpec which indicates which port to bind to (as was specified in the ctor).
   
   virtual bool OkayToAcceptConnection(SHNodeSpec & connectingSpec);
   // Only returns true if (connectingSpec) has an acceptable hostname.   (See above).
   
   virtual bool OkayToWriteFile(const char * fileName);
   // Returns true iff (fileName) doesn't start with a '/' char.
   
   virtual bool OkayToReadFile(const char * fileName);
   // Returns true iff (fileName) doesn't start with a '/' char.

   virtual bool OkayToInstantiateObject(const BMessage & archive);
   // Returns false--don't let any add-ons be loaded in.

   //////////////////////////////
   // SHComponent interface
   //////////////////////////////
   
   virtual const char * GetName() const;
   // Returns "strict"

   ////////////////////////////////////
   // SHDistributableObject interface
   ////////////////////////////////////
   
   virtual status_t GetAddOnSpec(SHFileSpec & spec) const;
   // Returns the path(s) to our add-on.
   
   ////////////////////////////  
   // BArchivable interface   
   ////////////////////////////

   StrictAccessPolicy(BMessage * archive);
   // Looks for *string* fields named "port", "password" and "debug" in (archive).
      
   static BArchivable * Instantiate(BMessage * archive);
   // Needed to be an instantiatable BArchivable object.
   
   virtual status_t Archive(BMessage * archive, bool deep=true) const;
   // Must be called at the beginning of subclass's Archive() methods.

private:
   int _port;
   char * _sub;
};

#ifndef __INTEL__
#pragma export reset
#endif

#endif