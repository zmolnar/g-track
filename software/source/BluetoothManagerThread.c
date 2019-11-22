/**
 * @file BluetoothManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BluetoothManagerThread.h"
#include "BluetoothShell.h"
#include "shell.h"
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

#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2048)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  BLT_STATE_INIT,
  BLT_STATE_DISABLED,
  BLT_STATE_CONNECTED,
  BLT_STATE_DISCONNECTED,
  BLT_STATE_ERROR,
} BLT_State_t;


typedef enum {
  BLT_CMD_START,
  BLT_CMD_STOP,
  BLT_CMD_PROCESS_URC,
  BLT_CMD_SEND_STREAM_DATA,
  BLT_CMD_SEND_USER_DATA,
} BLT_Command_t;

typedef struct {
  BLT_State_t state;
  msg_t commands[10];
  mailbox_t mailbox;
  Sim8xxCommand cmd;
  BluetoothStream_t stream;
  thread_reference_t shell;
} Bluetooth_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Bluetooth_t bluetooth;

static const ShellConfig BluetoothShellConfig = {
  (BaseSequentialStream *)&bluetooth.stream,
   BL_Commands,
};

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

static bool BLT_setHost(const char *host)
{
  SIM_CommandInit(&bluetooth.cmd);
  chsnprintf(bluetooth.cmd.request, sizeof(bluetooth.cmd.request),
             "AT+BTHOST=%s", host);
  SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

  return (SIM8XX_OK == bluetooth.cmd.status);
}

static bool BLT_setPin(const char *pin)
{
  SIM_CommandInit(&bluetooth.cmd);
  chsnprintf(bluetooth.cmd.request, sizeof(bluetooth.cmd.request),
             "AT+BTPAIRCFG=1,%s", pin);
  SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

  return (SIM8XX_OK == bluetooth.cmd.status);
}

static bool BLT_setupAndStart(void)
{
  bool result = false;

  if (BLT_setHost("gtrack")) {
    if (BLT_setPin("2019")) {
      if (BLT_powerOnDevice()) {
        result = true;
      } else {
        result = false;
      }
    } else {
      result = false;
    }
  } else {
    result = false;
  }

  return result;
}

static void BLT_startShell(void)
{
  if (!bluetooth.shell) {
    bluetooth.shell = chThdCreateFromHeap(NULL,
                                          SHELL_WA_SIZE,
                                          "bluetoothshell",
                                          NORMALPRIO + 1,
                                          shellThread,
                                          (void *)&BluetoothShellConfig);
  }
}

static void BLT_StopShell(void)
{
  if (bluetooth.shell && chThdTerminatedX(bluetooth.shell)) {
    chThdWait(bluetooth.shell);
    bluetooth.shell = NULL;
  }
}

static BLT_State_t BLT_initStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_INIT;

  switch(cmd) {
    case BLT_CMD_START: {
      if (BLT_setupAndStart()) {
        newState = BLT_STATE_DISCONNECTED;
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_STOP: {
      newState = BLT_STATE_DISABLED;
      break;
    }
    case BLT_CMD_PROCESS_URC:
    case BLT_CMD_SEND_STREAM_DATA:
    case BLT_CMD_SEND_USER_DATA:
    default: {
      break;
    }
  }

  if (BLT_STATE_INIT != newState)
    BLT_logStateChange(BLT_STATE_INIT, newState);

  return newState;
}

static BLT_State_t BLT_disabledStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_DISABLED;

  switch (cmd) {
    case BLT_CMD_START: {
      if (BLT_setupAndStart()) {
        newState = BLT_STATE_DISCONNECTED;
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_STOP:
    case BLT_CMD_PROCESS_URC:
    case BLT_CMD_SEND_STREAM_DATA:
    case BLT_CMD_SEND_USER_DATA:
    default: {
      break;
    }
  }

  if (BLT_STATE_DISABLED != newState)
    BLT_logStateChange(BLT_STATE_DISABLED, newState);

  return newState;
}

static BLT_State_t BLT_procesUrcInDisconnectedState(void)
{
  BLT_State_t state = BLT_STATE_DISCONNECTED;

  char urc[512] = {0};
  SIM_GetAndClearUrc(&SIM8D1, urc, sizeof(urc));

  if (URC_IsBtConnect(urc)) {
    URC_BtConnect_t connect = {0};
    if (!URC_BtConnectParse(urc, &connect))
      state = BLT_STATE_ERROR;
  } else if (URC_IsBtConnecting(urc)) {    
    SIM_CommandInit(&bluetooth.cmd);
    strncat(bluetooth.cmd.request, "AT+BTACPT=1", sizeof(bluetooth.cmd.request));
    SIM_ExecuteCommand(&SIM8D1, &bluetooth.cmd);

    if (SIM8XX_OK == bluetooth.cmd.status) {
      state = BLT_STATE_CONNECTED;
    } else {
      state = BLT_STATE_ERROR;
    }

  }

  return state;
}

static BLT_State_t BLT_disconnectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_DISCONNECTED;

  switch (cmd) {
    case BLT_CMD_STOP: {
      if (BLT_powerOffDevice()) {
        newState = BLT_STATE_DISABLED;
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_PROCESS_URC: {
      newState = BLT_procesUrcInDisconnectedState();
      break;
    }
    case BLT_CMD_START:
    case BLT_CMD_SEND_STREAM_DATA:
    case BLT_CMD_SEND_USER_DATA:
    default: {
      break;
    }
  }

  if (BLT_STATE_CONNECTED == newState)
    BLT_startShell();

  if (BLT_STATE_DISCONNECTED != newState)
    BLT_logStateChange(BLT_STATE_DISCONNECTED, newState);

  return newState;
}

static BLT_State_t BLT_procesUrcInConnectedState(void)
{
  BLT_State_t state = BLT_STATE_CONNECTED;

  char urc[512] = {0};
  SIM_GetAndClearUrc(&SIM8D1, urc, sizeof(urc));

  if (URC_IsBtSppData(urc)) {
    URC_BtSppData_t spp = {0};
    if (URC_BtSppDataParse(urc, &spp)) {
      BLS_ProcessRxData(&bluetooth.stream, spp.data, spp.length);
    }
  } else if (URC_IsBtDisconnect(urc)) {
    URC_BtDisconnect_t urcdata = {0};
    if (URC_BtDisconnectParse(urc, &urcdata)) {
      state = BLT_STATE_DISCONNECTED;
    } else {
      state = BLT_STATE_ERROR;
    }
  }

  return state;
}

static bool BLT_sendSppData(const uint8_t *data, size_t length)
{
  Sim8xxCommand *pcmd = &bluetooth.cmd;
  SIM_CommandInit(pcmd);
  strncat(pcmd->request, "AT+BTSPPSEND", sizeof(pcmd->request));
  size_t n = length < sizeof(pcmd->data) ? length : sizeof(pcmd->data);
  memcpy(pcmd->data, data, n);

  SIM_ExecuteCommand(&SIM8D1, pcmd);

  return (SIM8XX_SEND_OK == pcmd->status);
}

static BLT_State_t BLT_connectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_CONNECTED;

  switch (cmd) {
    case BLT_CMD_PROCESS_URC: {
      newState = BLT_procesUrcInConnectedState();
      break;
    }
    case BLT_CMD_STOP: {
      if (BLT_powerOffDevice()) {
        newState = BLT_STATE_DISABLED;
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_SEND_STREAM_DATA: {
      uint8_t *data = bluetooth.stream.tx.data;
      size_t length = bluetooth.stream.tx.end;
      if (BLT_sendSppData(data, length)) {
        BLS_ClearTxBuffer(&bluetooth.stream);
        BLS_NotifyWriter(&bluetooth.stream);
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_SEND_USER_DATA: {
      const uint8_t *data = bluetooth.stream.udata;
      size_t length = bluetooth.stream.ulength;
      if (BLT_sendSppData(data, length)) {
        BLS_NotifyWriter(&bluetooth.stream);
      } else {
        newState = BLT_STATE_ERROR;
      }
      break;
    }
    case BLT_CMD_START:
    default: {
      break;
    }
  }

  if (BLT_STATE_CONNECTED != newState) {
    BLT_StopShell();
    BLT_logStateChange(BLT_STATE_CONNECTED, newState);
  }

  return newState;
}

static BLT_State_t BLT_errorStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_ERROR;

  if (BLT_CMD_STOP == cmd) {
    BLT_powerOffDevice();
    newState = BLT_STATE_DISABLED;
  }

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
  BLS_ObjectInit(&bluetooth.stream);
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

void BLT_SendStreamDataI(void)
{
  chMBPostI(&bluetooth.mailbox, BLT_CMD_SEND_STREAM_DATA);
}


void BLT_SendStreamData(void)
{
  chSysLock();
  BLT_SendStreamDataI();
  chSysUnlock(); 
}

void BLT_SendUserData(void)
{
  chSysLock();
  chMBPostI(&bluetooth.mailbox, BLT_CMD_SEND_USER_DATA);
  chSysUnlock(); 
}

/****************************** END OF FILE **********************************/
