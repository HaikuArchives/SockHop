// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner

#ifndef _NODETREEITEM_H_
#define _NODETREEITEM_H_

#include <interface/ListItem.h>
#include <sockhop/SockHop.h>

class NodeTreeItem : public BStringItem
{
public:
    NodeTreeItem(const SHNodeSpec & spec);
    ~NodeTreeItem();
    
    SHNodeSpec GetNodeSpec() const;
    
    void AddChild(NodeTreeItem * child);
    bool RemoveChild(NodeTreeItem * child);
    
    // Returns child on success, null on failure.
    NodeTreeItem * GetChildNamed(const char * name);
    
    // Returns child on success, null on failure.
    NodeTreeItem * GetChildBySpec(const SHNodeSpec & spec);
    
    // Returns node on success, null on failure.
    NodeTreeItem * GetNodeByPath(const char * path);
    
    // Returns the pathname of this node.  You
    // must delete[] the string when you are done with it!
    char * GetPath(BOutlineListView * view) const;
    
    // Adds a new worker name to our list
    // Returns a pointer to the BStringItem that we added to our internal list.
    BStringItem * AddWorker(const char * name);
    
    // Removes and frees the given BStringItem from our internal list
    // Returns true iff the BStringItem was found in the list.
    bool RemoveWorker(BStringItem * workerItem);
    
    // Returns our list of BStringItems
    BList * GetWorkerList();
    
    // Returns true if (item) is a child (or granchild, or ...)
    // of this node.
    bool IsAncestorOf(NodeTreeItem * item) const;
    
    // Saves this node and all its descendants into (msg).
    void SaveTreeToMessage(BMessage * msg) const;
    
private:
    NodeTreeItem * GetNodeByPathAux(const char * path) const;
    
    SHNodeSpec _spec;
    BList _children;  // (other NodeTreeItem)'s
    BList _workers;   // (BStringItem *)'s of worker names
};

#endif
