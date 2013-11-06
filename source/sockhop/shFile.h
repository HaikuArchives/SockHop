
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


#ifndef _SHFILE_H
#define _SHFILE_H

#include <support/Flattenable.h>

// Adds the file into the given BMessage.  Returns B_NO_ERROR iff successful.
// Only one file (and nothing else) should be put in any one BMessage.
bool shFileToMessage(const char * fileName, BMessage * msg);

// Saves a file to the given fileName, from the given BMessage which was
// prepared with shFileToMessage.  Returns B_NO_ERROR on success.
bool shMessageToFile(const char * fileName, const BMessage * msg, time_t setDate);

#endif
