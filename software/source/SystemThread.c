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
  SYSTEM_TRACKING
} SystemState_t;

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static msg_t events[10];
mailbox_t systemMailbox;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
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
      newState = SYSTEM_RIDING;
      break;
    }
    case SYS_EVT_IGNITION_OFF: {
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
  (void)evt;
  return SYSTEM_PARKING;
}

static SystemState_t systemRidingHandler(SystemEvent_t evt) {
  (void)evt;
  return SYSTEM_RIDING;
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

  static SystemState_t state = SYSTEM_INIT;

  while(true) {
    SystemEvent_t evt;
    if (MSG_OK == chMBFetchTimeout(&systemMailbox, (msg_t*)&evt, TIME_INFINITE)) {
      switch(state) {
        case SYSTEM_INIT: {
          state = systemInitHandler(evt);
          break;
        }
        case SYSTEM_PARKING: {
          state = systemParkingHandler(evt);
          break;
        }
        case SYSTEM_RIDING: {
          state = systemRidingHandler(evt);
          break;
        }
        case SYSTEM_TRACKING: {
          state = systemTrackingHandler(evt);
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

/******************************* END OF FILE ***********************************/
