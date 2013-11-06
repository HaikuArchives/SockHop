// Just a simple SockHop client program to test connections...
#include <stdio.h>
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

void PostFullMessage(const char * dbg, BLooper * looper, BMessage * msg);
void PostFullMessage(const char * dbg, BLooper * looper, BMessage * msg)
{
   BMessage onSuccess('SUCC'); onSuccess.AddString(SH_NAME_TO, "/.."); onSuccess.AddString("dbg", dbg);
   BMessage onFailure('FAIL'); onFailure.AddString(SH_NAME_TO, "/.."); onFailure.AddString("dbg", dbg);
   BMessage whenDone('DONE');  whenDone.AddString(SH_NAME_TO, "/..");  whenDone.AddString("dbg", dbg);
//   msg->AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
   msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   looper->PostMessage(msg);
}


int main(int, char **)
{
   printf("Net-server crashing program is beginning!.\n"); 
   fflush(stdout);
   
   DebugLooper * debugLooper = new DebugLooper;
   BLooper * node = SHCreateRootNode(BMessenger(debugLooper), new SHDefaultAccessPolicy(0, 1));
   if (node == NULL) 
   {
      printf("Couldn't create root node!\n");
      exit(5);
   }
   node->Run(); 
   debugLooper->Run();

   pause("Press return to add 5 nodes to the SockHop server running on port 2958-2962.\n(Before doing this, make sure a server is running by typing '~/config/lib/libsockhop.so debug=1' into another terminal)\n");   
   
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Node 1", "localhost", 2958));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Node 2", "localhost", 2959));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Node 3", "localhost", 2960));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Node 4", "localhost", 2961));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Node 5", "localhost", 2962));
   PostFullMessage("Add nodes", node, &msg);

   pause("Press return to add a symbolic link from all to all!  (On my computer, net_server crashes as soon as I press return here)\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   msg.AddString(SH_NAME_TO, "/*");
   msg.AddString(SH_NAME_SYMLINKS, "../*");
   PostFullMessage("add link", node, &msg);
 
   pause("Press return to send a message from each node through all the symlinks.\n");
   msg.MakeEmpty(); msg.what = 0x69;
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("send messages", node, &msg);
   
   pause("Press return to clean up and exit.\n");
   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   return 0; 
}