/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "PeripheralManagerThread.h"
#include "SimHandlerThread.h"
#include "ChainOilerThread.h"
#include "SystemThread.h"
#include "ConfigManagerThread.h"
#include "Logger.h"
#include "Sdcard.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define PRP_SYS_FILE  "/system.nfo"
#define ARRAY_LENGTH(array) (sizeof((array))/(sizeof((array)[0])))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  PRP_EVENT_IGN,
  PRP_EVENT_SDC,
  PRP_EVENT_BT0,
  PRP_EVENT_SW1,
  PRP_EVENT_SW2,
} PRP_Event_t;

typedef struct PeripheralManager_s {
  msg_t events[10];
  mailbox_t mailbox;
  const SimConfig_t *config;
  virtual_timer_t sw1timer;
  struct EventCounter_s {
    uint32_t ign;
    uint32_t sdc;
    uint32_t bt0;
    uint32_t sw1;
    uint32_t sw2;
  } counter;
  struct PadState_s {
    uint32_t ign : 1;
    uint32_t sdc : 1;
    uint32_t bt0 : 1;
    uint32_t sw1 : 1;
    uint32_t sw2 : 1;
  } padState;
} PeripheralManager_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static PeripheralManager_t peripheralManager;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void PRP_writeSysInfo(void)
{
  char line[128]                 = {0};
  char sysinfo[8 * sizeof(line)] = {0};

  chsnprintf(line, sizeof(line), "Kernel:       %s\n", CH_KERNEL_VERSION);
  strncpy(sysinfo, line, sizeof(sysinfo));
#ifdef PORT_COMPILER_NAME
  chsnprintf(line, sizeof(line), "Compiler:     %s\n", PORT_COMPILER_NAME);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
  chsnprintf(line, sizeof(line), "Architecture: %s\n", PORT_ARCHITECTURE_NAME);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#ifdef PORT_CORE_VARIANT_NAME
  chsnprintf(line, sizeof(line), "Core Variant: %s\n", PORT_CORE_VARIANT_NAME);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
#ifdef PORT_INFO
  chsnprintf(line, sizeof(line), "Port Info:    %s\n", PORT_INFO);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
#ifdef PLATFORM_NAME
  chsnprintf(line, sizeof(line), "Platform:     %s\n", PLATFORM_NAME);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
#ifdef BOARD_NAME
  chsnprintf(line, sizeof(line), "Board:        %s\n", BOARD_NAME);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
#ifdef __DATE__
#ifdef __TIME__
  chsnprintf(line, sizeof(line), "Build time:   %s - %s\n", __DATE__, __TIME__);
  strncpy(sysinfo + strlen(sysinfo), line, sizeof(sysinfo) - strlen(sysinfo));
#endif
#endif

  LOG_OverWriteFile(PRP_SYS_FILE, sysinfo);
}

void PRP_IgnitionCallback(void *p) 
{
  (void)p;
  chSysLockFromISR();
  peripheralManager.counter.ign++;
  palDisableLineEventI(LINE_EXT_IGNITION);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_IGN);
  chSysUnlockFromISR();
}

void PRP_SdcDetectCallback(void *p) 
{
  (void)p;
  chSysLockFromISR();
  peripheralManager.counter.sdc++;
  palDisableLineEventI(LINE_SDC_CARD_DETECT);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_SDC);
  chSysUnlockFromISR();
}

void PRP_Button0Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  peripheralManager.counter.bt0++;
  palDisableLineEventI(LINE_BT0);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_BT0);
  chSysUnlockFromISR();
}

void PRP_Switch1Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  peripheralManager.counter.sw1++;
  palDisableLineEventI(LINE_EXT_SW1);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_SW1);
  chSysUnlockFromISR();
}

void PRP_Switch2Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  peripheralManager.counter.sw2++;
  palDisableLineEventI(LINE_EXT_SW2);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_SW2);
  chSysUnlockFromISR();
}

static bool PRP_isIgnitionOn(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_IGNITION);
}

static bool PRP_isSdcInserted(void)
{
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT);
}

static bool PRP_isButton0Pressed(void)
{
  return PAL_LOW == palReadLine(LINE_BT0);
}

static bool PRP_isSw1Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW1);
}

static bool PRP_isSw2Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW2);
}

static void PRP_readInitialStates(void)
{
  peripheralManager.padState.ign = palReadLine(LINE_EXT_IGNITION);
  peripheralManager.padState.sdc = palReadLine(LINE_SDC_CARD_DETECT);
  peripheralManager.padState.bt0 = palReadLine(LINE_BT0);
  peripheralManager.padState.sw1 = palReadLine(LINE_EXT_SW1);
  peripheralManager.padState.sw2 = palReadLine(LINE_EXT_SW2);
}

static void PRP_collectInitialEvents(void)
{
  chSysLock();

  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_IGN);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_BT0);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_SW1);
  chMBPostI(&peripheralManager.mailbox, PRP_EVENT_SW2);

  chSysUnlock();
}

static void PRP_waitForSdcardAndLoadConfig(void)
{
  bool succeeded = false;

  while (!succeeded) {
    palSetLine(LINE_LED_2_RED);

    while(!PRP_isSdcInserted()) {
      palToggleLine(LINE_LED_2_RED);
      chThdSleepMilliseconds(125);
    }

    palSetLine(LINE_LED_2_RED);

    if (PRP_isSdcInserted()) {
      if (SDC_Mount()) {
        palClearLine(LINE_LED_2_RED);
        PRP_writeSysInfo();
        CFM_LoadConfig();
        succeeded = true;
      }
    }
  }

  CFM_WaitForValidConfig();
  
  palSetLineCallback(LINE_SDC_CARD_DETECT, PRP_SdcDetectCallback, NULL);
  palEnableLineEvent(LINE_SDC_CARD_DETECT, PAL_EVENT_MODE_BOTH_EDGES);

  peripheralManager.config = CFM_GetSimConfig();
}

static void PRP_setupModem(void)
{
  const char *pin = peripheralManager.config->pin;
  while(!SHD_ResetAndConnectModem(pin))
    ;
}

static void PRP_sw1TimerCallback(void *p)
{
  (void)p;

  if (PRP_isSw1Pressed()) {
    COT_ForceStart();
  }
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(PRP_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("peripheral");

  PRP_waitForSdcardAndLoadConfig();
  PRP_setupModem();
  PRP_readInitialStates();
  PRP_collectInitialEvents();

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&peripheralManager.mailbox, &msg, TIME_INFINITE)) {
      PRP_Event_t evt = (PRP_Event_t)msg;
      switch (evt) {
      case PRP_EVENT_IGN: {
        chThdSleepMilliseconds(20);
        peripheralManager.padState.ign = palReadLine(LINE_EXT_IGNITION);
        if (PRP_isIgnitionOn()) {
          SYS_IgnitionOn();
        } else {
          SYS_IgnitionOff();
        }
        palSetLineCallback(LINE_EXT_IGNITION, PRP_IgnitionCallback, NULL);
        palEnableLineEvent(LINE_EXT_IGNITION, PAL_EVENT_MODE_BOTH_EDGES);
        break;
      }  
      case PRP_EVENT_SDC: {
        chThdSleepMilliseconds(100);
        peripheralManager.padState.sdc = palReadLine(LINE_SDC_CARD_DETECT);
        if (PRP_isSdcInserted()) {
          if (SDC_Mount()) {
            palClearLine(LINE_LED_2_RED);
            PRP_writeSysInfo();
            CFM_LoadConfig();
          } else {
            palSetLine(LINE_LED_2_RED);
          }
        } else {
          if (SDC_Unmount())
            palSetLine(LINE_LED_2_RED);
        }
        palSetLineCallback(LINE_SDC_CARD_DETECT, PRP_SdcDetectCallback, NULL);
        palEnableLineEvent(LINE_SDC_CARD_DETECT, PAL_EVENT_MODE_BOTH_EDGES);
        break;
      }
      case PRP_EVENT_BT0: {
        chThdSleepMilliseconds(20);
        peripheralManager.padState.bt0 = palReadLine(LINE_BT0);
        if (PRP_isButton0Pressed()) {
          COT_OneShot();
        }
        palSetLineCallback(LINE_BT0, PRP_Button0Callback, NULL);
        palEnableLineEvent(LINE_BT0, PAL_EVENT_MODE_BOTH_EDGES);
        break;
      }
      case PRP_EVENT_SW1: {
        chThdSleepMilliseconds(20);
        peripheralManager.padState.sw1 = palReadLine(LINE_EXT_SW1);
        if (PRP_isSw1Pressed()) {
          chSysLock();
          chVTSetI(&peripheralManager.sw1timer, TIME_MS2I(980),PRP_sw1TimerCallback, NULL);
          chSysUnlock();
        } else {
          chSysLock();
          if (chVTIsArmedI(&peripheralManager.sw1timer)) {
            chVTResetI(&peripheralManager.sw1timer);
            COT_OneShotI();
          } else {
            COT_ForceStopI();
          }
          chSysUnlock();
        }
        palSetLineCallback(LINE_EXT_SW1, PRP_Switch1Callback, NULL);
        palEnableLineEvent(LINE_EXT_SW1, PAL_EVENT_MODE_BOTH_EDGES);
        break;
      }
      case PRP_EVENT_SW2: {
        chThdSleepMilliseconds(20);
        peripheralManager.padState.sw2 = palReadLine(LINE_EXT_SW2);
        if (PRP_isSw2Pressed()) {
          ;
        }
        palSetLineCallback(LINE_EXT_SW2, PRP_Switch2Callback, NULL);
        palEnableLineEvent(LINE_EXT_SW2, PAL_EVENT_MODE_BOTH_EDGES);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
}

void PRP_Init(void)
{
  memset(peripheralManager.events, 0, sizeof(peripheralManager.events));
  chMBObjectInit(&peripheralManager.mailbox, peripheralManager.events, ARRAY_LENGTH(peripheralManager.events));
  peripheralManager.config = NULL;
  chVTObjectInit(&peripheralManager.sw1timer);
  memset(&peripheralManager.counter, 0, sizeof(peripheralManager.counter));
  memset(&peripheralManager.padState, 0, sizeof(peripheralManager.padState));

  SDC_Init();
}

/****************************** END OF FILE **********************************/

