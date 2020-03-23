/**
 * @file ReporterThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Dashboard.h"
#include "ConfigManagerThread.h"
#include "SystemThread.h"
#include "Logger.h"
#include "ReporterThread.h"
#include "Record.h"
#include "SimHandlerThread.h"
#include "SystemThread.h"
#include "Sim8xx.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define URL_LENGTH 512
#define MAX_NUM_OF_RECORD_IN_URL 3
#define BUFFER_WATERMARK 3
#define RECONNECT_PERIOD 60000
#define REPORTER_LOGFILE "/reporter.log"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  RPT_STATE_INIT,
  RPT_STATE_ENABLED,
  RPT_STATE_DISABLED,
  RPT_STATE_RECONNECTING,
} RPT_State_t;

typedef enum {
  RPT_CMD_START,
  RPT_CMD_STOP,
  RPT_CMD_CREATE_RECORD,
  RPT_CMD_ERASE_SENT_RECORDS,
  RPT_CMD_SEND_DATA,
  RPT_CMD_RESEND_DATA,
  RPT_CMD_RECONNECT,
} RPT_Command_t;

typedef struct Reporter_s {
  RPT_State_t state;
  msg_t events[10];
  mailbox_t mailbox;
  virtual_timer_t timer;
  const VehicleConfig_t *vehicleConfig;
  const GprsConfig_t *gprsConfig;
  const BackendConfig_t *backendConfig;
  RecordBuffer_t records;
  char url[URL_LENGTH];
  bool transactionIsPending;
  bool stopIsPostponed;
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
static void RPT_createRecord(Record_t *prec)
{
  GPS_Data_t gpsData;
  DSB_GetPosition(&gpsData);

  prec->deviceId = 0;
  strncpy(prec->vehicleId, reporter.vehicleConfig->id, sizeof(prec->vehicleId));
  prec->year = gpsData.time.year;
  prec->month = gpsData.time.month;
  prec->day = gpsData.time.day;
  prec->hour = gpsData.time.hour;
  prec->minute = gpsData.time.min;
  prec->second = gpsData.time.sec;
  prec->utcOffset = gpsData.utcOffset;
  prec->latitude = gpsData.latitude;
  prec->longitude = gpsData.longitude;
  prec->speed = (uint32_t)gpsData.speed;
  prec->numOfSatInUse = gpsData.gnssSatInUse;
  prec->batteryVoltage = 0.0;
  prec->gsmSignalStrehgth = SIM_GetSignalStrength(&SIM868);
  prec->systemMode = SYS_GetSystemState();
}

static void RPT_saveRecord(void)
{
  Record_t record;
  RPT_createRecord(&record);
  REC_PushRecord(&reporter.records, &record);
}

static bool RPT_dataNeedsToBeSent(void)
{
  size_t count = REC_GetNumOfUnsentRecords(&reporter.records);

  return (BUFFER_WATERMARK <= count);
}

static size_t RPT_generateURL(void)
{
  memset(reporter.url, 0, sizeof(reporter.url));
  strncpy(reporter.url, reporter.backendConfig->url, sizeof(reporter.url));

  size_t numOfRec = REC_GetSize(&reporter.records);

  if (MAX_NUM_OF_RECORD_IN_URL < numOfRec)
    numOfRec = MAX_NUM_OF_RECORD_IN_URL;

  size_t n = strlen(reporter.url);
  for (size_t i = 0; i < numOfRec; ++i) {
    Record_t *prec;
    REC_GetNextRecordAndMarkAsSent(&reporter.records, &prec);
    n += REC_Serialize(prec, i, &reporter.url[n], sizeof(reporter.url) - 1 - n);
    if (i < (numOfRec - 1)) {
      strncat(reporter.url, "&", sizeof(reporter.url) - 1 - n);
      ++n;
    }
  }

  return n;
}

static void RPT_removeSentRecords(void)
{
  while(REC_PopRecordIfSent(&reporter.records))
    ;
}

static void RPT_IpCallback(GSM_IpEvent_t *event)
{
  switch(event->type) {
  case IP_EVENT_NET_DISCONNECTED: {
    RPT_Reconnect();
    break;
  }
  case IP_EVENT_HTTP_ACTION: {
    palClearLine(LINE_EXT_LED);
    int32_t status = event->payload.httpaction.httpStatus;
    if (200 == status) {
      RPT_EraseSentRecords();
    } 
    else if (600 <= status) {
      RPT_Reconnect();
    } 
    else {
      RPT_ResendData();
    }
    break;
  }
  case IP_EVENT_NO_EVENT:
  default: {
    break;
  }
  }
}

static bool RPT_StartIpConnection(void)
{
  bool result     = false;
  const char *apn = reporter.gprsConfig->apn;

  if (SIM_IpSetup(&SIM868, apn)) {
    if (SIM_IpOpen(&SIM868)) {
      if (SIM_IpHttpStart(&SIM868)) {
        result = true;
      } 
    }
  }

  return result;
}

static RPT_State_t RPT_InitStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_INIT;

  switch(cmd) {
  case RPT_CMD_START: {
    REC_EmptyBuffer(&reporter.records);
    reporter.transactionIsPending = false;
    reporter.stopIsPostponed = false;
    if (RPT_StartIpConnection()) {
      newState = RPT_STATE_ENABLED;
    } else {
      RPT_Start();
      chThdSleepMilliseconds(1000);
    }
    break;
  }
  case RPT_CMD_STOP: {
    newState = RPT_STATE_DISABLED;
    break;
  }
  case RPT_CMD_CREATE_RECORD:
  case RPT_CMD_ERASE_SENT_RECORDS:
  case RPT_CMD_SEND_DATA:
  case RPT_CMD_RESEND_DATA:
  case RPT_CMD_RECONNECT:
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
    if (!reporter.transactionIsPending) {
      SIM_IpHttpStop(&SIM868);
      SIM_IpClose(&SIM868);
      newState = RPT_STATE_DISABLED;
    } else {
      reporter.stopIsPostponed = true;
    }
    break;
  }
  case RPT_CMD_CREATE_RECORD: {
    RPT_saveRecord();
    if (!reporter.transactionIsPending) {
      RPT_SendData();
    }
    break;
  }
  case RPT_CMD_SEND_DATA: {
    if (RPT_dataNeedsToBeSent()) {
      RPT_generateURL();
      while (!SIM_IpHttpGet(&SIM868, reporter.url))
        chThdSleepMilliseconds(1000);
      reporter.transactionIsPending = true;
      palSetLine(LINE_EXT_LED);
    }
    break;
  }
  case RPT_CMD_ERASE_SENT_RECORDS: {
    RPT_removeSentRecords();
    reporter.transactionIsPending = false;
    if (reporter.stopIsPostponed) {
      reporter.stopIsPostponed = false;
      RPT_Stop();
    } else {
      if (RPT_dataNeedsToBeSent()) {
        RPT_SendData();
      }
    }
    break;
  }
  case RPT_CMD_RESEND_DATA: {
    REC_CancelLastTransaction(&reporter.records);
    reporter.transactionIsPending = false;
    LOG_AppendToFile(REPORTER_LOGFILE, "Sending data failed, resend records.");
    if (reporter.stopIsPostponed) {
      reporter.stopIsPostponed = false;
      RPT_Stop();
    } else {
      if (RPT_dataNeedsToBeSent()) {
        RPT_SendData();
      }
    }
    break;
  }
  case RPT_CMD_RECONNECT: {
    REC_CancelLastTransaction(&reporter.records);
    reporter.transactionIsPending = false;
    LOG_AppendToFile(REPORTER_LOGFILE, "GPRS connection is lost, try to reconnect.");
    if (reporter.stopIsPostponed) {
      reporter.stopIsPostponed = false;
      RPT_Stop();
    } else {
      RPT_Reconnect();
      newState = RPT_STATE_RECONNECTING;
    }

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
    REC_EmptyBuffer(&reporter.records);
    reporter.transactionIsPending = false;
    reporter.stopIsPostponed = false;
    if (RPT_StartIpConnection()) {
      newState = RPT_STATE_ENABLED;
    } else {
      RPT_Start();
      chThdSleepMilliseconds(1000);
    }
    break;
  }
  case RPT_CMD_CREATE_RECORD:
  case RPT_CMD_ERASE_SENT_RECORDS:
  case RPT_CMD_SEND_DATA:
  case RPT_CMD_RESEND_DATA:
  case RPT_CMD_STOP:
  case RPT_CMD_RECONNECT:
  default: {
    break;
  }
  }  

  return newState;
}


static void RPT_reconnectTimerCallback(void *p)
{
  (void)p;
  RPT_Reconnect();
}

static RPT_State_t RPT_ReconnectingStateHandler(RPT_Command_t cmd)
{
  RPT_State_t newState = RPT_STATE_RECONNECTING;

  switch(cmd) {
  case RPT_CMD_RECONNECT: {
    if (RPT_StartIpConnection()) {
      LOG_AppendToFile(REPORTER_LOGFILE, "Reconnect to GPRS succeeded.");
      RPT_SendData();
      newState = RPT_STATE_ENABLED;
    } else {
      LOG_AppendToFile(REPORTER_LOGFILE, "Reconnect to GPRS failed.");
      chVTSet(&reporter.timer, TIME_MS2I(RECONNECT_PERIOD), RPT_reconnectTimerCallback, NULL);
    }
    break;
  }
  case RPT_CMD_STOP: {
    newState = RPT_STATE_DISABLED;
    break;
  }
  case RPT_CMD_CREATE_RECORD:
  case RPT_CMD_ERASE_SENT_RECORDS:
  case RPT_CMD_SEND_DATA:
  case RPT_CMD_RESEND_DATA:
  case RPT_CMD_START:  
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
  CFM_WaitForValidConfig();
  
  reporter.vehicleConfig = CFM_GetVehicleConfig();
  reporter.gprsConfig = CFM_GetGprsConfig();
  reporter.backendConfig = CFM_GetBackendConfig();

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
      case RPT_STATE_RECONNECTING: {
        reporter.state = RPT_ReconnectingStateHandler(cmd);
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
  chVTObjectInit(&reporter.timer);
  reporter.vehicleConfig = NULL;
  reporter.gprsConfig = NULL;
  reporter.backendConfig = NULL;
  REC_EmptyBuffer(&reporter.records);
  memset(reporter.url, 0, sizeof(reporter.url));
  reporter.transactionIsPending = false;
  reporter.stopIsPostponed = false;

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

void RPT_CreateRecordI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_CREATE_RECORD);
}

void RPT_CreateRecord(void)
{
  chSysLock();
  RPT_CreateRecordI();
  chSysUnlock();
}

void RPT_EraseSentRecordsI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_ERASE_SENT_RECORDS);
}

void RPT_EraseSentRecords(void)
{
  chSysLock();
  RPT_EraseSentRecordsI();
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

void RPT_ResendDataI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_RESEND_DATA);
}

void RPT_ResendData(void)
{
  chSysLock();
  RPT_ResendDataI();
  chSysUnlock();
}

void RPT_ReconnectI(void)
{
  chMBPostI(&reporter.mailbox, RPT_CMD_RECONNECT);
}

void RPT_Reconnect(void)
{
  chSysLock();
  RPT_ReconnectI();
  chSysUnlock();
}


/****************************** END OF FILE **********************************/
