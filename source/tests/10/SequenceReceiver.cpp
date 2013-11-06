#include <stdio.h>
#include <signal.h>
#include <sockhop/SockHop.h>
#include "SequenceReceiver.h"

#define SHSR_NAME_NAME "SequenceName"

static bool signalReceived = false;

SequenceReceiver::
SequenceReceiver(const char * name) 
{
    _count = 0;
    _name = new char[strlen(name)+1];
    strcpy(_name, name);  
}

SequenceReceiver::
SequenceReceiver(BMessage * arch)
  : SHWorker(arch)
{
   _count = 0;
   const char * str = arch->FindString(SHSR_NAME_NAME);
   if (str == NULL)
   {
       printf("SequenceReceiver:  couldn't find my name!\n");
       str = "<unknown>";
   }
   _name = new char[strlen(str)+1];
   strcpy(_name, str);
printf("SequenceReceiver %p:[%s] created from archive!\n",this,GetName());
}

status_t
SequenceReceiver::GetAddOnSpec(SHFileSpec & spec) const
{
   spec.AddFlavor(SHFlavor("./SequenceReceiver_x86", SH_ARCH_BEOS_X86, true));
   //spec.AddFlavor("add-ons/intel/SequenceReceiver_x86", SH_ARCH_INTEL, true);
   //spec.AddFlavor("add-ons/powerpc/SequenceReceiver_ppc", SH_ARCH_POWERPC, true);
   return B_NO_ERROR;
}

SequenceReceiver::
~SequenceReceiver()
{
   Stop();   // necessary to avoid race conditions in GetName()
   delete []_name;
}

const char *
SequenceReceiver::
GetName() const
{
   return _name;
}

BArchivable *
SequenceReceiver::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "SequenceReceiver")) return NULL;
   return new SequenceReceiver(archive);
}

status_t
SequenceReceiver::
Archive(BMessage * archive, bool deep) const
{
   status_t ret = SHWorker::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret; 
   archive->AddString(SHSR_NAME_NAME, _name);
   return B_NO_ERROR; 
}

void
SequenceReceiver::
MessageReceived(BMessage * msg)
{
   int32 msgID;
   if (msg->FindInt32(SHSR_NAME_COUNT, &msgID) == B_NO_ERROR)
   {
      if (msgID != _count) 
          printf("%p: Error, wrong id (%li, was expecting %i)\n",this, msgID,_count);
      _count = msgID+1;      
      if ((_count % 10) == 0) 
      {
         printf(".");
         fflush(stdout);
      }
   }
   else
   {
      printf("Error, message had no counter in it! (was expecting %i)\n",_count);
      msg->PrintToStream();
   }
}

void
SequenceReceiver::
SetName(const char * newName)
{
   delete[] _name;
   _name = new char[strlen(newName)+1];
   strcpy(_name, newName);
}

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


void SendFullMessage(BLooper * looper, BMessage * msg);
void SendFullMessage(BLooper * looper, BMessage * msg)
{
   BMessage onSuccess('SUCC'); onSuccess.AddString(SH_NAME_TO, "/..");
   BMessage onFailure('FAIL'); onFailure.AddString(SH_NAME_TO, "/..");
   BMessage whenDone('DONE');  whenDone.AddString(SH_NAME_TO, "/..");
//   msg->AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
   msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
//   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   if (BMessenger(looper).SendMessage(msg) != B_NO_ERROR) printf("WARNING:  SendFullMessage, message Send failed!\n");
}

void onSignal(int);
void onSignal(int)
{
   signalReceived = true;   
}

// Test driver too!
int main(int, char **)
{
   int32 count = 0;
//   int32 delay = 00000;

   signal(SIGINT, onSignal);
   
   // Make receiver node...   
   BLooper * debugLooper = new DebugLooper;
   debugLooper->Run();
   
   // Make root node...
   BLooper * root = SHCreateRootNode(BMessenger(debugLooper));
   root->Run();

   pause("Press return to set root node to compression mode.\n");
   BMessage batch(SH_COMMAND_SETPARAMETERS);
   batch.AddInt32(SH_PARAMNAME_TRANSMISSIONENCODING, SH_ENCODING_ZLIBHEAVY);
   SendFullMessage(root, &batch);   

pause("step 1...");   
   // set up children to send to...
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("asdf", "localhost"));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("jkl;", "localhost", 2959));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("aeou", "localhost", 2960));
   SendFullMessage(root, &msg);

pause("step 2...");   
   // and grandchildren to send to...
   msg.MakeEmpty();
   msg.AddString(SH_NAME_TO, "/asdf");
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("baby1", "localhost", 2961));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("baby2", "localhost", 2962));
   msg.AddFlat(SH_NAME_CHILDREN, &SHNodeSpec("baby3", "localhost", 2963));
   SendFullMessage(root, &msg);
pause("step 3...");   

   // Give them all a Receiver...
   BMessage r(SH_COMMAND_ADDCOMPONENTS);
   r.AddString(SH_NAME_TO, "/*");
   r.AddString(SH_NAME_TO, "/*/*");
   SequenceReceiver sr("test");
   BMessage pack;
   sr.Archive(&pack);
   for (int j=0; j<5; j++) r.AddMessage(SH_NAME_WORKERS, &pack);
   SendFullMessage(root, &r);
   
pause("step 4...");   
   
   // and send the receiver messages.
   while(1)
   {   
      if (signalReceived == true) break;
      
      snooze(50000);
      if ((count % 100) == 0) printf("Sending message %li\n",count);

      BMessage msg(123);
	  msg.AddInt32(SHSR_NAME_COUNT, count);

char temp[5000];
for (int i=0; i<(int)sizeof(temp); i++) temp[i] = 'x';
temp[4999] = '\0';
	  
	  msg.AddString("junk", temp);
	  
	  SendFullMessage(root, &msg);
      
      count++;
   }
   printf("bye!\n");        
   return 0;
}
