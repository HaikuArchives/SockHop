// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _SOCKHOPDEMOWINDOW_H_
#define _SOCKHOPDEMOWINDOW_H_

#include <interface/Window.h>
#include <interface/OutlineListView.h>

#include "NodeSpecView.h"
#include "NodeTreeItem.h"

class SockHopDemoWindow : public BWindow 
{
public:
   SockHopDemoWindow();
   ~SockHopDemoWindow();

   virtual void MessageReceived(BMessage * msg);
   
private:
   BMessage * MakeTempoMessage(int32 tempo);
   BBox * MakeBBox(BRect & rect, const char * label);
   char * GenUniqueName(NodeTreeItem * parent, const char * origName);
   void PostSockHopMessage(BMessage * msg, const char * optFailMessage = NULL);
   void UpdateJobCountDisplay();
   void AddHighlightedNodesAddresses(BMessage * add);
   void UpdateSelectedItem();
   void ResetNodeTree();
   bool SaveTreeTo(const NodeTreeItem * subTree, BEntry * file) const;
   bool LoadTreeFrom(const NodeTreeItem * insertUnder, BEntry * file);
   void PostTreeLoadMessages(const BMessage * readFrom, char * nodePath, const int nodePathLen);
   void UserMessage(const char * msg);
   
   BOutlineListView * _treeView;
   BBox * _treeViewBox;
   BListView * _workerView;
   NodeSpecView * _nodeSpecView;
   NodeTreeItem * _nodeRoot;
   NodeTreeItem * _selectedNode;
   BButton * _deleteNodesButton;
   BButton * _deleteWorkersButton;
   BLooper * _sockRoot;
   BStringView * _busyMessage;

   bool _lastOpenWasInsert;
   BFilePanel * _savePanel;
   BFilePanel * _openPanel;
   BEntry _lastSaved;
   BMenuItem * _saveMenuItem;
   int _jobCount;
   int32 _tempo;
   
   // statistics tracking
   uint32 _opsPerSecond[10];
   int _currentOpCounter;
   thread_id _pulseThread;
   BLocker _pulseLock;
   bool _opsActive;
   static long PulseLoop(void * data);  // thread function
};

#endif
