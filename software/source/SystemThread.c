/**
 * @file SystemThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "SystemThread.h"

#include "ChainOilerThread.h"
#include "Dashboard.h"
#include "GpsReaderThread.h"
#include "sim8xx.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define MAX_TRIES 3

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  SYSTEM_INIT,
  SYSTEM_PARKING,
  SYSTEM_RIDING,
  SYSTEM_TRACKING,
  SYSTEM_ERROR,
} SystemState_t;

typedef enum { SYS_EVT_IGNITION_ON, SYS_EVT_IGNITION_OFF } SystemEvent_t;

typedef enum {
  SYS_E_NO_ERROR,
  SYS_E_MODEM_POWER_ON,
  SYS_E_MODEM_POWER_OFF,
} SystemError_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static msg_t events[10];
static mailbox_t systemMailbox;
SystemState_t systemState = SYSTEM_INIT;
SystemError_t systemError = SYS_E_NO_ERROR;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static bool connectModem(void)
{
  uint32_t i = 0;
  while ((i++ < MAX_TRIES) && (!sim8xxIsConnected(&SIM8D1))) {
    sim8xxTogglePower(&SIM8D1);
  }

  return (i <= MAX_TRIES);
}

static bool disconnectModem(void)
{
  uint32_t i = 0;
  while ((i++ < MAX_TRIES) && (sim8xxIsConnected(&SIM8D1))) {
    sim8xxTogglePower(&SIM8D1);
  }

  return (i <= MAX_TRIES);
}

static SystemState_t systemInitHandler(SystemEvent_t evt)
{
  SystemState_t newState = SYSTEM_INIT;
  switch (evt) {
  case SYS_EVT_IGNITION_ON: {
    if (connectModem()) {
      GPS_Start();
      COT_Start();
      newState = SYSTEM_RIDING;
    } else {
      systemError = SYS_E_MODEM_POWER_ON;
      newState    = SYSTEM_ERROR;
    }
    break;
  }
  case SYS_EVT_IGNITION_OFF: {
    if (disconnectModem()) {
      COT_Stop();
      GPS_Stop();
      newState = SYSTEM_PARKING;
    } else {
      systemError = SYS_E_MODEM_POWER_OFF;
      newState    = SYSTEM_ERROR;
    }
    break;
  }
  default: {
    break;
  }
  }

  return newState;
}

static SystemState_t systemParkingHandler(SystemEvent_t evt)
{
  SystemState_t newState = SYSTEM_PARKING;
  switch (evt) {
  case SYS_EVT_IGNITION_ON: {
    if (connectModem()) {
      GPS_Start();
      COT_Start();
      newState = SYSTEM_RIDING;
    } else {
      systemError = SYS_E_MODEM_POWER_ON;
      newState    = SYSTEM_ERROR;
    }
  }
  case SYS_EVT_IGNITION_OFF:
  default: {
    break;
  }
  }
  return newState;
}

static SystemState_t systemRidingHandler(SystemEvent_t evt)
{
  SystemState_t newState = SYSTEM_RIDING;
  switch (evt) {
  case SYS_EVT_IGNITION_OFF: {
    COT_Stop();
    GPS_Stop();
    if (disconnectModem()) {
      newState = SYSTEM_PARKING;
    } else {
      systemError = SYS_E_MODEM_POWER_OFF;
      newState    = SYSTEM_ERROR;
    }
    break;
  }
  case SYS_EVT_IGNITION_ON:
  default: {
    break;
  }
  }

  return newState;
}

static SystemState_t systemTrackingHandler(SystemEvent_t evt)
{
  (void)evt;
  return SYSTEM_TRACKING;
}

static SystemState_t systemErrorHandler(SystemEvent_t evt)
{
  SystemState_t newState = SYSTEM_ERROR;
  switch (evt) {
  case SYS_EVT_IGNITION_OFF: {
    systemError = SYS_E_NO_ERROR;
    newState    = SYSTEM_INIT;
    break;
  }
  case SYS_EVT_IGNITION_ON:
  default: {
    break;
  }
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(SystemThread, arg)
{
  (void)arg;

  chRegSetThreadName(SYSTEM_THREAD_NAME);

  systemState = SYSTEM_INIT;

  while (true) {
    SystemEvent_t evt;
    if (MSG_OK ==
        chMBFetchTimeout(&systemMailbox, (msg_t *)&evt, TIME_INFINITE)) {
      switch (systemState) {
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
      case SYSTEM_ERROR: {
        systemState = systemErrorHandler(evt);
        break;
      }
      default: {
        ;
      }
      }
    }
  }
}

void SystemThreadInit(void)
{
  dbInit();
  memset(&events, 0, sizeof(events));
  chMBObjectInit(&systemMailbox, events, sizeof(events) / sizeof(events[0]));
}

void SystemThreadIgnitionOn(void)
{
  chSysLock();
  chMBPostI(&systemMailbox, SYS_EVT_IGNITION_ON);
  chSysUnlock();
}

void SystemThreadIgnitionOff(void)
{
  chSysLock();
  chMBPostI(&systemMailbox, SYS_EVT_IGNITION_OFF);
  chSysUnlock();
}

const char *SystemThreadGetStateString(void)
{
  static const char *const stateStr[] = {
      [SYSTEM_INIT]     = "INIT",
      [SYSTEM_PARKING]  = "PARKING",
      [SYSTEM_RIDING]   = "RIDING",
      [SYSTEM_TRACKING] = "TRACKING",
      [SYSTEM_ERROR]    = "ERROR",
  };

  return stateStr[(size_t)systemState];
}

const char *SystemThreadGetErrorString(void)
{
  static const char *const errorStr[] = {
      [SYS_E_NO_ERROR]        = "NO_ERROR",
      [SYS_E_MODEM_POWER_ON]  = "MODEM POWER ON",
      [SYS_E_MODEM_POWER_OFF] = "MODEM POWER OFF",
  };

  return errorStr[(size_t)systemError];
}

/****************************** END OF FILE **********************************/
