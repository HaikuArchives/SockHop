#include <stdio.h>
#include <sockhop/SockHop.h>
#include <sockhop/SHWildPathSorter.h>

class DebugLooper : public BLooper
{
private:
   virtual void MessageReceived(BMessage * msg)
   {
    //  printf("---BEGIN USER CODE MESSAGE---\n");
      msg->PrintToStream();
      if (msg->what == SH_CODE_CONNECTIONOPEN) printf("connection open...\n");
      if (msg->what == SH_CODE_CONNECTIONCLOSED) printf("connection closed...\n");
      
    //  printf("---END USER CODE MESSAGE---\n");
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

int main(int, char **)
{
   const int numAcceptors = 30;
   const int numMessages = 100;
   SHSessionAcceptor * acceptors[numAcceptors];
   SHDirectConnection * connections[numAcceptors];
   
   DebugLooper * debugLooper = new DebugLooper;
   debugLooper->Run();

int count = 0;
while(1)
{
   printf("%i\n",count++);
//   pause("Press return to add a bunch of SHSessionAcceptors...\n");
   for(int i=0; i<numAcceptors; i++)
   {
      acceptors[i]   = new SHSessionAcceptor(BMessenger(debugLooper), new SHDefaultAccessPolicy(2958+i), true);
   }
   
   
   for(int i=0; i<numAcceptors; i++)
   {
      connections[i] = new SHDirectConnection(BMessenger(debugLooper), SHNodeSpec("", "localhost", 2958+i), true);
   }

   for(int j=0; j<numMessages; j++)
   {
      BMessage m(15);
      m.AddString("garbage", "ha ha!");
      
      for (int k=0; k < numAcceptors; k++)
      {
         connections[k]->GetMessenger().SendMessage(&m);
      }
   }   

//   pause("Press return to exit.\n");

   for (int k = 0; k < numAcceptors; k++)
   {
      delete acceptors[k];
      delete connections[k];
   }

   fprintf(stderr, "Hey! %i\n", count);
}
   
   if (debugLooper->Lock()) debugLooper->Quit();
   return 0; 
 }