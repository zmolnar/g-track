/**
 * @file ReporterThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ReporterThread.h"
#include "SimHandlerThread.h"
#include "Sim8xx.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  RPT_STATE_INIT,
  RPT_STATE_ENABLED,
  RPT_STATE_DISABLED,
} RPT_State_t;

typedef enum {
  RPT_CMD_START,
  RPT_CMD_STOP,
  RPT_CMD_SEND,
} RPT_Command_t;

typedef struct Reporter_s {
  virtual_timer_t timer;
  RPT_State_t state;
  msg_t events[10];
  mailbox_t mailbox;
  semaphore_t modemReady;
} Reporter_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Reporter_t reporter;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void RPT_ModemCallback(GSM_ModemEvent_t *event)
{
  switch(event->type) {
    case MODEM_EVENT_SIM_UNLOCKED: {
      chSysLock();
      chSemResetI(&reporter.modemReady, 1);
      chSysUnlock();
    }
    case MODEM_NO_EVENT:
    default: {
      break;
    }
  }
}

static void RPT_IpCallback(GSM_IpEvent_t *event)
{
  static uint32_t i = 0;
  i++;
}

static void RPT_UnlockSIMCardAndWaitToBeReady(void)
{
  SIM_UnlockSIMCard(&SIM868, "3943");

  while (MSG_OK != chSemWait(&reporter.modemReady))
    ;
}

static void timercallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  chMBPostI(&reporter.mailbox, RPT_CMD_SEND);
  chVTSetI(&reporter.timer, TIME_S2I(10), timercallback, NULL);
  chSysUnlockFromISR();
}

static RPT_State_t RPT_InitStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_INIT;

  switch(cmd) {
  case RPT_CMD_START: {
    RPT_UnlockSIMCardAndWaitToBeReady();
    SIM_IpSetup(&SIM868, "internet.vodafone.net");
    SIM_IpOpen(&SIM868);
    SIM_IpHttpStart(&SIM868);
    chVTSet(&reporter.timer, TIME_S2I(10), timercallback, NULL);
    newState = RPT_STATE_ENABLED;
    break;
  }
  case RPT_CMD_STOP: {

    break;
  }
  default: {
    break;
  }
  }

  return newState;
}

static RPT_State_t RPT_EnabledStateHandler(RPT_Command_t cmd)
{
  const char *url = "http://www.melcontrol.com/gtrack.php?message=20200228122429 HelloGPRS!";
  SIM_IpHttpGet(&SIM868, url);

  return RPT_STATE_ENABLED;
}

static RPT_State_t RPT_DisabledStateHandler(RPT_Command_t cmd)
{
  return RPT_STATE_DISABLED;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(RPT_Thread, arg) {
  (void)arg;
  chRegSetThreadName("reporter");

  SIM_RegisterModemCallback(&SIM868, RPT_ModemCallback);
  SIM_RegisterIpCallback(&SIM868, RPT_IpCallback);

  // Just for testing
  chThdSleepSeconds(10);
  RPT_Start();

  while(true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&reporter.mailbox, &msg, TIME_INFINITE)) {
      RPT_Command_t cmd = (RPT_Command_t)msg;
      switch(reporter.state) {
      case RPT_STATE_INIT: {
        reporter.state = RPT_InitStateHandler(cmd);
        break;
      }
      case RPT_STATE_ENABLED: {
        reporter.state = RPT_EnabledStateHandler(cmd);
        break;
      }
      case RPT_STATE_DISABLED: {
        reporter.state = RPT_DisabledStateHandler(cmd);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
} 

void RPT_Init(void)
{
  chVTObjectInit(&reporter.timer);
  memset(reporter.events, 0, sizeof(reporter.events));
  chMBObjectInit(&reporter.mailbox, reporter.events, ARRAY_LENGTH(reporter.events));
  chSemObjectInit(&reporter.modemReady, 0);
  reporter.state = RPT_STATE_INIT;
}

void RPT_StartI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_START);
}

void RPT_Start(void)
{
  chSysLock();
  RPT_StartI();
  chSysUnlock();
}

void RPT_StopI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_STOP);
}

void RPT_Stop(void)
{
  chSysLock();
  RPT_StopI();
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
