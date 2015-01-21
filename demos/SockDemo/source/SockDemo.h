// This file, and all other source files that make up
// SockDemo, are placed into the public domain.  - Jeremy Friesner
 
#ifndef _SOCKDEMO_H_
#define _SOCKDEMO_H_

#define SD_TEXT_HEIGHT       20
#define SD_VIEW_SPACING       8
#define SD_COMPONENT_SPACING  4

// This BMessage should be posted back to us (i.e., to "/..") by every Worker
// we instantiate, when it quits.  That's so we'll know to take
// it out of the appropriate worker list!
// The BMessage should have the following info in it:
//    SH_NAME_FROM  : string, node path of the node the worker was on
//    SD_NAME_WORKERNAME : string, GetName() of the quitting worker
//
#define SD_COMMAND_WORKER_QUITTING 'sdwq'
#define SD_NAME_WORKERNAME "sdWorkerName"

// Sent to the workers in an SD_COMMAND_TEMPO message.  Their new tempo (1-5)
#define SD_NAME_TEMPO "sdTempo"

// May be sent back to us by workers;  Should contain the following field:
//    SD_NAME_STAT : int32, number of ops/sec the worker is doing.
#define SD_COMMAND_WORKER_STATS    'sdws'
#define SD_NAME_STAT "sdStat"

// This is sent by the SockDemo app when the user double-clicks a worker name.
// It just tells the worker to send its window to the front.
#define SD_COMMAND_WINDOW_TO_FRONT 'sdwf'

enum {
  SD_COMMAND_BASE = 'sDmo',
  SD_COMMAND_ADD_NODE,
  SD_COMMAND_DELETE_NODES,
  SD_COMMAND_DELETE_WORKERS,
  SD_COMMAND_NODETREE_SELECT,
  SD_COMMAND_WORKERLIST_SELECT,
  SD_COMMAND_NODE_ADD_SUCCEEDED,
  SD_COMMAND_NODE_ADD_FAILED,
  SD_COMMAND_WORKER_ADD_SUCCEEDED,
  SD_COMMAND_WORKER_ADD_FAILED,
  SD_COMMAND_OPERATION_DONE,
  SD_COMMAND_UPDATE_OPS_PER_SECOND,
  SD_COMMAND_USER_OPERATION_FAILED,
  
  // menu item codes
  SD_COMMAND_ABOUT_SOCKHOPDEMO,
  SD_COMMAND_OPEN,
  SD_COMMAND_INSERT,
  SD_COMMAND_CLOSE,
  SD_COMMAND_SAVE,
  SD_COMMAND_SAVE_AS,
  SD_COMMAND_ABOUT_SOCKHOP,
  SD_COMMAND_SETTING_UP,
  SD_COMMAND_USING_GUI,
  SD_COMMAND_ADD_ONS,
  SD_COMMAND_TEMPO
};

// This field contains an error message to display whenever
// a command fails.
#define SD_NAME_FAILUREMESSAGE "sdFailureMessage"

// These are used in the saving and opening of node subtree files.
#define SD_NAME_NODESPEC "sdNS"
#define SD_NAME_CHILD    "sdCh"

#include <sockhop/SockHop.h>

#endif
