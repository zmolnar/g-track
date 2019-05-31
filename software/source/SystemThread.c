/**
 * @file SystemThread.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "SystemThread.h"
#include "GpsReaderThread.h"
#include "Dashboard.h"
#include "ChainOilerThread.h"
#include "sim8xx.h"
#include <string.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/
typedef enum {
  SYSTEM_INIT,
  SYSTEM_PARKING,
  SYSTEM_RIDING,
  SYSTEM_TRACKING,
  SYSTEM_ERROR
} SystemState_t;

typedef enum { 
  SYS_EVT_IGNITION_ON, 
  SYS_EVT_IGNITION_OFF 
} SystemEvent_t;

typedef enum {
  SYS_E_NO_ERROR
} SystemError_t;

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static msg_t events[10];
static mailbox_t systemMailbox;
SystemState_t systemState;
SystemError_t systemError = SYS_E_NO_ERROR;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS */
/*******************************************************************************/
static void connectModem(void) {
  while(!sim8xxIsConnected(&SIM8D1)) {
    sim8xxTogglePower(&SIM8D1);
  }
}

static void disconnectModem(void) {
  while(sim8xxIsConnected(&SIM8D1)) {
    sim8xxTogglePower(&SIM8D1);
  }
}

static SystemState_t systemInitHandler(SystemEvent_t evt) {
  SystemState_t newState = SYSTEM_INIT;
  switch(evt) {
    case SYS_EVT_IGNITION_ON: {
      connectModem();
      GpsReaderStart();
      ChainOilerStart();
      newState = SYSTEM_RIDING;
      break;
    }
    case SYS_EVT_IGNITION_OFF: {
      ChainOilerStop();
      GpsReaderStop();
      disconnectModem();
      newState = SYSTEM_PARKING;
      break;
    }
    default: {
      ;
    }
  }
  return newState;
}

static SystemState_t systemParkingHandler(SystemEvent_t evt) {
  SystemState_t newState = SYSTEM_PARKING;
  switch(evt) {
    case SYS_EVT_IGNITION_ON: {
      connectModem();
      GpsReaderStart();
      ChainOilerStart();
      newState = SYSTEM_RIDING;
      break;
    }
    case SYS_EVT_IGNITION_OFF: {
      break;
    }
    default: {
      ;
    }
  }
  return newState;
}

static SystemState_t systemRidingHandler(SystemEvent_t evt) {
  SystemState_t newState = SYSTEM_RIDING;
  switch(evt) {
    case SYS_EVT_IGNITION_ON: {
      break;
    }
    case SYS_EVT_IGNITION_OFF: {
      ChainOilerStop();
      GpsReaderStop();
      disconnectModem();
      newState = SYSTEM_PARKING;
      break;
    }
    default: {
      ;
    }
  }
  return newState;
}

static SystemState_t systemTrackingHandler(SystemEvent_t evt) {
  (void)evt;
  return SYSTEM_TRACKING;
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(SystemThread, arg) {
  (void)arg;
  chRegSetThreadName("system");

  systemState = SYSTEM_INIT;

  while(true) {
    SystemEvent_t evt;
    if (MSG_OK == chMBFetchTimeout(&systemMailbox, (msg_t*)&evt, TIME_INFINITE)) {
      switch(systemState) {
        case SYSTEM_INIT: {
          systemState = systemInitHandler(evt);
          break;
        }
        case SYSTEM_PARKING: {
          systemState = systemParkingHandler(evt);
          break;
        }
        case SYSTEM_RIDING: {
          systemState = systemRidingHandler(evt);
          break;
        }
        case SYSTEM_TRACKING: {
          systemState = systemTrackingHandler(evt);
          break;
        }
        default: {
          ;
        }
      }
    }
  }
} 

void SystemThreadInit(void) {
    dbInit();
    memset(&events, 0, sizeof(events));
    chMBObjectInit(&systemMailbox, events, sizeof(events)/sizeof(events[0]));
}

void SystemThreadIgnitionOn(void) {
  chSysLock();
  chMBPostI(&systemMailbox, SYS_EVT_IGNITION_ON);
  chSysUnlock();
}

void SystemThreadIgnitionOff(void) {
  chSysLock();
  chMBPostI(&systemMailbox, SYS_EVT_IGNITION_OFF);
  chSysUnlock();
}

const char* SystemThreadGetStateStr(void) {
  static const char* const stateStr[] = {
    [SYSTEM_INIT]     = "INIT",
    [SYSTEM_PARKING]  = "PARKING",
    [SYSTEM_RIDING]   = "RIDING",
    [SYSTEM_TRACKING] = "TRACKING",
    [SYSTEM_ERROR]    = "ERROR"
  };

  return stateStr[(size_t)systemState];
}

const char* SystemThreadGetErrorStr(void) {
  static const char* const errorStr[] = {
    [SYS_E_NO_ERROR] = "NO_ERROR",
  };

  return errorStr[(size_t)systemError];
}

/******************************* END OF FILE ***********************************/
