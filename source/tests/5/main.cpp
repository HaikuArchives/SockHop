// Just a simple SockHop client program to test connections...
#include <stdio.h>
#include <stdlib.h>
#include <sockhop/SockHop.h>
#include "../4/SHBitChordSorter.h"
#include "../6/SHTestWorker.h"

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
   snooze(500000);
   printf(msg);
   gets(temp);   
}

void PostFullMessage(BLooper * looper, BMessage * msg);
void PostFullMessage(BLooper * looper, BMessage * msg)
{
   BMessage onSuccess('SUCC'); onSuccess.AddString(SH_NAME_TO, "/..");
   BMessage onFailure('FAIL'); onFailure.AddString(SH_NAME_TO, "/..");
   BMessage whenDone('DONE');  whenDone.AddString(SH_NAME_TO, "/..");
   msg->AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
   msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   looper->PostMessage(msg);
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

   pause("Press return to add Jeremy\n");   
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jeremy", "localhost", 2958));
   PostFullMessage(node, &msg);

   pause("Press return to add a new SHBitChordSorter to Jeremy\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   SHBitChordSorter newSorter(0xF);
   BMessage sorterMsg;
   newSorter.Archive(&sorterMsg);

   msg.AddMessage(SH_NAME_SORTERS,&sorterMsg);
   msg.AddString(SH_NAME_TO, "/Jeremy");
   PostFullMessage(node, &msg);

   pause("Press return to remove the new SHBitChordSorter from Jeremy\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_REMOVECOMPONENTS;
   msg.AddString(SH_NAME_SORTERS, "*");
   PostFullMessage(node, &msg);

   msg.MakeEmpty();  msg.what = SH_COMMAND_ADDCOMPONENTS;
   {
      pause("Press return to add new SHTestWorkers to Jeremy\n");
      for(int i=0; i<10; i++) 
      {
         char temp[50];
         sprintf(temp, "FredWorker%i",i);
         SHTestWorker newWorker(temp); 
         BMessage workerMsg;
         newWorker.Archive(&workerMsg);
         msg.AddMessage(SH_NAME_WORKERS,&workerMsg);
      }
      msg.AddString(SH_NAME_TO, "/Jeremy");
      PostFullMessage(node, &msg);
   }

#if 0
   {
     for (int i=0; i<10; i++)
     {
       pause("Press return to kill worker...\n");
      
       msg.MakeEmpty();  msg.what = SH_COMMAND_REMOVECOMPONENTS;
       char temp[50]; sprintf(temp, "FredWorker%i", i);
       msg.AddString(SH_NAME_COMPONENTS, temp);
       msg.AddString(SH_NAME_TO, "/Jeremy");
       PostFullMessage(node, &msg);
     }
   }
#endif

   pause("Press return to send a message to Jeremy's worker\n");
   msg.MakeEmpty(); msg.what = 0x01;
   msg.AddString(SH_NAME_TO, "/Jeremy");
   PostFullMessage(node, &msg);

   pause("Press return to remove workers 4, 5, and 6 from Jeremy\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_REMOVECOMPONENTS;
   msg.AddString(SH_NAME_TO, "/Jeremy");
   msg.AddString(SH_NAME_WORKERS, "FredWorker4");
   msg.AddString(SH_NAME_WORKERS, "FredWorker5");
   msg.AddString(SH_NAME_WORKERS, "FredWorker6");
   PostFullMessage(node, &msg);
   
   pause("Press return to clean up...\n");

   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   return 0; 
}