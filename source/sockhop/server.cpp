
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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <app/Application.h>
#include <sockhop/SHSessionAcceptor.h>
#include <sockhop/SHFlavor.h>
#include <sockhop/SockHopConstants.h>
#include <sockhop/SockHopFunctions.h>
#include <sockhop/SHDefaultAccessPolicy.h>
#include "shNode.h"
#include "SockHopInternalConstants.h"
#include "shStraightConnection.h"

class ServerApplication : public BApplication
{
public:
   ServerApplication(const char * serverName, char ** env, const BMessage & policyArchive, SHAccessPolicy * policy);
   ~ServerApplication();
   
   virtual void MessageReceived(BMessage * msg);
   
private:
   const char * _serverName;
   char ** _env;
   const BMessage _policyArchive;
   SHAccessPolicy * _policy;   // Note:  owned by _acceptor, hope it's okay to hold it here too!
   SHSessionAcceptor _acceptor;
};

//////////////////////////////////////////////////////////////////
//
// ServerApplication
//
//////////////////////////////////////////////////////////////////

ServerApplication ::
ServerApplication(const char * serverName, char ** env, const BMessage & policyArchive, SHAccessPolicy * policy)
   : BApplication("application/x-vnd.Sugoi-SockHopServer"), 
     _env(env), _serverName(serverName), _policyArchive(policyArchive),
     _acceptor(BMessenger(this), policy, false), _policy(policy)
{
   if (_acceptor.Start()) 
   {
      printf("SockHop server is listening on port %li, using %s access policy.\n", _acceptor.GetAcceptSpec().GetPortNumber(), policy->GetName());
   }
   else
   {
      printf("Error, couldn't bind to port!  Exiting...\n");
      be_app->PostMessage(B_QUIT_REQUESTED);
   }

   policy->OnServerStartup();
}

ServerApplication ::
~ServerApplication()
{
   _policy->OnServerShutdown();
}

void 
ServerApplication ::
MessageReceived(BMessage * msg)
{
   switch(msg->what)
   {
      case SH_CODE_CONNECTIONOPEN:
      {
         int32 id;
         if (msg->FindInt32(SH_NAME_CONNECTIONID, &id) == B_NO_ERROR)
         {
             SHDirectConnection * conn = _acceptor.DetachSession(id);
             if (conn)
             {
                SHNodeSpec callbackSpec;
                if (conn->GetNodeSpec().GetSpecMessage().FindFlat(SH_NAME_CALLBACKSPEC, &callbackSpec) == B_NO_ERROR)
                {
                   if (conn->Start())  // alas, this doesn't make a diff 'cause it's probably already started!
                   {
                       BMessage childMsg(SH_INTERNAL_SERVERSTARTINFO);
                       childMsg.AddFlat(SH_NAME_CALLBACKSPEC, &callbackSpec);
                       childMsg.AddMessage(SH_NAME_POLICY, &_policyArchive);
                        
                       size_t msgSize = childMsg.FlattenedSize();
                       char * temp = new char[msgSize];
                        
                       if (childMsg.Flatten(temp, msgSize) == B_NO_ERROR)
                       {
                          // Launch child process!      
                          const char * subArgv[1] = {_serverName};
      
                          thread_id subProcess = load_image(1, subArgv, (const char **)_env);
                          if (subProcess >= 0)
                          {
                             if (send_data(subProcess, SH_INTERNAL_SERVERSTARTINFO, temp, msgSize) == B_NO_ERROR)
                             {
                                if (resume_thread(subProcess) == B_NO_ERROR) 
                                {
                                   BMessage msg(SH_INTERNAL_WILLCONNECTBACK);
                                   conn->GetMessenger().SendMessage(&msg);
                                }
                                else printf("ServerAcceptor:  Error resuming new process!\n");
                             }
                             else printf("ServerAcceptor:  Error sending startup data to new child process!\n");
                          }
                          else printf("ServerAcceptor:  Unable to spawn child process!\n");   
                       }
                       else printf("ServerAcceptor:  Error flattening startup info!\n");
                       
                       delete [] temp;
                   }
                   else printf("ServerApplication:  Couldn't start send thread to tell client I'll call back!\n");
                }
                else printf("ServerApplication:  ERROR, couldn't find callback spec in message!\n");
                
                delete conn;  // After calling DetachSession(), it becomes our responsibility to kill it
             }
             else printf("ServerApplication:  ERROR, couldn't find SHDirectConnection id = %li!\n", id);
         }
         else printf("ServerApplication:  ERROR, couldn't find CONNECTIONID code!\n");
      }
      break;
   }
}
      

class NodeApplication : public BApplication
{
public:
   NodeApplication(const BMessage & startupMsg);
   ~NodeApplication();
   
private:
   shConnection * _connectLooper;
   BLooper * _nodeLooper;
};

//////////////////////////////////////////////////////////////////
//
// NodeApplication
//
//////////////////////////////////////////////////////////////////

NodeApplication ::
NodeApplication(const BMessage & msg)
   : BApplication("application/x-vnd.Sugoi-SockHopNode"), _connectLooper(NULL), _nodeLooper(NULL)
{
   BMessage policyMsg;
   bool success = false;  // default

   if (msg.FindMessage(SH_NAME_POLICY, &policyMsg) == B_NO_ERROR)
   {
      SHNodeSpec connectTo;

      if (msg.FindFlat(SH_NAME_CALLBACKSPEC, &connectTo) == B_NO_ERROR)
      {
         SHFileSpec addOnSpec;
         SHAccessPolicy * policy = NULL;

         if (policyMsg.FindFlat(SH_NAME_ADDONSPEC, &addOnSpec) == B_NO_ERROR)
         {
             if (addOnSpec.CountFlavors() > 0)  // i.e. are there any flavors in the msg?
             {
                SHDistributableObject * obj = SHCreateDistributableObject(&policyMsg);
             
                if (obj)
                {
                   if (cast_as(obj, SHAccessPolicy)) policy = (SHAccessPolicy *) obj;
                   else 
                   {
                      printf("Error, policy object wasn't an SHAccessPolicy!\n");
                      SHDeleteDistributableObject(obj);
                   }
                }
             }
             else policy = new SHDefaultAccessPolicy(&policyMsg);
         }
         else policy = new SHDefaultAccessPolicy(&policyMsg);
       
         if (policy)
         {
            int newNodeID = GetNextConnectionID();
            int parentID = GetNextConnectionID();
            _connectLooper = new shConnection(new shStraightConnection, BMessenger(), -1, parentID, BMessage(), connectTo, policy->GetDefaultThreadPriority(), policy->GetDefaultTransmissionEncoding());
            _nodeLooper = new shNode(BMessenger(_connectLooper), parentID, newNodeID, BMessage(), connectTo, true, policy);
            _connectLooper->SetReplyTarget(BMessenger(_nodeLooper));
            if (_connectLooper->StartThreads())
            {
               _connectLooper->Run();
               _nodeLooper->Run();
               success = true;
            }
            else printf("Error starting connection threads, aborting!\n");
         }
      }
      else printf("No SH_NAME_CALLBACKSPEC in startup message!\n");
   }
   else printf("No SH_NAME_POLICY in startup message!\n");

   if (success == false)
   {
      printf("Error starting the new node, aborting!\n");
      PostMessage(B_QUIT_REQUESTED);
   }
}

NodeApplication ::
~NodeApplication()
{
  if ((_nodeLooper)&&(_nodeLooper->Lock())) _nodeLooper->Quit();
  if ((_connectLooper)&&(_connectLooper->Lock())) _connectLooper->Quit();
}


BMessage BuildPolicyArchive(int argc, char ** argv);
BMessage BuildPolicyArchive(int argc, char ** argv)
{
   BMessage msg;
   
   for (int i=1; i<argc; i++)
   {
      char * next = strdup(argv[i]);
      if (next)
      {
         char * equals = strchr(next, '=');
         if (equals)
         {  
             *equals = '\0';
             msg.AddString(next, equals+1);
         }
         free(next);
      }
      else printf("SockHopServer: warning, strdup() failed!\n");
   }
   return msg;
}


int main(int argc, char ** argv, char ** env)
{
   if (has_data(find_thread(NULL)))
   {
      int bufSize = 100 * 1024;   // 100K oughta be enough for anyone
      char * buf = new char[bufSize];
  
      thread_id senderID;
      if (receive_data(&senderID, buf, bufSize) == SH_INTERNAL_SERVERSTARTINFO)
      {
         BMessage startMsg;
     
         if (startMsg.Unflatten(buf) == B_NO_ERROR)
         {
            delete [] buf;
            buf = NULL;
        
            NodeApplication app(startMsg);
            app.Run();  // Will return when app is asked to quit by the system, or when shNode tells it to quit
         }
      }
      else printf("SockHopNode:  Couldn't get startup data!\n");

      if (buf) delete [] buf;
      return 0;
   }
   else if ((argc == 2)&&(strcmp(argv[1], "-help") == 0))
   {
      printf("===============================================================================\n");
      printf("=\n");
      printf("= Usage:  libsockhop.so [policy=filename] [param1=value1] [param2=value2] [...]\n");
	  printf("= Here are some valid example invocations:\n");
	  printf("= \n=   libsockhop.so\n");
	  printf("=     (Starts a server process with the default access policy, and NO PASSWORD\n");
	  printf("=      REQUIRED!  While this makes testing easy, it's roughly equivalent to\n");
	  printf("=      running a telnet daemon with no password required, so only do this if you\n");
	  printf("=      are on an isolated network!)\n");
	  printf("= \n=   libsockhop.so password=topsecret\n");
	  printf("=     (Starts a server process with the default access policy, but will only\n");
	  printf("=      allow connections that supply the password 'topsecret' to connect)\n");
	  printf("= \n=   libsockhop.so password=topsecret port=2996\n");
	  printf("=     (Starts a server process with the default access policy, listening on\n");
	  printf("=      port 2996, that only allow connections that supply the password\n");
	  printf("=      'topsecret' to connect)\n");
	  printf("= \n=   libsockhop.so port=2996 password=topsecret debug=1\n");
	  printf("=     (Same as the previous example, but turns on debug printing also)\n");
	  printf("= \n=   libsockhop.so priority=15 encoding=zlibheavy\n");
	  printf("=     (Starts a server with all threads running at priority 15, and with\n");
	  printf("=      zlib level 6 compression applied to all TCP-transmitted BMessages)\n");
	  printf("= \n=   libsockhop.so policy=myPolicyAddOnFile class=myPolicy\n");
	  printf("=                 myParam=aNiceValue anotherParam=aMeanValue\n");
	  printf("=     (Starts a server with your own custom policy add-on, and adds the typed\n");
	  printf("=      in parameters as strings to the add-ons rehydration/archive BMessage.\n");
	  printf("=      If the class parameter isn't specified, it will be assumed that the name\n");
	  printf("=      of the policy class is the same as the name of the add-on file.\n");
	  printf("=      See the SockHop documentation for more info on custom SHAccessPolicy's.\n");
	  printf("=\n");
      printf("===============================================================================\n");
	  return 5;
   }
   else
   {
      // defaults
      if (argc <= 1) printf("(Note:  Run libsockhop.so -help for info on command line arguments)\n");
      
      // Instantiate our AccessPolicy.
      SHAccessPolicy * policy = NULL;
      BMessage archive = BuildPolicyArchive(argc, argv);
      const char * policyName;
      if (archive.FindString("policy", &policyName) == B_NO_ERROR)
      {
         SHFlavor flavor(policyName, SHGetArchitectureBits(), true);
         SHFileSpec fs;
         
         if ((flavor.InitCheck() == B_NO_ERROR)&&(fs.AddFlavor(flavor) == B_NO_ERROR))
         {
            archive.AddFlat(SH_NAME_ADDONSPEC, &fs);
         
            if (archive.HasString("class") == false)
            {
               const char * className = strrchr(policyName, '/');
               if (className == NULL) className = policyName;
            
               archive.AddString("class", className);
            }
         }
         else printf("ERROR:  policy add-on file [%s] not found!\n", policyName);
      }      
      else policy = new SHDefaultAccessPolicy(&archive);
     
      if (policy)
      {
         if (policy->Archive(&archive) == B_NO_ERROR)
         {
            ServerApplication app(argv[0], env, archive, policy);
            app.Run();
         }
         else printf("Fatal error:  The Archive() method of the SHAccessPolicy failed.\n");
      }
      else printf("Fatal error:  Couldn't instantiate your policy add-on!\n");
   }
   return 0;
}