// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#include <stdio.h>
#include <string.h>

#include <interface/MenuBar.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>
#include <interface/Button.h>
#include <interface/ScrollView.h>
#include <interface/StringView.h>
#include <interface/Box.h>
#include <interface/Alert.h>
#include <interface/Rect.h>

#include <storage/FilePanel.h>
#include <storage/Path.h>
#include <storage/File.h>

#include <support/Autolock.h>

#include <app/Application.h>

#include "SockDemo.h"
#include "SockHopDemoWindow.h"

#define SD_WINDOW_LEFT   50.0f
#define SD_WINDOW_TOP    50.0f

#define SD_WINDOW_WIDTH  500.0f
#define SD_WINDOW_HEIGHT 400.0f

#define CP {printf("CheckPoint: %s:%i\n", __FILE__, __LINE__);}

SockHopDemoWindow:: 
SockHopDemoWindow()
  : BWindow(BRect(SD_WINDOW_LEFT,SD_WINDOW_TOP,SD_WINDOW_WIDTH+SD_WINDOW_LEFT,SD_WINDOW_HEIGHT+SD_WINDOW_TOP), "SockHop Demo Application", B_TITLED_WINDOW, B_NOT_RESIZABLE),
    _selectedNode(NULL), _lastOpenWasInsert(false), _savePanel(NULL), _openPanel(NULL), _jobCount(1), _currentOpCounter(0), 
    _pulseThread(-1), _opsActive(false)
{
   for (unsigned int i=0; i < sizeof(_opsPerSecond)/sizeof(uint32); i++) _opsPerSecond[i] = 0;
   
   BMenuBar * menuBar = new BMenuBar(BRect(0, 0, 0, 0), "Menu Bar");
   
   BMenu * projectMenu = new BMenu("Project");
   projectMenu->AddItem(new BMenuItem("About SockHop Demo", new BMessage(SD_COMMAND_ABOUT_SOCKHOPDEMO)));
   projectMenu->AddItem(new BSeparatorItem);
   projectMenu->AddItem(new BMenuItem("Open Node Tree...",  new BMessage(SD_COMMAND_OPEN)));
   projectMenu->AddItem(new BMenuItem("Insert Node Tree...",new BMessage(SD_COMMAND_INSERT)));
   projectMenu->AddItem(new BMenuItem("Close",              new BMessage(SD_COMMAND_CLOSE)));
   projectMenu->AddItem(_saveMenuItem = new BMenuItem("Save", new BMessage(SD_COMMAND_SAVE)));
   _saveMenuItem->SetEnabled(false);  // til we do a Save As...
   projectMenu->AddItem(new BMenuItem("Save As...",         new BMessage(SD_COMMAND_SAVE_AS)));
   menuBar->AddItem(projectMenu);
   
   BMenu * speedMenu = new BMenu("Tempo");
   BMenuItem * defItem;
   speedMenu->AddItem(new BMenuItem("Ennui",        MakeTempoMessage(1)));
   speedMenu->AddItem(new BMenuItem("Dull",         MakeTempoMessage(2)));
   speedMenu->AddItem(defItem = new BMenuItem("Pleasant",     MakeTempoMessage(3)));
   speedMenu->AddItem(new BMenuItem("Hectic",       MakeTempoMessage(4)));
   speedMenu->AddItem(new BMenuItem("Balls Out",    MakeTempoMessage(5)));
   speedMenu->SetRadioMode(true);
   menuBar->AddItem(speedMenu);
   
   defItem->SetMarked(true);   
   _tempo = 3;

   BMenu * helpMenu = new BMenu("Help");
   helpMenu->AddItem(new BMenuItem("About SockHop",         new BMessage(SD_COMMAND_ABOUT_SOCKHOP)));
   helpMenu->AddItem(new BMenuItem("Setting Up",            new BMessage(SD_COMMAND_SETTING_UP)));
   helpMenu->AddItem(new BMenuItem("Using the GUI",         new BMessage(SD_COMMAND_USING_GUI)));
   helpMenu->AddItem(new BMenuItem("Add-Ons",               new BMessage(SD_COMMAND_ADD_ONS)));
   menuBar->AddItem(helpMenu);   
   
   AddChild(menuBar);

   float viewTop = menuBar->Bounds().bottom + 1.0f;

   BRect deleteNodesButtonRect(SD_VIEW_SPACING, 
                               SD_WINDOW_HEIGHT - SD_VIEW_SPACING - SD_TEXT_HEIGHT - SD_COMPONENT_SPACING, 
                               SD_WINDOW_WIDTH/2.0f, 
                               SD_WINDOW_HEIGHT - SD_VIEW_SPACING);
   AddChild(_deleteNodesButton = new BButton(deleteNodesButtonRect, "Delete Selected Nodes", "Delete Selected Nodes", new BMessage(SD_COMMAND_DELETE_NODES)));

   BRect nodeBoxRect(SD_VIEW_SPACING,viewTop+SD_VIEW_SPACING,SD_WINDOW_WIDTH/2,deleteNodesButtonRect.top-SD_COMPONENT_SPACING);
   _treeViewBox = MakeBBox(nodeBoxRect, "Node Tree");
   nodeBoxRect.right -= SD_VIEW_SPACING * 2;
   nodeBoxRect.bottom -= SD_VIEW_SPACING * 2;
   
   _treeView = new BOutlineListView(nodeBoxRect, "Node Tree", B_MULTIPLE_SELECTION_LIST);
   BScrollView * treeScroll = new BScrollView("treeScroll", _treeView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, true, true, B_NO_BORDER);
   _treeViewBox->AddChild(treeScroll);
   AddChild(_treeViewBox);
  
   BRect busyMessageRect(SD_WINDOW_WIDTH/2.0f + SD_VIEW_SPACING, 
                        viewTop + SD_WINDOW_HEIGHT/2.0f - SD_TEXT_HEIGHT - SD_VIEW_SPACING,
                        SD_WINDOW_WIDTH - SD_VIEW_SPACING,
                        viewTop + SD_WINDOW_HEIGHT/2.0f - SD_VIEW_SPACING);
   BBox * box = new BBox(busyMessageRect);
   float busyBoxHeight = busyMessageRect.Height();
   busyMessageRect.right -= busyMessageRect.left + 2.0f;
   busyMessageRect.bottom -= busyMessageRect.top + 2.0f;
   busyMessageRect.left = 1;
   busyMessageRect.top = 1;
   _busyMessage = new BStringView(busyMessageRect, "NumJobsGoing", "", 0L);
   box->AddChild(_busyMessage);
   AddChild(box);
   
   _busyMessage->SetAlignment(B_ALIGN_CENTER);
    
   BRect nodeSpecRect(SD_WINDOW_WIDTH/2 + SD_VIEW_SPACING, 
                      viewTop + SD_VIEW_SPACING, 
                      SD_WINDOW_WIDTH - SD_VIEW_SPACING, 
                      viewTop + SD_WINDOW_HEIGHT/2 - SD_VIEW_SPACING - busyBoxHeight - SD_VIEW_SPACING);
   box = MakeBBox(nodeSpecRect, "Node Specification");
   box->AddChild(_nodeSpecView = new NodeSpecView(nodeSpecRect)); 
   AddChild(box);
   
   BRect deleteWorkersButtonRect(SD_WINDOW_WIDTH/2 + SD_VIEW_SPACING, 
                          SD_WINDOW_HEIGHT - SD_VIEW_SPACING - SD_TEXT_HEIGHT - SD_COMPONENT_SPACING, 
                          SD_WINDOW_WIDTH - SD_VIEW_SPACING, 
                          SD_WINDOW_HEIGHT - SD_VIEW_SPACING);
   AddChild(_deleteWorkersButton = new BButton(deleteWorkersButtonRect, "Delete Selected Workers", "Delete Selected Workers", new BMessage(SD_COMMAND_DELETE_WORKERS)));

   BRect workerListSpecRect(SD_WINDOW_WIDTH/2 + SD_VIEW_SPACING, viewTop + SD_WINDOW_HEIGHT/2, 
                            SD_WINDOW_WIDTH - SD_VIEW_SPACING, deleteWorkersButtonRect.top - SD_COMPONENT_SPACING);   
   box = MakeBBox(workerListSpecRect, "Workers on Selected Node");
   workerListSpecRect.right -= SD_VIEW_SPACING * 2;
   _workerView = new BListView(workerListSpecRect, "Worker List", B_MULTIPLE_SELECTION_LIST);
   _workerView->SetSelectionMessage(new BMessage(SD_COMMAND_WORKERLIST_SELECT));
   _workerView->SetInvocationMessage(new BMessage(SD_COMMAND_WINDOW_TO_FRONT));
   BScrollView * workerScroll = new BScrollView("workerScroll", _workerView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true, B_NO_BORDER);
   box->AddChild(workerScroll);
   AddChild(box);

   _nodeRoot = new NodeTreeItem(SHNodeSpec("/"));
   _treeView->AddItem(_nodeRoot);
   _treeView->SetSelectionMessage(new BMessage(SD_COMMAND_NODETREE_SELECT));   

   _sockRoot = SHCreateRootNode(BMessenger(this), new SHDefaultAccessPolicy(0, 1));
   _sockRoot->Run();
   
   // Set Disabled buttons, etc
   PostMessage(SD_COMMAND_NODETREE_SELECT);
   
   // Set job count
   PostMessage(SD_COMMAND_OPERATION_DONE);
}

BMessage * 
SockHopDemoWindow::
MakeTempoMessage(int32 tempo)
{
   BMessage * msg = new BMessage(SD_COMMAND_TEMPO);
   msg->AddInt32(SD_NAME_TEMPO, tempo);
   return msg;
}

SockHopDemoWindow::
~SockHopDemoWindow()
{
   _treeView->RemoveItem(_nodeRoot);
   delete _nodeRoot;

   if (_pulseThread != -1)
   {
      BAutolock lock(_pulseLock);
      kill_thread(_pulseThread);
   }
      
   if (_sockRoot->Lock()) _sockRoot->Quit();

   delete _savePanel;
   delete _openPanel;
         
   be_app->PostMessage(B_QUIT_REQUESTED);
}

BBox *
SockHopDemoWindow::
MakeBBox(BRect & rect, const char * label)
{
   BBox * box = new BBox(rect, NULL);
   box->SetLabel(label);
   box->SetViewColor(255,255,255);

   // reduce child's rect so we can still see the BBox's border...
   rect = BRect(SD_COMPONENT_SPACING, 2*SD_VIEW_SPACING, rect.Width() - SD_COMPONENT_SPACING, rect.Height() - SD_COMPONENT_SPACING);
   return box;
}

void
SockHopDemoWindow ::
UpdateSelectedItem()
{
   bool isSomethingSelected = (_selectedNode != NULL);
   
   if (_selectedNode) 
   {
      SHNodeSpec spec = _selectedNode->GetNodeSpec();
      _nodeSpecView->setDisplayedSpec(&spec);
   }
   else _nodeSpecView->setDisplayedSpec(NULL);
   
   _nodeSpecView->setAddNodeEnabled(isSomethingSelected);
   _deleteNodesButton->SetEnabled(isSomethingSelected);
   _deleteWorkersButton->SetEnabled(false);   // because no workers are selected now

   // update worker list
   _workerView->MakeEmpty();
   if (_selectedNode) _workerView->AddList(_selectedNode->GetWorkerList());
}
      

void
SockHopDemoWindow::
AddHighlightedNodesAddresses(BMessage * add)
{
	int32 sel;
	int i = 0;
	while((sel = _treeView->CurrentSelection(i++)) >= 0)
	{
	   NodeTreeItem * item = (NodeTreeItem *) _treeView->ItemAt(sel);
	   
	   char * path = item->GetPath(_treeView);
	   add->AddString(SH_NAME_TO, path);
	   delete [] path;
	}
}

void
SockHopDemoWindow::
ResetNodeTree()               
{
   // delete the root node and replace it!
   if (_sockRoot->Lock()) _sockRoot->Quit();

   // turn off the current item 
   _selectedNode = NULL;
   UpdateSelectedItem();
   
   // Reset our node tree to just the root node
   _treeView->MakeEmpty(); //RemoveItem((int32)0);
   delete _nodeRoot;
   _nodeRoot = new NodeTreeItem(SHNodeSpec("/"));
   _treeView->AddItem(_nodeRoot);
   
   // create new root node
   _sockRoot = SHCreateRootNode(BMessenger(this));
   _sockRoot->Run(); 
   
   _jobCount = 0;
   UpdateJobCountDisplay();
}

bool
SockHopDemoWindow::
SaveTreeTo(const NodeTreeItem * subTree, BEntry * saveEntry) const
{
   // Recurse down the subTree, making a big BMessage out of things!
   BFile saveTo(saveEntry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
   if (saveTo.InitCheck() != B_NO_ERROR) return false;
   
   BMessage saveMsg;
   subTree->SaveTreeToMessage(&saveMsg);
   return (saveMsg.Flatten(&saveTo) == B_NO_ERROR);
}

void
SockHopDemoWindow::
PostTreeLoadMessages(const BMessage * readFrom, char * nodePath, const int nodePathLen)
{
   BMessage msg(SH_COMMAND_ADDCOMPONENTS);

   // First send all creation messages for children of (readFrom)...
   int i=0;
   BMessage nextChild;
   while(readFrom->FindMessage(SD_NAME_CHILD, i++, &nextChild) == B_NO_ERROR)
   {
      SHNodeSpec nextChildSpec;
      if (nextChild.FindFlat(SD_NAME_NODESPEC, &nextChildSpec) == B_NO_ERROR)
           msg.AddFlat(SH_NAME_CHILDREN, &nextChildSpec);
      else printf("MakeLoadMessage:  Warning, couldn't get nodeSpec for child, skipping...\n");
   }
   msg.AddString(SH_NAME_TO, nodePath);
   if (i > 1) 
   {
      BMessage onSuccess(SD_COMMAND_NODE_ADD_SUCCEEDED);
      onSuccess.AddString(SH_NAME_TO, "/..");
      msg.AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
            
      BMessage onFailure(SD_COMMAND_NODE_ADD_FAILED);
      onFailure.AddString(SH_NAME_TO, "/..");
      msg.AddMessage(SH_NAME_ONFAILURE, &onFailure);
      
      PostSockHopMessage(&msg);
   }
   
   // Now recurse to handle their children.
   char * endOfPath = strchr(nodePath, '\0');
   *endOfPath = '/';
   int spaceAvailable = (&nodePath[nodePathLen-1] - endOfPath) - 1;  // chars available for child's name, not including NUL byte
   i=0;
   while(readFrom->FindMessage(SD_NAME_CHILD, i++, &nextChild) == B_NO_ERROR)
   {
      SHNodeSpec nextChildSpec;
      if (nextChild.FindFlat(SD_NAME_NODESPEC, &nextChildSpec) == B_NO_ERROR)
      {
          if ((int) strlen(nextChildSpec.GetNodeName()) <= spaceAvailable)
          {
             strcpy(endOfPath+1, nextChildSpec.GetNodeName());
             PostTreeLoadMessages(&nextChild, nodePath, nodePathLen);
          }
          else printf("MakeLoadMessage:  Couldn't recurse to child [%s], out of buffer space.\n", nextChildSpec.GetNodeName());
      }
      else printf("MakeLoadMessage:  Warning2, couldn't get nodeSpec for child, skipping...\n");
   }
   *endOfPath = '\0';  // clean up
}

long
SockHopDemoWindow::
PulseLoop(void * data)
{
   SockHopDemoWindow * This = (SockHopDemoWindow *) data;
   while(1)
   {
      snooze(1000000);
      BAutolock lock(This->_pulseLock);  // so we won't get killed in the middle of a post... I don't know if that would be bad or not...
      This->PostMessage(SD_COMMAND_UPDATE_OPS_PER_SECOND);
   }
}

void
SockHopDemoWindow::
MessageReceived(BMessage * msg)
{
   if (_pulseThread == -1)
   {
      // For the label of the Tree view...
      _pulseThread = spawn_thread(PulseLoop, "SockDemo update pulse", B_DISPLAY_PRIORITY, this);
      resume_thread(_pulseThread);
   }

   switch(msg->what)
   {
      case SD_COMMAND_WORKER_STATS:
         // record a number of ops done by a worker...
         int32 ops;
         if (msg->FindInt32(SD_NAME_STAT, &ops) == B_NO_ERROR) _opsPerSecond[_currentOpCounter] += ops;
      break;
      
      case SD_COMMAND_UPDATE_OPS_PER_SECOND:
      {
         int totalOps = 0;
         int numSlots = sizeof(_opsPerSecond)/sizeof(uint32);
         for (int i=0; i<numSlots; i++) totalOps += _opsPerSecond[i];
         _currentOpCounter = (_currentOpCounter + 1) % numSlots;
         _opsPerSecond[_currentOpCounter] = 0;
         totalOps /= numSlots;
         if (totalOps == 0) 
         {
            if (_opsActive) _treeViewBox->SetLabel("Node Tree");
            _opsActive = false;
         }
         else
         {
            char temp[100];
            sprintf(temp, "Node Tree (%i ops/second)", totalOps);
            _treeViewBox->SetLabel(temp);         
            _opsActive = true;
         }
      }
      break;
      
      case SD_COMMAND_ADD_NODE:
         {
            SHNodeSpec spec = _nodeSpecView->GetDisplayedSpec();

            if (spec.IsNodeNameValid())
            {            
               BMessage add(SH_COMMAND_ADDCOMPONENTS);
               add.AddFlat(SH_NAME_CHILDREN, &spec);
                           BMessage onSuccess(SD_COMMAND_NODE_ADD_SUCCEEDED);
               onSuccess.AddString(SH_NAME_TO, "/..");
               add.AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
            
               BMessage onFailure(SD_COMMAND_NODE_ADD_FAILED);
               onFailure.AddString(SH_NAME_TO, "/..");
               add.AddMessage(SH_NAME_ONFAILURE, &onFailure);
            
               AddHighlightedNodesAddresses(&add);
            
               if (add.HasString(SH_NAME_TO)) PostSockHopMessage(&add);
            
               PostMessage(SD_COMMAND_NODETREE_SELECT);
             }
             else UserMessage("The Node Specification you entered was not valid.  Please make sure the Node Name field does not have the '/' character in it.");
         }
         break;

      case SD_COMMAND_OPERATION_DONE:
         _jobCount--;
         UpdateJobCountDisplay();
         break;
         
      case SD_COMMAND_NODE_ADD_SUCCEEDED:
         {
            SHNodeSpec spec;
            if (msg->FindFlat(SH_NAME_REGARDING, &spec) == B_NO_ERROR)
            {
               const char * from;
               NodeTreeItem * parent;
               
               if (((from = msg->FindString(SH_NAME_FROM)) != NULL)&&((parent = _nodeRoot->GetNodeByPath(from)) != NULL))
               {
                  NodeTreeItem * newItem = new NodeTreeItem(spec);
                  parent->AddChild(newItem);
                  _treeView->AddUnder(newItem, parent);
               }
            }
         }
         break;
      
      // This will be sent to us anytime a connection to a node is closed, for any reason.
      // Note that any children of the node will be gone too!
      case SD_COMMAND_NODE_ADD_FAILED:
         {
            SHNodeSpec spec;
            if (msg->FindFlat(SH_NAME_REGARDING, &spec) == B_NO_ERROR)
            {
               const char * from;
               NodeTreeItem * parent;
               
               if (((from = msg->FindString(SH_NAME_FROM)) != NULL)&&((parent = _nodeRoot->GetNodeByPath(from)) != NULL))
               {
                  NodeTreeItem * deadItem = parent->GetChildBySpec(spec);
                  if (deadItem)
                  {
                     parent->RemoveChild(deadItem);
                     _treeView->RemoveItem(deadItem);
                     if ((_selectedNode)&&(deadItem->IsAncestorOf(_selectedNode)))
                     {
                        _selectedNode = NULL;
                        UpdateSelectedItem();
                     }
                     delete deadItem;
                  }
                  else
                  {
                     char temp[2000];
                     sprintf(temp, "There was an error adding the child [%s]\nto node [%s], at location [%s:%li]",
                            spec.GetNodeName(), parent->GetNodeSpec().GetNodeName(), spec.GetHostName(), spec.GetPortNumber());
                     UserMessage(temp);
                  }
               }
            }
         }
         break;
                  
      case SD_COMMAND_DELETE_NODES:
         {
            int32 sel;
            int i = 0;
            BMessage rem(SH_COMMAND_QUIT);
            while((sel = _treeView->CurrentSelection(i++)) >= 0)
            {
               NodeTreeItem * item = (NodeTreeItem *) _treeView->ItemAt(sel);
               char * path = item->GetPath(_treeView);
               
               // Special case for root node:  No need to send a BMessage; instead just Quit() it and make a new one.
               if ((path[0] == '/')&&(path[1] == '\0')) 
               {
                  ResetNodeTree();
                  rem.MakeEmpty();   // Don't need to post message to the new _nodeRoot!
                  break;  // We already deleted everybody, no point in looking for more nodes to delete!
               }
               else rem.AddString(SH_NAME_TO, path);
               delete [] path;
            }
            if (rem.HasString(SH_NAME_TO)) PostSockHopMessage(&rem);
         }
         break;

      case SD_COMMAND_DELETE_WORKERS:
         {
             int index = 0;
             int sel;
             
             BMessage delWorkers(SH_COMMAND_REMOVECOMPONENTS);
             
             while((sel = _workerView->CurrentSelection(index++)) >= 0)
             {
                BStringItem * nextItem = (BStringItem *) _workerView->ItemAt(sel);
                delWorkers.AddString(SH_NAME_WORKERS, nextItem->Text());
             }
             
             AddHighlightedNodesAddresses(&delWorkers);
             if (delWorkers.HasString(SH_NAME_TO)) PostSockHopMessage(&delWorkers);
         }      
         break;
      
      case SD_COMMAND_NODETREE_SELECT:
         {
             int sel = _treeView->CurrentSelection();
             _selectedNode = (sel >= 0) ? ((NodeTreeItem *) _treeView->ItemAt(sel)) : NULL;
	         UpdateSelectedItem();
         }
         break;

      case SD_COMMAND_WINDOW_TO_FRONT:
         {
             int index = 0;
             int sel;
             
             BMessage delWorkers(SD_COMMAND_WINDOW_TO_FRONT);
             
             while((sel = _workerView->CurrentSelection(index++)) >= 0)
             {
                BStringItem * nextItem = (BStringItem *) _workerView->ItemAt(sel);
                delWorkers.AddString(SH_NAME_TOWORKERS, nextItem->Text());
             }
             
             AddHighlightedNodesAddresses(&delWorkers);
             if (delWorkers.HasString(SH_NAME_TO)) PostSockHopMessage(&delWorkers);
         }      
      break;
      
      case SD_COMMAND_WORKERLIST_SELECT:
         _deleteWorkersButton->SetEnabled(_workerView->CurrentSelection() >= 0);
         break;
         
      // Handle when the user drags an add-on file into our window.
      // Instantiates an object from the add on and sends it to all
      // selected nodes!
      case B_SIMPLE_DATA:
         {
            entry_ref nextRef;
            int index = 0;
            BMessage addWorkers(SH_COMMAND_ADDCOMPONENTS);
            while(msg->FindRef("refs", index++, &nextRef) == B_NO_ERROR)
            {
               BEntry be(&nextRef, true);
               BPath pa; be.GetPath(&pa);
               
               // Determine the class name from the file name (a bit of a hack, it's true...)
               char * lastSlash = strrchr(pa.Path(), '/');
               if (lastSlash)
               {
                   // Fake a BMessage archive so we can load it in with SHCreateDistributableObject()
                   BMessage msg;
                   msg.AddString("class", lastSlash+1);
                   SHFileSpec spec;
                   if (spec.AddFlavor(SHFlavor(pa.Path(), SHGetArchitectureBits(), true)) == B_NO_ERROR)
                   {
                      msg.AddFlat(SH_NAME_ADDONSPEC, &spec);
                   
                      SHDistributableObject * newObj = SHCreateDistributableObject(&msg);
                      SHWorker * newWorker = newObj ? cast_as(newObj, SHWorker) : NULL;
                      if (newWorker)
                      {
                         BMessage archive;
                         newWorker->Archive(&archive);
                         addWorkers.AddMessage(SH_NAME_WORKERS, &archive);
                      }
                      else UserMessage("I couldn't get an SHWorker from the file you dragged in.  Make sure the file was a SockDemo add-on, and that it hasn't been renamed (I depend on the filename to tell me the classname)");
                      if (newObj) SHDeleteDistributableObject(newObj);  
                  }
                  else UserMessage("Error, couldn't read add-on file!");
               }
            }
            
            // only send the message if we have valid workers archives to send in it!
            if (addWorkers.HasMessage(SH_NAME_WORKERS))
            {
               // add the address of every highlighted worker
               AddHighlightedNodesAddresses(&addWorkers);
               
               if (addWorkers.HasString(SH_NAME_TO)) 
               {
                  BMessage onSuccess(SD_COMMAND_WORKER_ADD_SUCCEEDED);
                  onSuccess.AddString(SH_NAME_TO, "/..");
                  addWorkers.AddMessage(SH_NAME_ONSUCCESS, &onSuccess);
                  
                  BMessage onFailure(SD_COMMAND_WORKER_ADD_FAILED);
                  onFailure.AddString(SH_NAME_TO, "/..");
                  addWorkers.AddMessage(SH_NAME_ONFAILURE, &onSuccess);
                  
                  // Add a message to the workers to set their tempo, after they've been added.
                  BMessage setTempo(SD_COMMAND_TEMPO);
                  setTempo.AddString(SH_NAME_TO, ".");  // Will be posted from target node(s), so we address it to "current node".
                  setTempo.AddInt32(SD_NAME_TEMPO, _tempo);
                  addWorkers.AddMessage(SH_NAME_ONSUCCESS, &setTempo);
                  
                  PostSockHopMessage(&addWorkers, "There was an error adding components to one or more nodes.");
               }
               else UserMessage("Please highlight one or more nodes in the Node Tree before dragging SockDemo add-ons into the window.  The add-on will be instantiated on the nodes you highlight.");
            }
            else UserMessage("The file(s) you dropped into the SockDemo window weren't recognized as SockDemo add-ons.  Make sure you are dragging add-ons that were compiled for your machine's architecture!  (i.e. x86 add-ons on an x86 machine, etc.)");
         }
         break;
      
      case SD_COMMAND_WORKER_ADD_SUCCEEDED:
         {
            const char * from;
            if (msg->FindString(SH_NAME_FROM, &from) == B_NO_ERROR)
            {
               NodeTreeItem * node = _nodeRoot->GetNodeByPath(from);
               if (node)
               {
                  const char * workerName;
                  
                  if (msg->FindString(SH_NAME_REGARDING, &workerName) == B_NO_ERROR)
                  {
                     BStringItem * item = node->AddWorker(workerName);
                     if (node == _selectedNode) _workerView->AddItem(item);
                  }
                  else printf("Couldn't find workerName!\n");
               }
               else printf("Couldn't find NodeTreeItem?\n");
            }
         }
         break;
         
      case SD_COMMAND_USER_OPERATION_FAILED:
         {
            const char * err;
            if (msg->FindString(SD_NAME_FAILUREMESSAGE, &err) == B_NO_ERROR)
            {
               UserMessage(err);
            }
         }
         break;
              
      case SD_COMMAND_WORKER_QUITTING:
         {
            const char * from;
            if (msg->FindString(SH_NAME_FROM, &from) == B_NO_ERROR)
            {
               const char * workerName;
               if (msg->FindString(SD_NAME_WORKERNAME, &workerName) == B_NO_ERROR)
               {
                  NodeTreeItem * node = _nodeRoot->GetNodeByPath(from);
                  
                  if (node)
                  {
                     if (node == _selectedNode)
                     {
                        int num = _workerView->CountItems();
                        for (int i=0; i<num; i++)
                        {
                           BStringItem * nextItem = (BStringItem *) _workerView->ItemAt(i);
                           if (strcmp(nextItem->Text(), workerName) == 0)
                           {
                              _workerView->RemoveItem(i);
                              (void)node->RemoveWorker(nextItem);
                              break;
                           }
                        }
                        _deleteWorkersButton->SetEnabled(_workerView->CurrentSelection() >= 0);
                     }
                     else
                     {
                        BList * list = node->GetWorkerList();
                        int num = list->CountItems();
                        for (int i=0; i<num; i++)
                        {
                           BStringItem * nextItem = (BStringItem *) list->ItemAt(i);
                           if (strcmp(nextItem->Text(), workerName) == 0) 
                           {
                              list->RemoveItem(i);
                              delete nextItem;
                              break;
                           }
                        }
                     }
                  }
                  else printf("Couldn't find node [%s]\n",from);
               }
               else printf("worker quitting:  couldn't find workerName (no %s field)\n",SD_NAME_WORKERNAME);
            }
            else printf("worker quitting, no from field (%s)!\n", SH_NAME_FROM);
         }
         break;

      case SD_COMMAND_ABOUT_SOCKHOPDEMO:
         UserMessage("SockDemo 1.1\nby Jeremy Friesner\njfriesne@ucsd.edu\n\nThis is a toy demo program whose main purpose is to demonstrate the capabilities of the SockHop system.\n\nTo learn more about how to operate the SockDemo program, please see the included README file.");
      break;
      
      case B_REFS_RECEIVED:
      {
         BList insertUnder;  // list of all the nodes we should insert our tree under...
         
         if (_lastOpenWasInsert)
         {
            int i = 0;
            int sel;
            while((sel = _treeView->CurrentSelection(i++)) >= 0) insertUnder.AddItem(_treeView->ItemAt(sel));        
            
            if (insertUnder.CountItems() == 0)
            {
               BAlert *error = new BAlert("SockDemo Error", "Please select a node to insert under before choosing 'Insert Tree'.", "Oops!");
               error->Go(NULL);  // go() will delete error when it returns
               break;
            }
         }
         else
         {
            // reset the tree!
            insertUnder.AddItem(_nodeRoot);
         }

         // Find file ref         
         entry_ref ref;
         if (msg->FindRef("refs", &ref) == B_NO_ERROR)
         {
            // load the file into (fileMsg)
            BMessage fileMsg;
		    {
		       BFile file(&ref, B_READ_ONLY);
		       if ((file.InitCheck() != B_NO_ERROR)||(fileMsg.Unflatten(&file) != B_NO_ERROR))
 		       {
                  BAlert *error = new BAlert("SockDemo Error", "SockDemo was unable to open your tree file!", "Okay");
                  error->Go(NULL);  // go() will delete (error) when it returns
                  break;
               }
		    }
		   
            if (!_lastOpenWasInsert) ResetNodeTree();      
            
            int num = insertUnder.CountItems();
            for (int i=0; i<num; i++)
            {
               NodeTreeItem * nextItem = (NodeTreeItem *) insertUnder.ItemAt(i);
               char * path = nextItem->GetPath(_treeView);
		       char buffer[2000];  // should be enough space!  
		       if (path[1] == '\0') buffer[0] = '\0';   // empty string for root node to avoid '//'
		                       else strncpy(buffer, path, sizeof(buffer));
		       delete [] path;

               PostTreeLoadMessages(&fileMsg, buffer, sizeof(buffer));
            }
         }
      }
      break;
      
      case SD_COMMAND_INSERT:
	  case SD_COMMAND_OPEN:
         _lastOpenWasInsert = (msg->what == SD_COMMAND_INSERT);
         if (_openPanel) _openPanel->Show();
         else 
         {
            BMessenger toMe(this);
	        _openPanel = new BFilePanel(B_OPEN_PANEL, &toMe, NULL, 0, false);
	        _openPanel->Show();
         }
         _openPanel->SetButtonLabel(B_DEFAULT_BUTTON, _lastOpenWasInsert ? "Insert" : "Open");
	  break;

	  case SD_COMMAND_CLOSE:
	     PostMessage(B_QUIT_REQUESTED);
	  break;

	  case SD_COMMAND_SAVE:
	  {
         const char * name;
         entry_ref entry;
         if ((msg->FindString("name", &name) == B_NO_ERROR)&&(msg->FindRef("directory", &entry) == B_NO_ERROR))
         {
            BDirectory directory(&entry);
            BEntry saveTo(&directory, name, true);
            if ((saveTo.InitCheck() == B_NO_ERROR)&&(SaveTreeTo(_nodeRoot, &saveTo)))
            {
                _saveMenuItem->SetEnabled(true);
                _lastSaved = saveTo;
            }
            else
            {
                BAlert *error = new BAlert("SockDemo Error", "SockDemo was unable to save your tree!", "Okay");
                error->Go(NULL);
            }
         }
         else if (_lastSaved.InitCheck() == B_NO_ERROR)
         {
            // We've saved this before...
            if (SaveTreeTo(_nodeRoot, &_lastSaved))
            {
                _saveMenuItem->SetEnabled(true);
            }
            else
            {
                BAlert *error = new BAlert("SockDemo Error", "SockDemo was unable to save your tree!", "Okay");
                error->Go(NULL);
            }            
         }
         else PostMessage(SD_COMMAND_SAVE_AS);  // open the save requester...
	  }
	  break;

	  case SD_COMMAND_SAVE_AS:
	     if (_savePanel) _savePanel->Show();
	     else
	     {
	        BMessenger toMe(this);
	        BMessage save(SD_COMMAND_SAVE);
	        _savePanel = new BFilePanel(B_SAVE_PANEL, &toMe, NULL, 0, false, &save);
	        _savePanel->Show();
	     }
	  break;

	  case SD_COMMAND_ABOUT_SOCKHOP:
	     UserMessage("SockHop is a network-based distributed processing system for the BeOS.  With SockHop you can easily write programs that run on many BeOS machines at once.");
	  break;

	  case SD_COMMAND_SETTING_UP:
	     UserMessage("To set up this demo, execute libsockhop.so on every machine you wish to be part of your virtual computer.  If you only have one computer, you can still use this demo by running one or instances of libsockhop.so on different ports on your machine, to simulate multiple machines.  You can also use this demo without running any instances of libsockhop.so, as long as you only create local nodes (e.g. nodes without a hostname specified)\n\nFor a more complete description, please see the included README file.");
	  break;

	  case SD_COMMAND_USING_GUI:
	     UserMessage("The SockDemo GUI consists of a Node Tree view, a Node Specification View, and a worker list.  To add a new node to the tree, click on the node in the tree that is to be the parent of the new node, then enter the fields for the new node into the box at the upper right, and click 'Add Node To Tree'.  To add a worker to one or more nodes in the tree, select the nodes you wish to add the worker to, then drag the appropriate add-on file (e.g. add-ons/x86/LineDrawingWorker) into the SockDemo window.  For this to work, the add-on file must be in its original directory relative to the SockDemo executable!\n\nFor a more complete description, please see the included README file.");
	  break;

	  case SD_COMMAND_ADD_ONS:           
	     UserMessage("Add-on files for the SockDemo can be found in the add-ons/<your-architecture> directory.  To use them, click on one or more nodes in the Node Tree view, then drag their file icons into the SockDemo window.  In addition to playing with the SockDemo add-ons provided, you can write your own.  See the source directories for examples.\n\nFor a more complete description, please see the included README file.");
	  break;

      case SD_COMMAND_TEMPO:
         if (msg->FindInt32(SD_NAME_TEMPO, &_tempo) == B_NO_ERROR)
         {
              PostSockHopMessage(msg, "There was an error adjusting the drawing tempo.");
         }
      break;
      
      default:
//         printf("Got Unknown Message: (save=%i)\n", SD_COMMAND_SAVE);
//         msg->PrintToStream();
         break;
   }
}

void
SockHopDemoWindow::
UserMessage(const char * msg)
{
   BAlert * a = new BAlert("SockDemo Help", msg, "Ah, so");
   a->Go(NULL);
}

void
SockHopDemoWindow::
UpdateJobCountDisplay()
{
   if (_jobCount < 0) printf("wtf?  _jobCount is %i\n",_jobCount);
   if (_jobCount > 0)
   {
      char temp[100];
      sprintf(temp, "%i network operation%s pending.", _jobCount, (_jobCount > 1) ? "s" : "");
      _busyMessage->SetText(temp);
      _busyMessage->SetViewColor(255,255,100);
   }
   else
   {
      _busyMessage->SetText("Network operations complete.");  
      _busyMessage->SetViewColor(100,255,100);
   }
}

void
SockHopDemoWindow::
PostSockHopMessage(BMessage * msg, const char * optFailMessage)
{
   BMessage whenDone(SD_COMMAND_OPERATION_DONE);
   whenDone.AddString(SH_NAME_TO, "/..");
 
   if (optFailMessage)
   {  
      BMessage onFailure(SD_COMMAND_USER_OPERATION_FAILED);
      onFailure.AddString(SH_NAME_TO, "/..");
      onFailure.AddString(SD_NAME_FAILUREMESSAGE, optFailMessage);
      msg->AddMessage(SH_NAME_ONFAILURE, &onFailure);
   }
   
   msg->AddMessage(SH_NAME_WHENDONE, &whenDone);
   _sockRoot->PostMessage(msg);
   _jobCount++;
   UpdateJobCountDisplay();
}
