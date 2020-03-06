/**
 * @file ReporterThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Dashboard.h"
#include "ConfigManagerThread.h"
#include "ReporterThread.h"
#include "SimHandlerThread.h"
#include "SystemThread.h"
#include "Sim8xx.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define BUFFER_LENGTH 5
#define URL_LENGTH 512
#define BUFFER_WATERMARK 2

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
  RPT_CMD_GET_POSITION,
  RPT_CMD_EMPTY_BUFFER,
  RPT_CMD_SEND_DATA,
} RPT_Command_t;

typedef struct Reporter_s {
  RPT_State_t state;
  msg_t events[10];
  mailbox_t mailbox;
  const GprsConfig_t *config;
  virtual_timer_t timer;
  struct PositionBuffer_s {
    GPS_Data_t data[BUFFER_LENGTH];
    size_t wrindex;
    size_t rdindex;
  } buffer;
  char url[URL_LENGTH];
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
static void RPT_emptyBuffer(void)
{
  memset(reporter.buffer.data, 0, sizeof(reporter.buffer.data));
  reporter.buffer.wrindex = 0;
  reporter.buffer.rdindex = 0;
}

static void RPT_pushPositionIntoBuffer(void)
{
  GPS_Data_t *nextFree = &reporter.buffer.data[reporter.buffer.wrindex];
  DSB_GetPosition(nextFree);

  reporter.buffer.wrindex = (reporter.buffer.wrindex + 1) % BUFFER_LENGTH;
  if (reporter.buffer.wrindex == reporter.buffer.rdindex)
    reporter.buffer.rdindex = (reporter.buffer.rdindex + 1) % BUFFER_LENGTH;
}

static bool RPT_popOldestFromBuffer(GPS_Data_t *p)
{
  memset(p, 0, sizeof(*p));
  bool result = false;

  if (reporter.buffer.rdindex != reporter.buffer.wrindex) {
    memcpy(p, &reporter.buffer.data[reporter.buffer.rdindex], sizeof(*p));
    reporter.buffer.rdindex = (reporter.buffer.rdindex + 1) % BUFFER_LENGTH;
    result = true;
  }

  return result;
}

static bool RPT_dataNeedsToBeSent(void)
{
  size_t length = BUFFER_LENGTH + reporter.buffer.wrindex - reporter.buffer.rdindex;
  length %= BUFFER_LENGTH;

  return (BUFFER_WATERMARK <= length);
}

static size_t RPT_generateURL(void)
{
  size_t length = 0;



  return length;
}

static void RPT_IpCallback(GSM_IpEvent_t *event)
{
  switch(event->type) {
  case IP_EVENT_NET_DISCONNECTED: {
    // TODO implement net disconnect handler.
    break;
  }
  case IP_EVENT_HTTP_ACTION: {
    if (200 == event->payload.httpaction.httpStatus) {
      RPT_EmptyBuffer();
    } 
    break;
  }
  case IP_EVENT_NO_EVENT:
  default: {
    break;
  }
  }
}

static RPT_State_t RPT_InitStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_INIT;

  switch(cmd) {
  case RPT_CMD_START: {
    const char *apn = reporter.config->apn;
    SIM_IpSetup(&SIM868, apn);
    SIM_IpOpen(&SIM868);
    SIM_IpHttpStart(&SIM868);
    newState = RPT_STATE_ENABLED;
    break;
  }
  case RPT_CMD_STOP: {
    newState = RPT_STATE_DISABLED;
    break;
  }
  case RPT_CMD_GET_POSITION:
  case RPT_CMD_EMPTY_BUFFER:
  case RPT_CMD_SEND_DATA:
  default: {
    break;
  }
  }

  return newState;
}

static RPT_State_t RPT_EnabledStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_ENABLED;

  switch(cmd) {
  case RPT_CMD_STOP: {
    SIM_IpHttpStop(&SIM868);
    SIM_IpClose(&SIM868);
    newState = RPT_STATE_DISABLED;
    break;
  }
  case RPT_CMD_GET_POSITION: {
    RPT_pushPositionIntoBuffer();
    if (RPT_dataNeedsToBeSent()) {
      RPT_SendData();
    }
    break;
  }
  case RPT_CMD_SEND_DATA: {
    if (RPT_dataNeedsToBeSent()) {
      RPT_generateURL();
      SIM_IpHttpGet(&SIM868, reporter.url);
    }
    break;
  }
  case RPT_CMD_EMPTY_BUFFER: {
    RPT_emptyBuffer();
    break;
  }
  case RPT_CMD_START:
  default: {
    break;
  }
  }  

  return newState;
}

static RPT_State_t RPT_DisabledStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_DISABLED;

  switch(cmd) {
  case RPT_CMD_START: {
    const char *apn = reporter.config->apn;
    SIM_IpSetup(&SIM868, apn);
    SIM_IpOpen(&SIM868);
    SIM_IpHttpStart(&SIM868);
    newState = RPT_STATE_ENABLED;
    break;
  }
  case RPT_CMD_GET_POSITION:
  case RPT_CMD_EMPTY_BUFFER:
  case RPT_CMD_SEND_DATA:
  case RPT_CMD_STOP:
  default: {
    break;
  }
  }  

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(RPT_Thread, arg) {
  (void)arg;
  chRegSetThreadName("reporter");

  SYS_WaitForSuccessfulInit();

  reporter.config = CFM_GetGprsConfig();

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
  reporter.state = RPT_STATE_INIT;
  memset(reporter.events, 0, sizeof(reporter.events));
  chMBObjectInit(&reporter.mailbox, reporter.events, ARRAY_LENGTH(reporter.events));
  reporter.config = NULL;
  chVTObjectInit(&reporter.timer);
  memset(reporter.buffer.data, 0, sizeof(reporter.buffer.data));
  reporter.buffer.wrindex = 0;
  reporter.buffer.rdindex = 0;
  memset(reporter.url, 0, sizeof(reporter.url));

  SIM_RegisterIpCallback(&SIM868, RPT_IpCallback);
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

void RPT_GetPositionI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_GET_POSITION);
}

void RPT_GetPosition(void)
{
  chSysLock();
  RPT_GetPositionI();
  chSysUnlock();
}

void RPT_EmptyBufferI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_EMPTY_BUFFER);
}

void RPT_EmptyBuffer(void)
{
  chSysLock();
  RPT_EmptyBufferI();
  chSysUnlock();
}

void RPT_SendDataI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_SEND_DATA);
}

void RPT_SendData(void)
{
  chSysLock();
  RPT_SendDataI();
  chSysUnlock();
}

/****************************** END OF FILE **********************************/