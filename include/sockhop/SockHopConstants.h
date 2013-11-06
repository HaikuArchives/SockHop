
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


#ifndef _SOCKHOPCONSTANTS_H_
#define _SOCKHOPCONSTANTS_H_

////////////////////////////////////////////////////////
//
//  SockHopConstants.h
//
//  This file contains definitions for all the
//  numeric and string constants that are understood
//  by SockHop.
//
/////////////////////////////////////////////////////////

// SockHop assigns special meaning to BMessages it receives
// that have one of the following codes in their 'what' field.
// All other type codes are assumed to be user messages and will 
// be handed to SHWorkers for user-level processing.
enum {
  SH_COMMAND_BASE = 'SoHo',     // (Place holder, no special meaning)

  // These 'what' values are appropriate for passing to SockHop from user code.
  SH_COMMAND_ADDCOMPONENTS,     // "Add child nodes, SHWorkers, SHSorters, files, or symlinks"
  SH_COMMAND_REMOVECOMPONENTS,  // "Remove child nodes, SHWorkers, SHSorters, files, or symlinks"
  SH_COMMAND_SETPARAMETERS,     // "Set these misc. parameters"
  SH_COMMAND_GETPARAMETERS,     // "Retrieve current values of these misc. parameters"
  SH_COMMAND_QUIT,              // "Destroy yourself (and the subtree beneath your node)"
  SH_COMMAND_LARGEST,           // marks the end of the current range of command values

  SH_COMMAND_MAXIMUM         = SH_COMMAND_BASE + 29, // Largest possible SH_COMMAND_ value allowed (ever!)

  // These 'what' values may be passed by SockHop back to user code.
  SH_CODE_CONNECTIONOPEN     = SH_COMMAND_BASE + 30, // (Sent when SHSessionAcceptor or SHDirectConnection opens a new connection)
  SH_CODE_CONNECTIONCLOSED,                          // (Sent when SHSessionAcceptor or SHDirectConnection detects a closed connection)

  SH_ENCODING_NONE           = SH_COMMAND_BASE + 40, // (Default message encoding method:  BMessages are sent uncompressed, one-by-one, over the link)
  SH_ENCODING_BATCH,                                 // (Alternate method:  BMessages are placed into a single "batch" BMessage before transmission.  Of dubious value!)
  SH_ENCODING_ZLIBLIGHT,                             // (Third method:  BMessages are "batched" and compressed with zlib level 1 encoding
  SH_ENCODING_ZLIBMEDIUM,                            // (Third method:  BMessages are "batched" and compressed with zlib level 6 encoding
  SH_ENCODING_ZLIBHEAVY,                             // (Third method:  BMessages are "batched" and compressed with zlib level 9 encoding

  // These 'what' values are used by internally by SockHop.  User code should never send
  // a BMessage with any of the values in this range to SockHop, and SockHop will
  // never send a BMessage with one of these values back to user code!
  SH_INTERNAL_COMMANDS_BEGIN = SH_COMMAND_BASE + 50, // Begins range of command values reserved for SockHop's internal use
  SH_INTERNAL_COMMANDS_END   = SH_COMMAND_BASE + 100 // End range of reserved command values.
};

// Port number that SockHop will TCP-connect to by default.
#define SH_DEFAULT_PORT      2958

// Maximum number of bytes (including the NUL byte) 
// that may be in a node name string (the full path may be longer, though)
#define SH_MAX_NODENAME_LENGTH  512

// The maximum size allowed for the SHNodeSpecs that initiate new connections.  Currently 20KBytes.
#define SH_MAX_INITSPEC_SIZE (20*1024)

// Typecodes used to identify flattened SHNodeSpecs, SHFileSpecs, and SHFlavors, respectively
#define SH_NODESPEC_TYPECODE 'SoHn'
#define SH_FILESPEC_TYPECODE 'SoHf'
#define SH_FLAVOR_TYPECODE   'SoHv'

// Flavor bitchords; Used by SHFileSpec to identify various supported hardware architectures
#define SH_ARCH_BEOS_PPC 0x000001  // BeOS on PowerPC-based machines
#define SH_ARCH_BEOS_X86 0x000002  // BeOS on x86-based machines
// Watch this space!
#define SH_ARCH_ANY      0xFFFFFF  // indicates all types of SockHop-running machine

// SHSorter bitchords;  These are the flags that SockHop may pass into the (flags)
// argument of SHSorter subclass's DoesMessageGoTo() method.
#define SH_FLAG_IS_PARENT  0x0000001  // The target is our parent node
#define SH_FLAG_IS_LOCAL   0x0000002  // The target is running in our address space
#define SH_FLAG_IS_SYMLINK 0x0000004  // The target is actually a symbolic link, not really our own child

// These are BMessage field names that SockHop will parse out of the BMessages you send to it.
// In your code, you should always use the SH_NAME_* codes, instead of explicitely using the strings
// they represent.  This will save you from the horrors of typos, and will make upgrading your code
// easier should the names change.  
//
// Note that some of these fields are only relevant for certain SH_COMMAND_* codes, and that all of 
// these fields are optional (if they aren't specified, a reasonable default behavior will occur)
//
#define SH_NAME_WHICHSORTER "shWhichSorter" // For any BMessage type:  String field, specifies by name which SHSorter to use.
#define SH_NAME_SORTERS     "shSorters"     // For SH_COMMAND_ADDCOMPONENTS:  Array of BMessages representing archived SHSorters.  For SH_COMMAND_REMOVECOMPONENTS:  Array of regexp strings specifying SHSorter(s) to remove.
#define SH_NAME_WORKERS     "shWorkers"     // For SH_COMMAND_ADDCOMPONENTS:  Array of BMessages representing archived SHWorkers.  For SH_COMMAND_REMOVECOMPONENTS:  Array of regexp strings specifying SHWorker(s) to remove.
#define SH_NAME_CHILDREN    "shChildren"    // For SH_COMMAND_ADDCOMPONENTS:  Array of SHNodeSpecs representing nodes to create and add as children of the receiving node.  For SH_COMMAND_REMOVECOMPONENTS:  Array of regexp strings specifying which children or symlinks to remove.
#define SH_NAME_SYMLINKS    "shSymLinks"    // For SH_COMMAND_ADDCOMPONENTS:  Array of wildpath strings telling which nodes to symlink to.  For SH_COMMAND_REMOVECOMPONENTS:  Array of regexp strings specifying symlink names to remove.
#define SH_NAME_FILES       "shFiles"       // For SH_COMMAND_ADDCOMPONENTS:  Array of SHFileSpecs representing files to cache onto the receiving node.  For SH_COMMMAND_REMOVECOMPONENTS:  Array SHFileSpecs representing files to delete from the receiving nodes.
#define SH_NAME_ONSUCCESS   "shOnSuccess"   // For any BMessage type:  Array of BMessages to be posted by the receiving node(s) on successful completion of this operation at that node.
#define SH_NAME_ONFAILURE   "shOnFailure"   // For any BMessage type:  Array of BMessages to be posted by the receiving node(s) on failure of this operation at that node.
#define SH_NAME_WHENDONE    "shWhenDone"    // For any BMessage type:  Array of BMessages to be posted by the originating node, when operation has completed across the entire tree.

#define SH_NAME_TO          "shTo"          // Array of strings, used by SHWildPathSorter to determine which nodes message goes to.
#define SH_NAME_TOWORKERS   "shToWorkers"   // Array of strings, used by SHWildPathSorter to determine which workers message goes to. 

// These are BMessage field names that SockHop may add to BMessages that it sends back to you.
#define SH_NAME_REGARDING         "shRegarding"  // For many BMessage types:  String, SHFileSpec, or SHNodeSpec, added by SockHop to OnSuccess and OnFailure messages.  Indicates the object that the BMessage is related to.
#define SH_NAME_FROM              "shFrom"       // Node Pathname string added to OnSuccess and OnFailure messages, indicating which node the return message came from.
#define SH_NAME_CONNECTIONID      "shCID"        // In BMessages that came from SHDirectConnection objects, this field contains an int32 with the ID number of the originating SHDirectConnection.

// This field name is added to BMessages that you archive SHDistributableObjects
// into.  Usually you don't have to worry about it, but it is documented here because
// in special cases it might be useful to user code to know the contents of this field.
#define SH_NAME_ADDONSPEC         "shAddOnSpec"  // SHFileSpec that indicates where to find the add-on binary.

// Parameter field names that SockHop nodes will respond to inside of SH_COMMAND_SETPARAMETER or SH_COMMAND_GETPARAMETER messages
#define SH_PARAMNAME_DEBUG                "shDebug"                 // Data is an Int32, indicating debug level.  Currently 0 means debug printing off, >0 means debug printing on
#define SH_PARAMNAME_DEFAULTSORTER        "shDefaultSorter"         // Data is a String, specifies default sorter name
#define SH_PARAMNAME_THREADPRIORITY       "shThreadPriority"        // Data is an Int32, indicating priority that all subsequently spawned threads should be given
#define SH_PARAMNAME_TRANSMISSIONENCODING "shTransmissionEncoding"  // Data should be one of the SH_ENCODING_* tokens, indicating message encoding to be used by all subsequently spawned messaging threads

// Note that there are other BMessage field names that SockHop uses internally.
// These names all start with "sh" and then a capital letter.  To avoid conflicts, 
// it is best to avoid similar field names in user messages that are sent via SockHop.

#endif
