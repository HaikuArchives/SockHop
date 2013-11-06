#include <stdio.h>

// Just a simple SockHop client program to test connections...
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
   printf("\n\n\n");
   printf(msg);
   printf("\n\n\n");
   gets(temp);   
}

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
   DebugLooper * debugLooper = new DebugLooper;
   debugLooper->Run();

   BLooper * node = SHCreateRootNode(BMessenger(debugLooper));
   node->Run();

   pause("Press return to add Jeremy, Joanna, Joellen\n");   
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);

   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jed", "localhost", 2956));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joaquin", "localhost", 2957));


   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jeremy", "localhost", 2958));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joanna", "localhost", 2959));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Joellen", "localhost", 2960));
//msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("Mac", "dekiru", 2959));   


   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jack", "localhost", 2961));
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Jorge", "localhost", 2962));


   PostFullMessage("Adding Jeremy, Joanna, Joellen", node, &msg);

pause("Press return to send a message to all children...\n");
msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
msg.AddString(SH_NAME_TO, "/*");
PostFullMessage("Sending message to all children", node, &msg);

   pause("Press return to add Dogbert...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   msg.AddFlat(SH_NAME_CHILDREN,&SHNodeSpec("Dogbert", "localhost", 2961));
   msg.AddString(SH_NAME_TO, "/");
   PostFullMessage("Adding Dogbert", node, &msg);

   pause("Press return to add children to /Jeremy (Ratbert and an in-process Clone)...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("Clone"));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("Ratbert", "localhost", 2962));
   msg.AddString(SH_NAME_TO, "/Jeremy");
   PostFullMessage("Adding Ratbert & Clone", node, &msg);

   pause("Press return to send a message to all children...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*");
   PostFullMessage("Sending message to all children", node, &msg);
   
   pause("Press return to send a message to all grandchildren...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("Sending message to all granchildren", node, &msg);
   
   pause("Press return to send a message to all granchildren and children...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_BASE;
   msg.AddString(SH_NAME_TO, "/*");
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("sending message to children & grandchildren", node, &msg);

   pause("Press return to everyone some SHFileSpecs...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_ADDCOMPONENTS;
   SHFileSpec file1;  
   if (file1.AddFlavor(SHFlavor("file1")) != B_NO_ERROR) printf("Error adding file1 flavor!\n"); 
   if (file1.AddFlavor(SHFlavor("file3")) != B_NO_ERROR) printf("Error adding file3 flavor!\n");
   SHFileSpec file2;  
   if (file2.AddFlavor(SHFlavor("file2.x86", "file2.native", SH_ARCH_BEOS_X86, false)) != B_NO_ERROR) printf("Error adding file2 flavor!\n");
   if (file2.AddFlavor(SHFlavor("file2.ppc", "file2.native", SH_ARCH_BEOS_PPC, false)) != B_NO_ERROR) printf("Error adding file2 flavor!\n");

   if (file2.AddFlavor(SHFlavor("file4.x86", SH_ARCH_BEOS_X86, false)) != B_NO_ERROR) printf("Error adding file4 x86 flavor!\n");
   if (file2.AddFlavor(SHFlavor("file4.ppc", SH_ARCH_BEOS_PPC, false)) != B_NO_ERROR) printf("Error adding file4 ppc flavor!\n");

   if (file2.AddFlavor(SHFlavor("file5.x86", "file5.native", SH_ARCH_BEOS_X86, false)) != B_NO_ERROR) printf("Error adding file5 x86 flavor!\n");
   if (file2.AddFlavor(SHFlavor("file5.ppc", "file5.native", SH_ARCH_BEOS_PPC, false)) != B_NO_ERROR) printf("Error adding file5 ppc flavor!\n");
   
   msg.AddFlat(SH_NAME_FILES, &file1);
   msg.AddFlat(SH_NAME_FILES, &file2);   
   msg.AddString(SH_NAME_TO, "/*");
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("sending SHFileSpecs to DogBert", node, &msg);

   pause("Press return to kill the grandchildren...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_QUIT;
   msg.AddString(SH_NAME_TO, "/*/*");
   PostFullMessage("Killing granchildren", node, &msg);
   
   pause("Press return to kill the children...\n");
   msg.MakeEmpty(); msg.what = SH_COMMAND_QUIT;
   msg.AddString(SH_NAME_TO, "/*");
   PostFullMessage("Killing children", node, &msg);

   pause("Press return to clean up...\n");
   
   printf("Cleaning up!\n");  
   if (node->Lock()) node->Quit();
   if (debugLooper->Lock()) debugLooper->Quit();
   printf("bye bye!\n");
   return 0; 
 }
 