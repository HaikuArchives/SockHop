// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#include <interface/ListItem.h>
#include <interface/OutlineListView.h>

#include "NodeTreeItem.h"
#include "SockDemo.h"

#include <stdio.h>
#include <string.h>


NodeTreeItem::
NodeTreeItem(const SHNodeSpec & spec)
   : BStringItem(spec.GetNodeName()), _spec(spec)
{ 
   // empty
}

NodeTreeItem::
~NodeTreeItem()
{
   int num = _children.CountItems();
   for (int i=0; i<num; i++) delete ((NodeTreeItem *)_children.ItemAt(i));
 
   int numWorkers = _workers.CountItems();
   for (int j=0; j<numWorkers; j++) delete ((BStringItem *)_workers.ItemAt(j));
}

SHNodeSpec
NodeTreeItem::
GetNodeSpec() const
{
   return _spec;
}

void
NodeTreeItem::
AddChild(NodeTreeItem * child)
{
   _children.AddItem(child);
}

bool
NodeTreeItem::
RemoveChild(NodeTreeItem * child)
{
   return _children.RemoveItem(child);
}

BStringItem *
NodeTreeItem::
AddWorker(const char * name)
{
   BStringItem * ret = new BStringItem(name);
   _workers.AddItem(ret);
   return ret;
}

bool
NodeTreeItem::
RemoveWorker(BStringItem * item)
{
   if (_workers.RemoveItem(item)) 
   {
      delete item;
      return true;
   }
   else 
   {
      printf("NodeTreeItem:  warning, item [%s] not found in our list!\n",item->Text());   
      return false;
   }
}

NodeTreeItem *
NodeTreeItem::
GetNodeByPath(const char * path)
{
   // special case:  We are root node, they want the root node.
   const char * myNodeName = _spec.GetNodeName();
   if ((myNodeName[0] == '/')&&(path[0] == '/')&&
       (myNodeName[1] == '\0')&&(path[1] == '\0')) return this;
       
   if (*path == '/') path++;
   return(GetNodeByPathAux(path));
}

NodeTreeItem *
NodeTreeItem::
GetNodeByPathAux(const char * path) const
{
   bool lastBit = false;
   int numKids = _children.CountItems();
   
   if (numKids == 0) return NULL;
   
   char * nextSlash = strchr(path, '/');
   if (nextSlash == NULL) 
   {
      nextSlash = strchr(path, '\0');
      lastBit = true;
   }
   
   int len = nextSlash - path;
   if (len == 0) return NULL;
   
   NodeTreeItem * nextNode = NULL;
   for (int i=0; i<numKids; i++)
   {
      NodeTreeItem * next = (NodeTreeItem *)_children.ItemAt(i);
      if (strncmp(next->_spec.GetNodeName(), path, len) == 0)
      {
         nextNode = next;
         break;
      }
   }

   if (nextNode) return(lastBit ? nextNode : nextNode->GetNodeByPathAux(nextSlash+1));   
            else return NULL;
}

NodeTreeItem *
NodeTreeItem::
GetChildNamed(const char * childName)
{
   int numKids = _children.CountItems();
   for (int i=0; i<numKids; i++)
   {
      NodeTreeItem * next = (NodeTreeItem *)_children.ItemAt(i);
      if (strcmp(next->_spec.GetNodeName(), childName) == 0) return next;
   }
   return NULL;
}


NodeTreeItem *
NodeTreeItem::
GetChildBySpec(const SHNodeSpec & spec)
{
   int numKids = _children.CountItems();
   for (int i=0; i<numKids; i++)
   {
      NodeTreeItem * next = (NodeTreeItem *)_children.ItemAt(i);
      if (spec == next->_spec) return next;
   }
   return NULL;
}

char *
NodeTreeItem::
GetPath(BOutlineListView * view) const
{
   // Do a first pass up to the root node to figure out how much space we'll need...
   int byteCount = 0;
   const NodeTreeItem * next = this;
   
   while(next)
   {
      const char * nodeName = next->_spec.GetNodeName();
      byteCount += strlen(nodeName);
      if (*nodeName != '/') byteCount++;
      next = (NodeTreeItem *) view->Superitem(next);
   }

   // special case:  root node only!
   if (byteCount == 1) 
   {
      char * ret = new char[2];
      strcpy(ret, "/");
      return ret;
   }
   
   char * ret = new char[byteCount];
   char * last = ret + byteCount - 1;  // last byte in temp
   
   next = this;
   bool first = true;
   while(next)
   {
      const char * nodeName = next->_spec.GetNodeName();
      if (*nodeName != '/')
      {
         int nodeNameLen = strlen(nodeName);
         last -= nodeNameLen + 1;
         *last = '/';
         if (first) 
         {
            strcpy(last+1, nodeName);
            first = false;
         }
         else memcpy(last+1, nodeName, nodeNameLen);
      }
      next = (NodeTreeItem *) view->Superitem(next);
   }
   
   return ret;
}


bool
NodeTreeItem ::
IsAncestorOf(NodeTreeItem * item) const
{
    if (item == this) return true;
    
    int num = _children.CountItems();
    for (int i=0; i<num; i++)
       if (((NodeTreeItem *)_children.ItemAt(i))->IsAncestorOf(item)) return true;
    
    return false;  
}

BList *
NodeTreeItem ::
GetWorkerList()
{
    return &_workers;
}


void
NodeTreeItem::
SaveTreeToMessage(BMessage * msg) const
{
   msg->what = SD_COMMAND_SAVE;
   
   // First save myself.
   msg->AddFlat(SD_NAME_NODESPEC, (BFlattenable *)&_spec);
   
   // Now put my children.
   int num = _children.CountItems();
   for (int i=0; i<num; i++)
   {
      BMessage childMsg(SD_COMMAND_SAVE);
      ((NodeTreeItem *)_children.ItemAt(i))->SaveTreeToMessage(&childMsg);
      msg->AddMessage(SD_NAME_CHILD, &childMsg);
   }
}
