#include <stdio.h>
#include <stdlib.h>

// Just a simple SockHop client program to test connections...
#include <sockhop/SockHop.h>
#include <sockhop/SHWildPathSorter.h>

class DebugLooper : public BLooper
{
private:
   virtual void MessageReceived(BMessage * msg)
   {
      printf("---BEGIN USER CODE MESSAGE---\n");
      msg->PrintToStream();
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
   node->PostMessage(&msg);

   pause("Press return to add a new shSorter to Jeremy\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   SHWildPathSorter newSorter;
   BMessage sorterMsg;
   newSorter.Archive(&sorterMsg);
   msg.AddMessage(SH_NAME_SORTERS,&sorterMsg);
   msg.AddString(SH_NAME_TO, "/Jeremy");
   node->PostMessage(&msg);
      
   pause("Press return to clean up...\n");
   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   return 0; 
 }