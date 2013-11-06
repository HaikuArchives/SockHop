// Just a simple SockHop client program to test direct connections...
#include <stdio.h>
#include <stdlib.h>
#include <sockhop/SockHop.h>
#include "../../sockhop/SockHopInternalConstants.h"
#include "../4/SHBitChordSorter.h"
#include "../6/SHTestWorker.h"

char * TypeToString(uint32 what);
char * TypeToString(uint32 what)
{
   switch(what)
   {
      case SH_COMMAND_BASE:           return "SH_COMMAND_BASE";
      case SH_COMMAND_ADDCOMPONENTS:  return "SH_COMMAND_ADDCOMPONENTS";
	  case SH_COMMAND_REMOVECOMPONENTS:        return "SH_COMMAND_REMOVECOMPONENTS";
      case SH_COMMAND_SETPARAMETERS:  return "SH_COMMAND_SETPARAMETERS";
      case SH_COMMAND_GETPARAMETERS:  return "SH_COMMAND_GETPARAMETERS";
      case SH_COMMAND_QUIT:           return "SH_COMMAND_QUIT";
	  case SH_INTERNAL_BASE:          return "SH_INTERNAL_BASE";
      case SH_CODE_CONNECTIONOPEN:        return "SH_INTERNAL_CONNECTION_OPEN";
	  case SH_CODE_CONNECTIONCLOSED:      return "SH_INTERNAL_CONNECTION_CLOSED";
	  case SH_INTERNAL_NODEPATH:      return "SH_INTERNAL_NODEPATH";
	  case SH_INTERNAL_KILLME:        return "SH_INTERNAL_KILLME";
	  case SH_INTERNAL_NEEDFILE:      return "SH_INTERNAL_NEEDFILE";
	  case SH_INTERNAL_HEREISFILE:    return "SH_INTERNAL_HEREISFILE";
	  case SH_INTERNAL_TAGUNREF:      return "SH_INTERNAL_TAG_UNREF";
	  case SH_INTERNAL_NOOP:          return "SH_INTERNAL_NOOP";
	  case SH_INTERNAL_LINKREQUEST:   return "SH_INTERNAL_LINKREQUEST";
	  case SH_INTERNAL_LINKREQUESTCOMPLETE:   return "SH_INTERNAL_LINKREQUEST_COMPLETE";
	  case SH_INTERNAL_REMOVEWORKER:  return "SH_INTERNAL_REMOVEWORKER";
	  case SH_INTERNAL_LARGEST:       return "SH_INTERNAL_LARGEST";
      default:                        return "User Message?";
   }
}

class DebugLooper : public BLooper
{
public:
   DebugLooper(SHAccessPolicy * policy)
      : _connection(BMessenger(this), policy, true)
   {
      printf("Now accepting connections at:\n");
      _connection.GetAcceptSpec().PrintToStream();
   }

   void SendLineMessage(BMessage * msg)
   {
      _connection.SendMessageToAllSessions(msg);
   }
      
   virtual void MessageReceived(BMessage * msg)
   {
//      printf("Got a message %li\n",msg->what);

      printf("---BEGIN USER CODE MESSAGE (%s)(%li)---\n",TypeToString(msg->what), msg->FlattenedSize());
      msg->PrintToStream();
/*
      
      SHNodeSpec nodeRe;
      SHFileSpec fileRe;
      if (msg->FindFlat(SH_NAME_REGARDING, &nodeRe) == B_NO_ERROR) nodeRe.PrintToStream();
      else if (msg->FindFlat(SH_NAME_REGARDING, &fileRe) == B_NO_ERROR) fileRe.PrintToStream();
   
      printf("---END USER CODE MESSAGE---\n");
*/
   }

private:
   SHSessionAcceptor _connection;
};

void pause(char *);
void pause(char * msg)
{
   char temp[100];
   snooze(1500000);
   printf(msg);
   gets(temp);   
}


void PostFullMessage(BLooper * looper, BMessage * msg);
void PostFullMessage(BLooper * looper, BMessage * msg)
{
   BMessage onSuccess('SUCC'); onSuccess.AddString(SH_NAME_TO, "/..");
   BMessage onFailure('FAIL'); onFailure.AddString(SH_NAME_TO, "/..");
   BMessage whenDone('DONE');  whenDone.AddString(SH_NAME_TO, "/..");
//   msg->AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
//   msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
//   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   if (looper->PostMessage(msg) != B_NO_ERROR) printf("WARNING:  PostFullMessage, message post failed!\n");
}


int main(int argc, char ** argv)
{
   char * host = "127.0.0.1";
   char * pwd = NULL;
   int port = 0;
   
   if (argc >= 2) host = argv[1];
   if (argc >= 3) port = atoi(argv[2]);
   if (argc >= 4) pwd = argv[3];
  
   DebugLooper * debugLooper = new DebugLooper(new SHDefaultAccessPolicy(port, 0, pwd));
   debugLooper->Run();

   while(1)
   {
      printf("Type some text to send across the line, or just enter to quit.\n");
      char temp[500];
      gets(temp);
      if (*temp == '\0') break;
      BMessage m(69);
      m.AddString("heyDude", temp);
      debugLooper->SendLineMessage(&m);
   }
   printf("Cleaning up...\n");
   if (debugLooper->Lock()) debugLooper->Quit();
   printf("bye bye!\n");
   return 0; 
}
