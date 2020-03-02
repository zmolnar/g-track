/**
 * @file BluetoothManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BluetoothManagerThread.h"
#include "BluetoothShell.h"
#include "ConfigManagerThread.h"
#include "SystemThread.h"
#include "Logger.h"
#include "Sim8xx.h"
#include "SimHandlerThread.h"
#include "chprintf.h"
#include "hal.h"
#include "shell.h"

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
  BLT_CMD_PROCESS_EVENT,
  BLT_CMD_SEND_STREAM_DATA,
  BLT_CMD_SEND_USER_DATA,
} BLT_Command_t;

typedef struct {
  BLT_State_t state;
  msg_t commands[10];
  mailbox_t mailbox;
  BluetoothStream_t stream;
  thread_reference_t shell;
  GSM_BluetoothEvent_t btevent;
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
  if (bluetooth.shell) {
    chThdTerminate(bluetooth.shell);
    BLS_NotifyReader(&bluetooth.stream);
    chThdWait(bluetooth.shell);
    bluetooth.shell = NULL;
  }
}

static void BLT_eventCallback(GSM_BluetoothEvent_t *p)
{
  bluetooth.btevent = *p;
  BLT_ProcessEvent();
}

static bool BLT_setupAndStart(void)
{
  bool result          = false;
  const char *hostname = CFM_GetBluetoothHostName();
  const char *pin      = CFM_GetBluetoothPin();

  if (SIM_BluetoothSetup(&SIM868, hostname, pin)) {
    if (SIM_RegisterBluetoothCallback(&SIM868, BLT_eventCallback)) {
      if (SIM_BluetoothStart(&SIM868)) {
        result = true;
      }
    }
  }

  return result;
}

static BLT_State_t BLT_initStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_INIT;

  switch (cmd) {
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
  case BLT_CMD_PROCESS_EVENT:
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
  case BLT_CMD_PROCESS_EVENT:
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

BLT_State_t BLT_procesEventInConnectedState(void)
{
  BLT_State_t newState = BLT_STATE_CONNECTED;

  switch (bluetooth.btevent.type) {
  case GSM_BT_INCOMING_DATA: {
    BluetoothStream_t *stream = &bluetooth.stream;
    const char *idata         = bluetooth.btevent.payload.incomingData.data;
    size_t ilen               = strlen(idata);
    BLS_ProcessRxData(stream, idata, ilen);
    break;
  }
  case GSM_BT_DISCONNECTED: {
    BLT_StopShell();
    newState = BLT_STATE_DISCONNECTED;
    break;
  }
  case GSM_BT_CONNECTING:
  case GSM_BT_NO_EVENT:
  case GSM_BT_CONNECTED:
  default: {
    break;
  }
  }

  return newState;
}

static BLT_State_t BLT_connectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_CONNECTED;

  switch (cmd) {
  case BLT_CMD_PROCESS_EVENT: {
    newState = BLT_procesEventInConnectedState();
    break;
  }
  case BLT_CMD_STOP: {
    if (SIM_BluetoothStop(&SIM868)) {
      newState = BLT_STATE_DISABLED;
    } else {
      newState = BLT_STATE_ERROR;
    }
    break;
  }
  case BLT_CMD_SEND_STREAM_DATA: {
    char *data    = bluetooth.stream.tx.data;
    size_t length = bluetooth.stream.tx.end;
    if (SIM_BluetoothSendSppData(&SIM868, data, length)) {
      BLS_ClearTxBuffer(&bluetooth.stream);
      BLS_NotifyWriter(&bluetooth.stream);
    } else {
      newState = BLT_STATE_ERROR;
    }
    break;
  }
  case BLT_CMD_SEND_USER_DATA: {
    const char *data = bluetooth.stream.udata;
    size_t length    = bluetooth.stream.ulength;
    if (SIM_BluetoothSendSppData(&SIM868, data, length)) {
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

BLT_State_t BLT_procesEventInDisconnectedState(void)
{
  BLT_State_t newState = BLT_STATE_DISCONNECTED;

  switch (bluetooth.btevent.type) {
  case GSM_BT_CONNECTED: {
    if (0 == strncmp("SPP", bluetooth.btevent.payload.connected.profile, 3)) {
      BLT_startShell();
      newState = BLT_STATE_CONNECTED;
    }
    break;
  }
  case GSM_BT_CONNECTING: {
    if (0 == strncmp("SPP", bluetooth.btevent.payload.connecting.profile, 3)) {
      bool success = SIM_BluetoothAcceptConnection(&SIM868);
      if (success) {
        BLT_startShell();
        newState = BLT_STATE_CONNECTED;
      } else {
        newState = BLT_STATE_ERROR;
      }
    }
    break;
  }
  case GSM_BT_INCOMING_DATA: {
    newState = BLT_STATE_CONNECTED;
    BLT_startShell();
    BLT_ProcessEvent();
    break;
  }
  case GSM_BT_DISCONNECTED:
  case GSM_BT_NO_EVENT:
  default: {
    break;
  }
  }

  return newState;
}

static BLT_State_t BLT_disconnectedStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_DISCONNECTED;

  switch (cmd) {
  case BLT_CMD_PROCESS_EVENT: {
    newState = BLT_procesEventInDisconnectedState();
    break;
  }
  case BLT_CMD_STOP: {
    if (SIM_BluetoothStop(&SIM868)) {
      newState = BLT_STATE_DISABLED;
    } else {
      newState = BLT_STATE_ERROR;
    }
    break;
  }
  case BLT_CMD_SEND_STREAM_DATA:
  case BLT_CMD_SEND_USER_DATA:
  case BLT_CMD_START:
  default: {
    break;
  }
  }

  if (BLT_STATE_DISCONNECTED != newState) {
    BLT_logStateChange(BLT_STATE_CONNECTED, newState);
  }

  return newState;
}

static BLT_State_t BLT_errorStateHandler(BLT_Command_t cmd)
{
  BLT_State_t newState = BLT_STATE_ERROR;

  if (BLT_CMD_STOP == cmd) {
    SIM_BluetoothStop(&SIM868);
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

  SYS_WaitForSuccessfulInit();

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&bluetooth.mailbox, &msg, TIME_INFINITE)) {
      BLT_Command_t cmd = (BLT_Command_t)msg;
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
  BLS_ObjectInit(&bluetooth.stream);
  memset(&bluetooth.btevent, 0, sizeof(bluetooth.btevent));
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

void BLT_ProcessEvent(void)
{
  chSysLock();
  chMBPostI(&bluetooth.mailbox, BLT_CMD_PROCESS_EVENT);
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
