
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


#ifndef SHSessionAcceptor_h
#define SHSessionAcceptor_h

#include <sockhop/SHNodeSpec.h>
#include <sockhop/SHDirectConnection.h>
#include <sockhop/SHAccessPolicy.h>

#include <support/Locker.h>

////////////////////////////////////////////////////////////
//
// SHSessionAcceptor
//
// This class allows you to listen for and receive connections 
// from multiple SHDirectConnections on one port.  
//
/////////////////////////////////////////////////////////////

class shNode;

#ifdef __INTEL__
_EXPORT class SHSessionAcceptor;
#else
#pragma export on
#endif 

class SHSessionAcceptor
{
public:
    //////////////
    // Lifecycle
    //////////////

    SHSessionAcceptor(const BMessenger & target, SHAccessPolicy * policy, bool startImmediately);
    // (target) is where status BMessages from this SHSessionAcceptor will go, by default.
    // (policy) determines most facets of the behavior of this SHSessionAcceptor.
    // (policy) should have been allocated with the new operator or SHCreateDistributableObject(),
    // and it becomes property of this SHSessionAcceptor (i.e., the SHSessionAcceptor will delete
    // it when the SHSessionAcceptor is deleted).  For many purposes, passing in a 
    // new SHDefaultAccessPolicy object will be sufficient.
    // If (startImmediately) is set to false, then the accept thread won't be started
    // until StartAccepting() is called.   
    
    ~SHSessionAcceptor();
    // Unbinds from the port it was listening on, closes all attached sessions,
    // and destroys the SHSessionAcceptor.
    
    ///////////////////////////////////
    // SHSessionAcceptor's new members
    ///////////////////////////////////
    
    bool Start();
    // If you specified (startImmediately = false) in the SHSessionAcceptor constructor, 
    // then you need to call this to activate the SHSessionAcceptor.  When this method is called,
    // the SHSessionAcceptor will bind to a port (as specified by it's SHAccessPolicy
    // object's GetListeningLocation() method) and begin accepting connections.
    // Returns true on success, or false if there was an error binding to the port.
    // Returns true with no side effects if the SHSessionAcceptor is already
    // accepting connections.
    
    SHNodeSpec GetAcceptSpec() const;
    // Returns the SHNodeSpec that this SHSessionAcceptor is receiving on.  
    // The returned SHNodeSpec is the same one that the GetListeningLocation() 
    // method of the SHAccessPolicy you passed to the constructor of this object 
    // returns, but with the hostName and port number fields set appropriately.  
    // Send this SHNodeSpec other programs, and they can use it to easily connect back 
    // to this SHSessionAcceptor.  This method only returns a valid result if the SHSessionAcceptor
    // has already been successfully activated via a call to the StartAccepting()
    // method.
    
    status_t CloseSession(int32 sessionID);
    // Closes the session with the given ID.
    // Returns B_NO_ERROR if the session was found and closed, B_ERROR if it could not be found.

    void CloseAllSessions();
    // Closes all sessions owned by this SHSessionAcceptor.

    status_t SendMessageToAllSessions(const BMessage & message);
    // Sends the given BMessage to all connected sessions.  Returns B_NO_ERROR if
    // everything goes okay, B_ERROR if there were problems sending the message (in which
    // some of the sessions may not have got the message)
    
    status_t SendMessageToSession(int32 messageID, const BMessage & message);
    // Sends the given BMessage to the session with the given (messageID).
    // Returns B_NO_ERROR if the message send succeeded, some other error code if
    // it failed.

    SHDirectConnection * DetachSession(int32 messageID);
    // Removes the session with the given (messageID) from the SHSessionAcceptor's control,
    // and returns it to the calling code as an SHDirectConnection.
    // Returns NULL if the session with the given (messageID) could not be found.
    // NOTE:  Once this method returns an SHDirectConnection, it becomes the calling code's
    // responsibility to manage and delete that SHDirectConnection!
        
    void DetachAllSessions(BList & getConnectionObjects);
    // Calls DetachSession() on all current sessions, and adds pointers to the
    // SHDirectConnection objects for all sessions as items in the passed-in BList.
    // NOTE:  It becomes the caller's responsibility to delete ALL SHDirectConnections
    // that are added to the BList!

private:
    SHDirectConnection * GetConnectionByID(int32 id, int * optSetIndex = NULL);
    
    BLocker _synchList;
    BList   _sessions;    // (SHDirectConnection *)'s
    
    static long acceptLoopStub(void * data);
    long acceptLoop();

    SHAccessPolicy * _policy;
    thread_id _acceptThreadId;
    SHNodeSpec _acceptAt;
    BMessenger _messageTarget;
    int _acceptorSock;
};

#ifndef __INTEL__
#pragma reset
#endif

#endif
