// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner

#include <stdio.h>
#include <stdlib.h>

#include "SockDemo.h"
#include "NodeSpecView.h"

#include <interface/TextControl.h>
#include <interface/Button.h>

NodeSpecView::
NodeSpecView(BRect frame)
  : BView(frame, "Node Spec", B_FOLLOW_RIGHT | B_FOLLOW_TOP, 0)
{
   BRect pos(0, SD_VIEW_SPACING, (frame.right - frame.left) - SD_VIEW_SPACING, SD_TEXT_HEIGHT + SD_VIEW_SPACING);
   
   AddChild(_nodeNameText = new BTextControl(pos, "NodeName", "Node Name", "", NULL));
   pos.top += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;  pos.bottom += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;
   AddChild(_hostNameText = new BTextControl(pos, "HostName", "Host Name", "", NULL));
   pos.top += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;  pos.bottom += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;
   AddChild(_portText = new BTextControl(pos, "Port", "Port", "", NULL));
   pos.top += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;  pos.bottom += SD_TEXT_HEIGHT + SD_COMPONENT_SPACING;
   AddChild(_passwordText = new BTextControl(pos, "Password", "Password", "", NULL));
   pos.top += SD_TEXT_HEIGHT + SD_VIEW_SPACING;  pos.bottom += SD_TEXT_HEIGHT + SD_VIEW_SPACING;

   AddChild(_addButton = new BButton(pos, "Add Node", "Add Node", new BMessage(SD_COMMAND_ADD_NODE)));   
   setAddNodeEnabled(false);
}

NodeSpecView::
~NodeSpecView()
{
   // empty
}
 
SHNodeSpec
NodeSpecView::
GetDisplayedSpec() const
{
   const char * nodeName = _nodeNameText->Text();
   const char * hostName = _hostNameText->Text();
   int port = atoi(_portText->Text());
   const char * password = _passwordText->Text();
   
   if (*nodeName == '\0') nodeName = "NewNode";

   BMessage passwordMsg;
   passwordMsg.AddString("password", password);
   
   if (*hostName == '\0') return(SHNodeSpec(nodeName));
                     else return(SHNodeSpec(nodeName, hostName, port, &passwordMsg));
}
  

void
NodeSpecView::
setDisplayedSpec(const SHNodeSpec * optSpec)
{
   SHNodeSpec defaultSpec;
   if (optSpec == NULL) optSpec = &defaultSpec;
   
   _nodeNameText->SetText(optSpec->GetNodeName());
   _hostNameText->SetText(optSpec->GetHostName());
   
   const char * pwd = "";
   (void)optSpec->GetConstSpecMessage().FindString("password", &pwd); 
   _passwordText->SetText(pwd);
   
   char temp[50];
   int port = optSpec->GetPortNumber();
   sprintf(temp, "%i", port ? port : SH_DEFAULT_PORT);
   
   _portText->SetText(temp);
}
  
void
NodeSpecView::
setAddNodeEnabled(bool enable)
{
   if (enable)
   {
      _addButton->SetLabel("Add Node to Tree");
      _addButton->SetEnabled(true);
   }
   else
   {
      _addButton->SetLabel("(Select Parent Node First)");
      _addButton->SetEnabled(false);
   }
}
