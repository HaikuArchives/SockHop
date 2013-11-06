
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
#include <malloc.h>
#include <sockhop/SHWildPathSorter.h>
#include <sockhop/SHStringMatcher.h>

// Returns a pointer into the given string that is past any '.' or './' specifiers
static const char * GoPastInitialCurrentDirectorySpecifiers(const char * startAt);
static const char * GoPastInitialCurrentDirectorySpecifiers(const char * startAt)
{
   while(1)
   {
      if (startAt[0] == '.')
      {
         if (startAt[1] == '\0') return(&startAt[1]);
         if (startAt[1] == '/') startAt += 2;
                           else return startAt;
      }
      else return(startAt);
   }
}


SHWildPathSorter::
SHWildPathSorter()
  : SHSorter(NULL), 
    _matcher(new SHStringMatcher) 
{
   // empty
}

SHWildPathSorter::
SHWildPathSorter(BMessage * archive)
  : SHSorter(archive), _matcher(new SHStringMatcher)
{
   // empty
}

SHWildPathSorter::
~SHWildPathSorter()
{
   delete _matcher;
}

const char *
SHWildPathSorter::
GetName() const
{
   return "wildpath";
}

bool 
SHWildPathSorter::
DoesMessageGoToNode(BMessage & msg, const SHNodeSpec & child, uint32 flags)
{
   bool nodeIsParent = (flags & SH_FLAG_IS_PARENT) != 0;
   int next = 0;
   const char * shTo;
      
   while((shTo = msg.FindString(SH_NAME_TO, next++)) != NULL)
   {   
      // empty shTo means the shTo is indicating this node: therefore, don't forward it anywhere!
      const char * pshTo = GoPastInitialCurrentDirectorySpecifiers(shTo);
      if ((*pshTo != '\0')&&(ParsePath(pshTo, child, nodeIsParent))) return(true);
   }

   // if next == 1, no shTo's were not found, which means we should
   // broadcast to all our children (but not our parent, to avoid loops).
   return((next == 1) ? (nodeIsParent == false) : false);
}

bool 
SHWildPathSorter::
DoesMessageGoToWorker(BMessage & msg, const char * workerName)
{
   int next = 0;
   const char * shToWorker;
      
   while((shToWorker = msg.FindString(SH_NAME_TOWORKERS, next++)) != NULL)
   {   
      if (DoesWildcardMatch(shToWorker, workerName)) return(true);
   }

   // if next == 1, no shToWorkers' were not found, which means we should
   // broadcast to all our workers.
   return(next == 1);
}


bool
SHWildPathSorter::
ParsePath(const char * shTo, const SHNodeSpec & child, bool nodeIsParent)
{
   char tempBuf[SH_MAX_NODENAME_LENGTH];
   bool retVal;
   
   shTo = GoPastInitialCurrentDirectorySpecifiers(shTo);  // remove initial ".", or "./", or "././", or ...
   GetFirstPartOfPath(shTo, tempBuf, sizeof(tempBuf));

   if (*tempBuf != '\0') 
   {
      // We only send to parent if that is *explicitely* requested by the ".." token
      // (we don't let you specify the parent's name, or use wildcarding)
      if (nodeIsParent) return(strcmp(tempBuf, "..") == 0);

      // Other relative path: just check to see if the first path segment matches this child name.
      retVal = DoesWildcardMatch(tempBuf, child.GetNodeName());
   }
   else
   {
      // empty tempBuf means the message has a '/' at the front:  an absolute path!
      bool IAmTheRootNode = (strcmp(GetNodePath(), "/") == 0);
      
      // Is there anything following that leading slash?
      if (shTo[1] != '\0')
      {
         // If so, then one of two rules apply:
         //   - I am the root node, and I should ignore the leading slash
         //   - I'm not the root node, and I should pass the message to my parent only
         retVal = IAmTheRootNode ? ParsePath(&shTo[1], child, nodeIsParent) : nodeIsParent;
      }
      else
      {
         // If nothing follows the leading slash, then forward it to daddy, if we aren't already root
         retVal = IAmTheRootNode ? false : nodeIsParent;
      }
   }
   
   return(retVal);
}
   
  
void 
SHWildPathSorter::
BeforeMessageRelay(BMessage & msg)
{
   // we need to chop off the first section of all shTo's (except if they start with a '/'--
   // if they do, then only chop if we are the root node!)
   int next = 0;
   const char * shTo;
   
   while((shTo = msg.FindString(SH_NAME_TO, next)) != NULL)  // next is incremented at end of loop
   {
      shTo = GoPastInitialCurrentDirectorySpecifiers(shTo);
      
      if (*shTo == '\0')
      {
         // shTo is already empty, so this message was for this node. 
         // Thus we can delete it!
         if (msg.RemoveData(SH_NAME_TO, next) != B_OK)
         {
           printf("Error removing shTo[%i]!\n",next);
         }
      }
      else
      {
         // if shTo starts with a slash, then it is a message on its
         // way to the root node, and the pattern should not be modified
         // until it gets there!
         if (*shTo == '/')
         {
             if (strcmp(GetNodePath(), "/") == 0)  // are we the root node?
             {
                char * temp = strchr(shTo+1, '/');
                char * temp2 = strdup(temp ? (temp+1) : "");
                
                if (temp2)
                {
                    if (msg.ReplaceString(SH_NAME_TO, next, temp2) != B_OK) printf("Error replacing nextPart!\n");
	                free(temp2);
                }
                else printf("Error strduping temp in slash removal!\n");
             }
         }
         else
         {
	         // shTo has at least one section, so we will pass
	         // it to that section.  We need to delete the first
	         // section so that the next node starts at the next "step" in the string.
	         // if the slash 
            char * nextPart = strchr(shTo, '/');
         
            if (nextPart) nextPart++; else nextPart = "";
         
            // replace shTo with nextPart....
            char * temp = strdup(nextPart);  // maybe I can get away without doing this?
            if (temp)
	        {
	           if (msg.ReplaceString(SH_NAME_TO, next, temp) != B_OK) printf("Error replacing nextPart!\n");
	           free(temp);
	        }
	        else printf("Error strduping nextPart!\n");
         }
	     next++;
      }
   }   
}

// put just the first section of (from) into (to)...
void
SHWildPathSorter::
GetFirstPartOfPath(const char * from, char * to, int toSize) const
{
   char * firstSlash = strchr(from, '/');
   if ((firstSlash)&&((firstSlash - from) < toSize)) 
   {
      char * t = to;
      const char * f = from;
      
      while(f < firstSlash) *(t++) = *(f++);
      *t = '\0';
   }
   else 
   {
      strncpy(to, from, toSize);
      to[toSize-1] = '\0';
   }
}

    
bool
SHWildPathSorter::
DoesMessageDistributeLocally(BMessage & msg)
{
   // check the first part of the pathstrings to see if we match!
   int next = 0;
   const char * shTo;
   const char * myPath = GetNodePath();
   
   // If we have any empty shTo's, that means a descriptor "ran out" of text on
   // the last delivery, and so it must be for us!
   while((shTo = msg.FindString(SH_NAME_TO, next++)) != NULL)
   {
      shTo = GoPastInitialCurrentDirectorySpecifiers(shTo);
      if (shTo[0] == '\0') return true;
      
      // Check for special case:  We are '/', message is for '/'
      if ((shTo[0] == '/')&&(myPath[0] == '/')&&
          (shTo[1] == '\0')&&(myPath[1] == '\0')) return true;    
   }
   
   // if next == 1, no shTo's were not found, and it's a broadcast message.
   return(next == 1);
}

bool
SHWildPathSorter::
DoesWildcardMatch(const char * wildcard, const char * matchMe)
{
   if (_matcher)
   {
       // If wildcard is NULL, then we'll just use the previous setting...
       bool setRes = wildcard ? _matcher->SetSimpleExpression(wildcard) : true;
	   if (setRes)
	   {
	      bool matchRes = _matcher->Match(matchMe);
	      return matchRes;
	   }
	   else 
	   {
	      printf("Couldn't compile regular expression!\n");
	      return false;
	   }
	}
	else 
	{
	    // If regexp code isn't compiled in, then just match on exact matches.
        return(strcmp(wildcard, matchMe) == 0);
	}
}


BArchivable *
SHWildPathSorter::
Instantiate(BMessage * archive)
{
   if (!validate_instantiation(archive, "SHWildPathSorter")) return NULL;
   return new SHWildPathSorter(archive);
}

status_t
SHWildPathSorter::
Archive(BMessage * archive, bool deep) const
{
   status_t ret = SHSorter::Archive(archive, deep);
   if (ret != B_NO_ERROR) return ret;
   
   archive->AddString("class",  class_name(this));  // Because we can be instantiated
   return B_OK; 
}

/* FBC */
void SHWildPathSorter::_SHWildPathSorter1() {}
void SHWildPathSorter::_SHWildPathSorter2() {}
void SHWildPathSorter::_SHWildPathSorter3() {}
void SHWildPathSorter::_SHWildPathSorter4() {}
