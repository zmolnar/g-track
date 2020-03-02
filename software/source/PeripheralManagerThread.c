/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "PeripheralManagerThread.h"
#include "ChainOilerThread.h"
#include "SystemThread.h"

#include "ConfigManagerThread.h"
#include "DebugShell.h"
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
  PRP_EVENT_SDC,
  PRP_EVENT_USB,
  PRP_EVENT_IGNITION,
  PRP_EVENT_BT0,
  PRP_EVENT_SW1,
  PRP_EVENT_SW2,
} PRP_Event_t;

typedef struct PeripheralManager_s {
  msg_t events[10];
  mailbox_t mailbox;
  struct EventCounter_s {
    uint32_t sdc;
    uint32_t usb;
    uint32_t ignition;
    uint32_t bt0;
    uint32_t sw1;
    uint32_t sw2;
  } counter;

} PeripheralManager_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static PeripheralManager_t manager;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void PRP_writeSysInfo(void)
{
  char line[128]             = {0};
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

  LOG_Overwrite(PRP_SYS_FILE, sysinfo);
}

void PRP_IgnitionCallback(void *p) 
{
  (void)p;
  chSysLockFromISR();
  manager.counter.ignition++;
  chMBPostI(&manager.mailbox, PRP_EVENT_IGNITION);
  chSysUnlockFromISR();
}

void PRP_SdcDetectCallback(void *p) 
{
  (void)p;
  chSysLockFromISR();
  manager.counter.sdc++;
  chMBPostI(&manager.mailbox, PRP_EVENT_SDC);
  chSysUnlockFromISR();
}

void PRP_USBCallback(void *p)
{
  (void)p;
  chSysLockFromISR();
  manager.counter.usb++;
  chMBPostI(&manager.mailbox, PRP_EVENT_USB);
  chSysUnlockFromISR();
}

void PRP_Button0Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  manager.counter.bt0++;
  chMBPostI(&manager.mailbox, PRP_EVENT_BT0);
  chSysUnlockFromISR();
}

void PRP_Switch1Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  manager.counter.sw1++;
  chMBPostI(&manager.mailbox, PRP_EVENT_SW1);
  chSysUnlockFromISR();
}

void PRP_Switch2Callback(void *p)
{
  (void)p;
  chSysLockFromISR();
  manager.counter.sw2++;
  chMBPostI(&manager.mailbox, PRP_EVENT_SW2);
  chSysUnlockFromISR();
}

void PRP_ConfigureGPIOInterrupts(void)
{
  palSetLineCallback(LINE_EXT_IGNITION, PRP_IgnitionCallback, NULL);
  palEnableLineEvent(LINE_EXT_IGNITION, PAL_EVENT_MODE_BOTH_EDGES);

  palSetLineCallback(LINE_SDC_CARD_DETECT, PRP_SdcDetectCallback, NULL);
  palEnableLineEvent(LINE_SDC_CARD_DETECT, PAL_EVENT_MODE_BOTH_EDGES);

  palSetLineCallback(LINE_USB_VBUS_SENSE, PRP_USBCallback, NULL);
  palEnableLineEvent(LINE_USB_VBUS_SENSE, PAL_EVENT_MODE_BOTH_EDGES);

  palSetLineCallback(LINE_BT0, PRP_Button0Callback, NULL);
  palEnableLineEvent(LINE_BT0, PAL_EVENT_MODE_BOTH_EDGES);

  palSetLineCallback(LINE_EXT_SW1, PRP_Switch1Callback, NULL);
  palEnableLineEvent(LINE_EXT_SW1, PAL_EVENT_MODE_BOTH_EDGES);

  palSetLineCallback(LINE_EXT_SW2, PRP_Switch2Callback, NULL);
  palEnableLineEvent(LINE_EXT_SW2, PAL_EVENT_MODE_BOTH_EDGES);
}

static bool PRP_isSdcInserted(void)
{
  return PAL_HIGH == palReadLine(LINE_SDC_CARD_DETECT);
}

static bool PRP_isUsbConnected(void)
{
  return PAL_HIGH == palReadLine(LINE_USB_VBUS_SENSE);
}

static bool PRP_isButton0Pressed(void)
{
  return PAL_LOW == palReadLine(LINE_BT0);
}

static bool PRP_isIgnitionOn(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_IGNITION);
}

static bool PRP_isSw1Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW1);
}

static bool PRP_isSw2Pressed(void)
{
  return PAL_HIGH == palReadLine(LINE_EXT_SW2);
}

static void PRP_collectInitialEvents(void)
{
  chSysLockFromISR();

  chMBPostI(&manager.mailbox, PRP_EVENT_IGNITION);

  if (PRP_isSdcInserted())
    chMBPostI(&manager.mailbox, PRP_EVENT_SDC);

  if (PRP_isUsbConnected())
    chMBPostI(&manager.mailbox, PRP_EVENT_USB);

  if (PRP_isButton0Pressed())
    chMBPostI(&manager.mailbox, PRP_EVENT_BT0);

  if (PRP_isSw1Pressed())
    chMBPostI(&manager.mailbox, PRP_EVENT_SW1);

  if (PRP_isSw2Pressed())
    chMBPostI(&manager.mailbox, PRP_EVENT_SW2);

  chSysUnlockFromISR();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(PRP_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("peripheral");

  PRP_collectInitialEvents();

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&manager.mailbox, &msg, TIME_INFINITE)) {
      PRP_Event_t evt = (PRP_Event_t)msg;
      switch (evt) {
      case PRP_EVENT_SDC: {
        if (PRP_isSdcInserted()) {
          SDC_Mount();
          PRP_writeSysInfo();
          CFM_LoadConfig();
        } else {
          SDC_Unmount();
        }
        break;
      }
      case PRP_EVENT_USB: {
        if (PRP_isUsbConnected()) {
          DSH_Start();
        } else {
          DSH_Stop();
        }
        break;
      }
      case PRP_EVENT_IGNITION: {
        if (PRP_isIgnitionOn()) {
          SYS_IgnitionOn();
        } else {
          SYS_IgnitionOff();
        }
        break;
      }
      case PRP_EVENT_BT0: {
        if (PRP_isButton0Pressed()) {
          COT_OneShot();
        }

        break;
      }
      case PRP_EVENT_SW1: {
        if (PRP_isSw1Pressed()) {
          ;
        }
        break;
      }
      case PRP_EVENT_SW2: {
        if (PRP_isSw2Pressed()) {
          ;
        }
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
  memset(manager.events, 0, sizeof(manager.events));
  chMBObjectInit(&manager.mailbox, manager.events, ARRAY_LENGTH(manager.events));
  memset(&manager.counter, 0, sizeof(manager.counter));

  SDC_Init();
  DSH_Init();
}

/****************************** END OF FILE **********************************/
