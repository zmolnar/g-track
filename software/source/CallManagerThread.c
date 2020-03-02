/**
 * @file CallManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "CallManagerThread.h"
#include "SystemThread.h"
#include "Logger.h"
#include "Sim8xx.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof((a)[0]))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  CLL_STATE_INIT,
  CLL_STATE_ENABLED,
  CLL_STATE_DISABLED,
} CLL_State_t;

typedef struct CallManager_s {
  CLL_State_t state;
  msg_t commands[10];
  mailbox_t mailbox;
} CallManager_t;

typedef enum {
  CLL_CMD_START,
  CLL_CMD_STOP,
  CLL_CMD_URC_RECEIVED,
} CLL_Command_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static CallManager_t callmanager;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void CLL_unlockSimCard(void)
{
#if 0
  Sim8xxCommand cmd;
  SIM_CommandInit(&cmd);
  AT_CpinCreate(cmd.request, sizeof(cmd.request), "3943");
  SIM_ExecuteCommand(&SIM8D1, &cmd);

  if (SIM8XX_OK != cmd.status) {
    // TODO error handling.
  }
#endif  
}

static CLL_State_t CLL_initStateHandler(CLL_Command_t cmd)
{
  CLL_State_t newState = CLL_STATE_INIT;

  switch(cmd) {
    case CLL_CMD_START: {
      CLL_unlockSimCard();
      newState = CLL_STATE_ENABLED;
      break;
    }
    case CLL_CMD_STOP: {
      newState = CLL_STATE_DISABLED;
      break;
    }
    case CLL_CMD_URC_RECEIVED:
    default: {
      break;
    }
  }

  return newState;
}
#if 0
bool cpinReady = false;
bool callReady = false;
bool smsReady = false;
#endif
char urcbuf[512] = {0};

static void CLL_processUrc(void)
{
#if 0
  const char *urc = SIM_GetUrcMessage(&SIM8D1);
  if (!urc) return;

  memset(urcbuf, 0, sizeof(urcbuf));
  memcpy(urcbuf, urc, strlen(urc));

  SIM_ClearUrcMessage(&SIM8D1);

  LOG_Write("/urc.log", urcbuf);
  if (0 == strcasecmp(urcbuf, "\r\n+CPIN: READY\r\n"))
    cpinReady = true;
  else if (0 == strcasecmp(urcbuf, "\r\nCall Ready\r\n"))
    callReady = true;
  else if (0 == strcasecmp(urcbuf, "\r\nSms Ready\r\n"))
    smsReady = true;
  else
  {
    ;/* code */
  }
#endif
}

static CLL_State_t CLL_enabledStateHandler(CLL_Command_t cmd)
{
  CLL_State_t newState = CLL_STATE_ENABLED;

  switch (cmd) {
  case CLL_CMD_STOP: {
    newState = CLL_STATE_DISABLED;
    break;
  }
  case CLL_CMD_URC_RECEIVED: {
    CLL_processUrc();
    break;
  }
  case CLL_CMD_START:
  default: {
    break;
  }
  }

  return newState;
}

static CLL_State_t CLL_disabledStateHandler(CLL_Command_t cmd)
{
  CLL_State_t newState = CLL_STATE_DISABLED;

  switch (cmd) {
  case CLL_CMD_START: {
    newState = CLL_STATE_ENABLED;
    break;
  }
  case CLL_CMD_URC_RECEIVED:
  case CLL_CMD_STOP:
  default: {
    break;
  }
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(CLL_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("call");

  SYS_WaitForSuccessfulInit();

  while(true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&callmanager.mailbox, &msg, TIME_INFINITE)) {
      CLL_Command_t cmd = (CLL_Command_t)msg;
      switch (callmanager.state) {
        case CLL_STATE_INIT: {
          callmanager.state = CLL_initStateHandler(cmd);
          break;
        }
        case CLL_STATE_ENABLED: {
          callmanager.state = CLL_enabledStateHandler(cmd);
          break;
        }
        case CLL_STATE_DISABLED: {
          callmanager.state = CLL_disabledStateHandler(cmd);
          break;
        }
        default: {
          break;
        }
      }
    }
  }
} 

void CLL_Init(void)
{
  callmanager.state = CLL_STATE_INIT;
  memset(&callmanager.commands, 0, sizeof(callmanager.commands));
  chMBObjectInit(&callmanager.mailbox, 
                  callmanager.commands, 
                  ARRAY_LENGTH(callmanager.commands));
}

void CLL_Start(void)
{
  chSysLock();
  chMBPostI(&callmanager.mailbox, CLL_CMD_START);
  chSysUnlock();
}

void CLL_UrcReceived(void)
{
  chSysLock();
  chMBPostI(&callmanager.mailbox, CLL_CMD_URC_RECEIVED);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
