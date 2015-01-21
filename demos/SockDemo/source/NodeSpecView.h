// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _NODESPECVIEW_H_
#define _NODESPECVIEW_H_

#include <interface/View.h>
#include <sockhop/SockHop.h>

class NodeSpecView : public BView
{
public:
   NodeSpecView(BRect frame);
   ~NodeSpecView();

   SHNodeSpec GetDisplayedSpec() const;
   
   // (Sending in NULL clears the display)
   void setDisplayedSpec(const SHNodeSpec * optNodeSpec);

   void setAddNodeEnabled(bool enable);
      
private:
   BTextControl * _nodeNameText;
   BTextControl * _hostNameText;
   BTextControl * _portText;
   BTextControl * _passwordText;
   BButton * _addButton;
};


#endif
