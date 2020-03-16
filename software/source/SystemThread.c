/**
 * @file SystemThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "SystemThread.h"

#include "BluetoothManagerThread.h"
#include "CallManagerThread.h"
#include "ChainOilerThread.h"
#include "Dashboard.h"
#include "GpsReaderThread.h"
#include "Logger.h"
#include "SimHandlerThread.h"
#include "ReporterThread.h"

#include "chprintf.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof((a)[0]))

#define SYS_LOGFILE "/system.log"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  SYS_STATE_INIT,
  SYS_STATE_PARKING,
  SYS_STATE_RIDING,
  SYS_STATE_TRACKING,
} SYS_State_t;

typedef enum {
  SYS_CMD_IGNITION_ON,
  SYS_CMD_IGNITION_OFF,
} SYS_Command_t;

typedef struct System_s {
  msg_t events[10];
  mailbox_t mailbox;
  SYS_State_t state;
  semaphore_t sysinitialized;
} System_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
System_t system = {0};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static const char *SYS_getStateString(SYS_State_t state)
{
  static const char *const stateStrs[] = {
      [SYS_STATE_INIT]     = "INIT",
      [SYS_STATE_PARKING]  = "PARKING",
      [SYS_STATE_RIDING]   = "RIDING",
      [SYS_STATE_TRACKING] = "TRACKING",
  };

  return stateStrs[(size_t)state];
}

static void SYS_logStateChange(SYS_State_t from, SYS_State_t to)
{
  char entry[32] = {0};
  chsnprintf(entry, sizeof(entry), "%s -> %s", SYS_getStateString(from), SYS_getStateString(to));
  LOG_Write(SYS_LOGFILE, entry);
}

static SYS_State_t SYS_initStateHandler(SYS_Command_t evt)
{
  SYS_State_t newState = SYS_STATE_INIT;

  switch (evt) {
  case SYS_CMD_IGNITION_ON: {
    GPS_Start();
    COT_Start();
    BLT_Start();
    RPT_Start();
    // CLL_Start();
    newState = SYS_STATE_RIDING;
    chSemSignal(&system.sysinitialized);
   break;
  }
  case SYS_CMD_IGNITION_OFF: {
    BLT_Stop();
    COT_Stop();
    GPS_Stop();
    RPT_Stop();
    newState = SYS_STATE_PARKING;
    chSemSignal(&system.sysinitialized);
    break;
  }
  default: {
    break;
  }
  }

  if (SYS_STATE_INIT != newState)
    SYS_logStateChange(SYS_STATE_INIT, newState);

  return newState;
}

static SYS_State_t SYS_parkingStateHandler(SYS_Command_t evt)
{
  SYS_State_t newState = SYS_STATE_PARKING;

  switch (evt) {
  case SYS_CMD_IGNITION_ON: {
    GPS_Start();
    COT_Start();
    BLT_Start();
    RPT_Start();
    newState = SYS_STATE_RIDING;
    break;
  }
  case SYS_CMD_IGNITION_OFF:
  default: {
    break;
  }
  }

  if (SYS_STATE_PARKING != newState)
    SYS_logStateChange(SYS_STATE_PARKING, newState);

  return newState;
}

static SYS_State_t SYS_ridingStateHandler(SYS_Command_t evt)
{
  SYS_State_t newState = SYS_STATE_RIDING;

  switch (evt) {
  case SYS_CMD_IGNITION_OFF: {
    BLT_Stop();
    COT_Stop();
    GPS_Stop();
    RPT_Stop();
    newState = SYS_STATE_PARKING;
    break;
  }
  case SYS_CMD_IGNITION_ON:
  default: {
    break;
  }
  }

  if (SYS_STATE_RIDING != newState)
    SYS_logStateChange(SYS_STATE_RIDING, newState);

  return newState;
}

static SYS_State_t SYS_trackingStateHandler(SYS_Command_t evt)
{
  (void)evt;
  return SYS_STATE_TRACKING;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(SYS_Thread, arg)
{
  (void)arg;

  chRegSetThreadName(SYSTEM_THREAD_NAME);

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&system.mailbox, &msg, TIME_INFINITE)) {
      SYS_Command_t cmd = (SYS_Command_t)msg;
      switch (system.state) {
      case SYS_STATE_INIT: {
        system.state = SYS_initStateHandler(cmd);
        break;
      }
      case SYS_STATE_PARKING: {
        system.state = SYS_parkingStateHandler(cmd);
        break;
      }
      case SYS_STATE_RIDING: {
        system.state = SYS_ridingStateHandler(cmd);
        break;
      }
      case SYS_STATE_TRACKING: {
        system.state = SYS_trackingStateHandler(cmd);
        break;
      }
      default: {
        ;
      }
      }
    }
  }
}

void SYS_Init(void)
{
  memset(&system.events, 0, sizeof(system.events));
  chMBObjectInit(&system.mailbox, system.events, ARRAY_LENGTH(system.events));
  system.state = SYS_STATE_INIT;
  chSemObjectInit(&system.sysinitialized, 0);
}

void SYS_WaitForSuccessfulInit(void)
{
  while(MSG_OK != chSemWait(&system.sysinitialized))
    ;

  chSemSignal(&system.sysinitialized);
}

void SYS_IgnitionOn(void)
{
  chSysLock();
  chMBPostI(&system.mailbox, SYS_CMD_IGNITION_ON);
  chSysUnlock();
}

void SYS_IgnitionOff(void)
{
  chSysLock();
  chMBPostI(&system.mailbox, SYS_CMD_IGNITION_OFF);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
