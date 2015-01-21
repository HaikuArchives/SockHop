// A very simple SockHop client program, just to test connections & basic functionality.
#include <stdio.h>
#include <stdlib.h>
#include <sockhop/SockHop.h>

// Just prints out any BMessages it receives, for review.
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

void pause(int32, char *);
void pause(int32 seconds, char * msg)
{
   snooze(seconds * 1000000);
   char temp[100];
   printf("\n\n\n*** ");
   printf(msg);
   printf("\n\n\n");
   gets(temp);   
}

// Adds requests for full error-reporting to the (msg) and posts it.
void PostFullMessage(char * re, BLooper * looper, BMessage * msg);
void PostFullMessage(char * re, BLooper * looper, BMessage * msg)
{
   BMessage onSuccess('SUCC'); onSuccess.AddString(SH_NAME_TO, "/.."); onSuccess.AddString("re", re);
   BMessage onFailure('FAIL'); onFailure.AddString(SH_NAME_TO, "/.."); onFailure.AddString("re", re);
   BMessage whenDone('DONE');  whenDone.AddString(SH_NAME_TO, "/..");  whenDone.AddString("re", re);
   msg->AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
   msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   if (looper->PostMessage(msg) != B_NO_ERROR) printf("Warning, message posting failed!\n");
}

int main(int, char **)
{
   printf("\nThis is a very crude test program/demo to show basic SockHop functionality.\n");
   printf("It demonstrates how to connect to SockHop servers, cache files on them, and\n");
   printf("Send BMessages to them.  Before you run this demo, you should have SockHop servers\n");
   printf("running on localhost on ports 2958 through 2962.  To do this, type one of\n");
   printf("the following commands into each five Terminal windows):\n");
   printf("\n");
   printf("~/config/lib/libsockhop.so port=2958 debug=true\n");
   printf("~/config/lib/libsockhop.so port=2959 debug=true\n");
   printf("~/config/lib/libsockhop.so port=2960 debug=true\n");
   printf("~/config/lib/libsockhop.so port=2961 debug=true\n");
   printf("~/config/lib/libsockhop.so port=2962 debug=true\n");
   printf("\n");
   printf("(If you wish to connect to servers on other hosts, you will need to edit\n");
   printf("the source code and recompile.  It's hard coded, that's how crude this program is!)\n");
   printf("Note that all status/error BMessages returned by SockHop back to this program are\n");
   printf("simply printed to stdout.  For simplicity, no synchronization is enforced, so you\n");
   printf("may see BMessages for an operation printed out after the prompt for the next\n");
   printf("operation has already been printed.\n");
   printf("\n");
      
   pause(0, "Press return to create the root node of the tree (in the local process space)\n");   
   DebugLooper * debugLooper = new DebugLooper;
   debugLooper->Run();
   BLooper * node = SHCreateRootNode(BMessenger(debugLooper));
   if (node == NULL) 
   {
      printf("Error creating root node!  Exiting.\n");
      exit(0);
   }
   node->Run();

   pause(0, "Press return to add child nodes named 'Jeremy', 'Joanna', 'Joellen' to the root node.\nJeremy will run on port 2958, Joanna on 2959, Joellen on 2960.");   
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jeremy", "localhost", 2958));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joanna", "localhost", 2959));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joellen", "localhost", 2960));
   PostFullMessage("Adding Jeremy, Joanna, Joellen", node, &msg);

   pause(2, "Press return to send a do-nothing BMessage to all three children.\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*");
   PostFullMessage("Sending message to all children", node, &msg);

   pause(1, "Press return to add a fourth child, named Dogbert, on port 2961...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Dogbert", "localhost", 2961));
   msg.AddString(SH_NAME_TO, "/");
   PostFullMessage("Adding Dogbert", node, &msg);

   pause(2, "Press return to add two children to Jeremy (Ratbert, on port 2962, and Clone, which will run as thread in Jeremy's process)...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("Clone"));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("Ratbert", "localhost", 2962));
   msg.AddString(SH_NAME_TO, "/Jeremy");
   PostFullMessage("Adding Ratbert & Clone", node, &msg);
    
   pause(3, "Press return to send a message to all direct children of the root node.\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*");
   PostFullMessage("Sending message to all children", node, &msg);
   
   pause(2, "Press return to send a message to all grandchildren.\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("Sending message to all granchildren", node, &msg);
   
   pause(2, "Press return to send a message to all granchildren and children...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*");
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("sending message to children & grandchildren", node, &msg);

   pause(2, "Press return to cache files 'file1' and 'file2' on all nodes.\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   // Note that we don't specify SH_NAME_TO in the BMessage.  
   // Not specifying it means "broadcast this BMessage to all nodes!"
   SHFileSpec spec;
   if (spec.AddFlavor(SHFlavor("file1")) != B_NO_ERROR) printf("Warning, couldn't find file1!!!\n");
   if (spec.AddFlavor(SHFlavor("file2")) != B_NO_ERROR) printf("Warning, couldn't find file2!!!\n");
   msg.AddFlat(SH_NAME_FILES, &spec);
   PostFullMessage("sending SHFileSpecs", node, &msg);

   pause(4, "Press return to kill the grandchildren...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_QUIT;
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("Killing granchildren", node, &msg);
   
   pause(2, "Press return to kill the children...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_QUIT;
   msg.AddString(SH_NAME_TO, "/*");
   PostFullMessage("Killing children", node, &msg);

   pause(1, "Press return to clean up and exit.\n");   
   printf("Cleaning up!\n");  
   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   printf("bye bye!\n");
   return 0; 
}