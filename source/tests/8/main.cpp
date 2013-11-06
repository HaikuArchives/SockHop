// Just a simple SockHop client program to test connections...
#include <stdio.h>
#include <stdlib.h>
#include <sockhop/SockHop.h>

class DebugLooper : public BLooper
{
private:
   virtual void MessageReceived(BMessage * msg)
   {
      printf("---BEGIN USER CODE MESSAGE---\n");
      msg->PrintToStream();
      
      SHNodeSpec nodeRe;
      SHFileSpec fileRe;
      if (msg->FindFlat(SH_NAME_REGARDING, &nodeRe) == B_NO_ERROR) nodeRe.PrintToStream();
      else if (msg->FindFlat(SH_NAME_REGARDING, &fileRe) == B_NO_ERROR) fileRe.PrintToStream();
   
      printf("---END USER CODE MESSAGE---\n");
   }
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
   char * host = "localhost";
   int port = SH_DEFAULT_PORT;
   
   if (argc >= 2) host = argv[1];
   if (argc >= 3) port = atoi(argv[2]);
   
   printf("Connecting to [%s:%i]\n",host,port);

   DebugLooper * debugLooper = new DebugLooper;
   BLooper * node = SHCreateRootNode(BMessenger(debugLooper));
   node->Run(); 
   debugLooper->Run();

   pause("Press return to set root node to compression mode.\n");
   BMessage batch(SH_COMMAND_SETPARAMETERS);
   batch.AddInt32(SH_PARAMNAME_TRANSMISSIONENCODING, SH_ENCODING_ZLIBHEAVY);
   PostFullMessage(node, &batch);
   
   pause("Press return to add Jeremy and Joanna\n");   
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jeremy", "localhost", 2958));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joanna", "localhost", 2959));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joellen", "localhost", 2960));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Dogbert", "localhost", 2961));
   PostFullMessage(node, &msg);
   
for (int i=0; i<1000; i++)
{
   BMessage m(i);
   printf("Sending message %i (%li)\n",i, m.what);
   m.AddString(SH_NAME_TO, "/*");
   PostFullMessage(node, &m);
   snooze(10000);
}
   
//   pause("Press return to clean up...\n");

   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   
//   pause("Press return to exit.\n");

   return 0; 
}