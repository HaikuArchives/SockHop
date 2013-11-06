
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


#ifndef _SOCKHOP_H_
#define _SOCKHOP_H_

/////////////////////////////////////////////////
//
// SockHop.h
//
// This is a "wrapper" include file that pulls in
// all the SockHop include files for you.  This
// is the only SockHop file you should ever need to
// #include from your code.
//
/////////////////////////////////////////////////

#include <sockhop/SockHopConstants.h>
#include <sockhop/SockHopFunctions.h>
#include <sockhop/SHFileSpec.h>
#include <sockhop/SHNodeSpec.h>
#include <sockhop/SHSorter.h>
#include <sockhop/SHWorker.h>
#include <sockhop/SHDirectConnection.h>
#include <sockhop/SHSessionAcceptor.h>
#include <sockhop/SHDefaultAccessPolicy.h>
#include <sockhop/SHStringMatcher.h>
#include <sockhop/SHFlavor.h>

#endif