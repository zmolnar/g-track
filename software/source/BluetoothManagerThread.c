/**
 * @file BluetoothManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BluetoothManagerThread.h"
#include "Logger.h"
#include "sim8xx.h"
#include "at.h"
#include "urc.h"
#include "hal.h"
#include "chprintf.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

#define BLT_LOGFILE "/bluetooth.log"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  BLT_STATE_INIT,
  BLT_STATE_ENABLED,
  BLT_STATE_DISABLED,
  BLT_STATE_CONNECTED,
  BLT_STATE_DISCONNECTED,
  BLT_STATE_ERROR,
} BLT_State_t;

typedef enum {
  BLT_CMD_START,
  BLT_CMD_STOP,
  BLT_CMD_PROCESS_URC,
} BLT_Command_t;

typedef struct {
  BLT_State_t state;
  msg_t commands[10];
  mailbox_t mailbox;
  Sim8xxCommand cmd;
} Bluetooth_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Bluetooth_t bluetooth = {0};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static const char *BLT_getStateString(BLT_State_t state)
{
  static const char *const stateStrs[] = {
      [BLT_STATE_INIT]         = "INIT",
      [BLT_STATE_ENABLED]      = "ENABLED",
      [BLT_STATE_DISABLED]     = "DISABLED",
      [BLT_STATE_CONNECTED]    = "CONNECTED",
      [BLT_STATE_DISCONNECTED] = "DISCONNECTED",
      [BLT_STATE_ERROR]        = "ERROR",

  };

  return stateStrs[(size_t)state];
}

static void BLT_logStateChange(BLT_State_t from, BLT_State_t to)
{
  char entry[32] = {0};
  chsnprintf(entry, sizeof(entry), "%s -> %s", BLT_getStateString(from), BLT_getStateString(to));
  LOG_Write(BLT_LOGFILE, entry);
}

static bool BLT_powerOnDevice(void)
{
  SIM_CommandInit(&bluetooth.cmd);
  AT_BtpowerCreateOn(bluetooth.cmd.request, sizeof(bluetooth.cmd.request));
  SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

  return (SIM8XX_OK == bluetooth.cmd.status);
}

static bool BLT_powerOffDevice(void)
{
  SIM_CommandInit(&bluetooth.cmd);
  AT_BtpowerCreateOff(bluetooth.cmd.request, sizeof(bluetooth.cmd.request));
  SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

  return (SIM8XX_OK == bluetooth.cmd.status);
}

static BLT_State_t BLT_processUrc(void)
{
  BLT_State_t state = BLT_STATE_ENABLED;

  char urc[512] = {0};
  char *urctext = SIM_GetUrcMessage(&SIM8D1);
  strncpy(urc, urctext, strlen(urctext));
  SIM_ClearUrcMessage(&SIM8D1);

  if (URC_IsBtConnecting(urc)) {    
    SIM_CommandInit(&bluetooth.cmd);
    strncat(bluetooth.cmd.request, "AT+BTACPT=1", sizeof(bluetooth.cmd.request));
    SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

    if (SIM8XX_OK == bluetooth.cmd.status) {
      state = BLT_STATE_CONNECTED;
    } else {
      state = BLT_STATE_ERROR;
    }
  } else if (URC_IsBtSppData(urc)) {
    URC_BtSppData_t sppdata = {0};
    if (URC_BtSppDataParse(urc, &sppdata)) {
      Sim8xxCommand *pcmd = &bluetooth.cmd;
      SIM_CommandInit(pcmd);
      strncat(pcmd->request, "AT+BTSPPSEND", sizeof(pcmd->request));
      chsnprintf(pcmd->data, sizeof(pcmd->data), "%s", sppdata.data);
      SIM_ExecuteCommand(&SIM8D1, pcmd);

      if (SIM8XX_SEND_OK != bluetooth.cmd.status) {
        state = BLT_STATE_ERROR;
      }
    }
  } else if (URC_IsBtConnect(urc)) {
    URC_BtConnect_t connect = {0};
    if (URC_BtConnectParse(urc, &connect))
      ;//state = BLT_STATE_CONNECTED;
  }

  return state;
}

static BLT_State_t BLT_initStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_INIT;

  switch(cmd) {
    case BLT_CMD_START: {
      if (BLT_powerOnDevice())
        newState = BLT_STATE_ENABLED;
      else 
        newState = BLT_STATE_ERROR;
      break;
    }
    case BLT_CMD_STOP: {
      newState = BLT_STATE_DISABLED;
      break;
    }
    case BLT_CMD_PROCESS_URC:
    default: {
      break;
    }
  }

  if (BLT_STATE_INIT != newState)
    BLT_logStateChange(BLT_STATE_INIT, newState);

  return newState;
}

static BLT_State_t BLT_enabledStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_ENABLED;

  switch (cmd) {
    case BLT_CMD_STOP: {
      if (BLT_powerOffDevice())
        newState = BLT_STATE_DISABLED;
      else
        newState = BLT_STATE_ERROR;
      break;
    }
    case BLT_CMD_PROCESS_URC: {
      newState = BLT_processUrc();
      break;
    }
    case BLT_CMD_START:
    default: {
      break;
    }
  }

  if (BLT_STATE_ENABLED != newState)
    BLT_logStateChange(BLT_STATE_ENABLED, newState);

  return newState;
}

static BLT_State_t BLT_disabledStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_DISABLED;

  switch (cmd) {
    case BLT_CMD_START: {
      newState = BLT_STATE_ENABLED;
      break;
    }
    case BLT_CMD_STOP:
    case BLT_CMD_PROCESS_URC:
    default: {
      break;
    }
  }

  if (BLT_STATE_DISABLED != newState)
    BLT_logStateChange(BLT_STATE_DISABLED, newState);

  return newState;
}

static BLT_State_t BLT_connectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_CONNECTED;

  switch (cmd) {
    case BLT_CMD_PROCESS_URC: {
      newState = BLT_processUrc();
      break;
    }
    case BLT_CMD_STOP: {
      newState = BLT_STATE_DISABLED;
    }
    case BLT_CMD_START:
    default: {
      break;
    }
  }

  if (BLT_STATE_CONNECTED != newState)
    BLT_logStateChange(BLT_STATE_CONNECTED, newState);

  return newState;
}

static BLT_State_t BLT_disconnectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_DISCONNECTED;


  if (BLT_STATE_DISCONNECTED != newState)
    BLT_logStateChange(BLT_STATE_DISCONNECTED, newState);

  return newState;
}

static BLT_State_t BLT_errorStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_ERROR;

  if (BLT_STATE_ERROR != newState)
    BLT_logStateChange(BLT_STATE_ERROR, newState);

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(BLT_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("bluetooth");

  bluetooth.state = BLT_STATE_INIT;

  while(true) {
    BLT_Command_t cmd;
    if (MSG_OK == chMBFetchTimeout(&bluetooth.mailbox, (msg_t *)&cmd, TIME_INFINITE)) {
      switch (bluetooth.state) {
        case BLT_STATE_INIT: {
          bluetooth.state = BLT_initStateHandler(cmd);
          break;
        }
        case BLT_STATE_ENABLED: {
          bluetooth.state = BLT_enabledStateHandler(cmd);
          break;
        }
        case BLT_STATE_DISABLED: {
          bluetooth.state = BLT_disabledStateHandler(cmd);
          break;
        }
        case BLT_STATE_CONNECTED: {
          bluetooth.state = BLT_connectedStateHandler(cmd);
          break;
        }
        case BLT_STATE_DISCONNECTED: {
          bluetooth.state = BLT_disconnectedStateHandler(cmd);
          break;
        }
        case BLT_STATE_ERROR: {
          bluetooth.state = BLT_errorStateHandler(cmd);
          break;
        }
        default: {
          break;
        }
      }
    }
  }
} 

void BLT_Init(void)
{
  bluetooth.state = BLT_STATE_INIT;
  memset(bluetooth.commands, 0, sizeof(bluetooth.commands));
  chMBObjectInit(&bluetooth.mailbox, bluetooth.commands, ARRAY_LENGTH(bluetooth.commands));
  memset(&bluetooth.cmd, 0, sizeof(bluetooth.cmd));
}

void BLT_Start(void)
{
  chSysLock();
  chMBPostI(&bluetooth.mailbox, BLT_CMD_START);
  chSysUnlock();
}

void BLT_Stop(void)
{
  chSysLock();
  chMBPostI(&bluetooth.mailbox, BLT_CMD_STOP);
  chSysUnlock();
}

void BLT_ProcessUrc(void)
{
  chSysLock();
  chMBPostI(&bluetooth.mailbox, BLT_CMD_PROCESS_URC);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
