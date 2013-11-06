
/**************************************************************************
SockHop (libsockhop.so):  Distributed network programming system for BeOS
Copyright (C) 1999 by Jeremy Friesner (jaf@chem.ucsd.edu)

This library is free software; you can redistribute it and/or 
modify it under the terms of the GNU Library General Public 
License as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later version. 

This library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
Library General Public License for more details. 

You should have received a copy of the GNU Library General Public 
License along with this library; if not, write to the 
Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
Boston, MA  02111-1307, USA. 
**************************************************************************/


#include <stdio.h>
#include <string.h>
#include "shFile.h"
#include <app/Message.h>
#include <storage/Entry.h>
#include <storage/File.h>
#include <storage/Directory.h>
#include <support/TypeConstants.h>
#include <sockhop/SockHopConstants.h>
#include <kernel/fs_attr.h>

#define SH_NAME_DATA  "shData"
#define SH_NAME_ATTRS "shAttrs"

static bool shCarvePath(BDirectory & dir, char * path);

//(dir) is assumed to be valid
//(path) must be relative to (dir) (i.e. no leading '/')
static bool shCarvePath(BDirectory & dir, char * path)
{
   bool ret;
   char * slash = strchr(path, '/');
   
   if (dir.InitCheck() != B_NO_ERROR) 
   {
      printf("shCarvePath:  InitCheck says bad dir!\n");
      return false;
   }
   
   if (slash) 
   {
      // (path) represents a directory
      *slash = '\0';  // temporarily terminate after first part
   
      BEntry subEnt;
      if (dir.FindEntry(path, &subEnt, true) == B_OK)
      {
         struct stat st;
         
         if (subEnt.GetStat(&st) == B_NO_ERROR) 
         {
            if (S_ISDIR(st.st_mode)||(S_ISLNK(st.st_mode)))
            {
               // directory already exists.  Good.  Carry on...
               BDirectory dir(&subEnt);
               ret = shCarvePath(dir, slash+1);
            }
            else
            {
               // It's a file.  We can't handle this (for now)
               ret = false;
            }
         }
         else ret = false;   // Internal error?
      }
      else 
      {
         BDirectory subDir;
         
         // Create a directory and recurse to it.  
         if (dir.CreateDirectory(path, &subDir) == B_NO_ERROR)
         {
            ret = shCarvePath(subDir, slash+1);
         }
         else ret = false;
      }
      
      *slash = '/';
   }
   else
   {
      // end of the line, yay!
      ret = true;
   }   
   return(ret);
}


bool shFileToMessage(const char * fileName, BMessage * msg)
{
//printf("shFileToMessage: loading %s\n",fileName);

   // First, load in main file...
   BFile file(fileName, B_READ_ONLY);
   
   if (file.InitCheck() != B_NO_ERROR) return false;
   off_t mainSize;
   if (file.GetSize(&mainSize) != B_NO_ERROR) return false;

   if (mainSize > 0)
   {   
      char * mainData = new char[mainSize];
      if (file.Read(mainData, mainSize) != mainSize) return false;
   
      bool ret = (msg->AddData(SH_NAME_DATA, B_ANY_TYPE, mainData, mainSize) == B_NO_ERROR);
      delete mainData;
      
      if (ret == false) return false;   
   }
   
   // Now load the attributes
   BMessage attrMsg;
   char buf[B_ATTR_NAME_LENGTH];
   while(file.GetNextAttrName(buf) == B_NO_ERROR)
   {
//      printf("shFileToMessage:  archiving attribute [%s]\n",buf);
      
      struct attr_info attrInfo;
      if (file.GetAttrInfo(buf, &attrInfo) != B_NO_ERROR)
      {
         printf("shFileToMessage:  Error getting attribute info for attribute [%s]!\n",buf);
         return false;
      }
      
      char * attrData = new char[attrInfo.size];
      ssize_t ret = file.ReadAttr(buf, attrInfo.type, 0L, attrData, attrInfo.size);
      if ((ret == attrInfo.size)&&(attrMsg.AddData(buf, attrInfo.type, attrData, attrInfo.size) != B_NO_ERROR)) ret = -1;
      delete attrData;
      if (ret < attrInfo.size)
      {
         printf("shFileToMessage:  Error archiving attribute [%s]\n",buf);
         return false;
      }    
   }
   return(msg->AddMessage(SH_NAME_ATTRS, &attrMsg) == B_NO_ERROR);   
}

bool shMessageToFile(const char * fileName, const BMessage * msg, time_t setTime)
{
//printf("shMessageToFile: saving [%s]\n",fileName);
   { 
      // Carve out any directories that need carving...
	  if (*fileName == '/') 
	  {
	     BDirectory dir("/");
	     char * temp = new char[strlen(fileName+1)+1];
	     strcpy(temp, fileName+1);
	     
	     if (shCarvePath(dir, temp) == false) 
	     {
	        delete [] temp;
	        return false;
	     }
	     delete [] temp;
	  }
	  else
	  {
	     BDirectory dir(".");
	     
	     char * temp = new char[strlen(fileName)+1];
	     strcpy(temp, fileName);
	     if (shCarvePath(dir, temp) == false) 
	     {
	        delete [] temp;
	        return false;    
	     }
	     delete [] temp;
	  }
	  // Open save file
	  BFile file(fileName, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	  if (file.InitCheck() != B_NO_ERROR) return false;

      // Get main data
	  const void * mainData;
	  ssize_t mainDataLen;
	  if (msg->FindData(SH_NAME_DATA, B_ANY_TYPE, 0, &mainData, &mainDataLen) == B_NO_ERROR) 
	  {
	     // Save main data
         if (file.Write(mainData, mainDataLen) < mainDataLen) return false;
      }

      // Get & Save attributes
      BMessage attrMsg;
      if (msg->FindMessage(SH_NAME_ATTRS, &attrMsg) != B_NO_ERROR) return false;

      char * name;
      uint32 type;
      int32 count;
      
      for (int32 i=0; (attrMsg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_NO_ERROR); i++)
      {
         if (count != 1)
         {
            printf("shMessageToFile:  WARNING:  Count wasn't 1, do we have multivalued file attributes now???\n");
         }
         
         const void * attrData;
         ssize_t attrSize;
         if (attrMsg.FindData(name, type, 0, &attrData, &attrSize) != B_NO_ERROR)
         {
            printf("shMessageToFile:  Error finding data for attribute [%s]\n",name);
            return false;
         }
         
         // Write attribute to our file
         if (file.WriteAttr(name, type, 0, attrData, attrSize) != attrSize)
         {
            printf("shMessageToFile:  Error writing attribute [%s] (%li bytes)\n", name, attrSize);
            return false;
         }
      }      
  }
   
   // Now set the file's date...
   BEntry entry(fileName);
   if (entry.Exists() == false) return false;
   if (entry.SetModificationTime(setTime) != B_NO_ERROR) return false;   
   return true;   
}

