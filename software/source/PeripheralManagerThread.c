/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "PeripheralManagerThread.h"

#include "DebugShell.h"
#include "Sdcard.h"
#include "sim8xx.h"

#include <string.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/
typedef enum {
  SDC_INSERTED,
  SDC_REMOVED,
  USB_CONNECTED,
  USB_DISCONNECTED
} PeripheralEvent_t;

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static SerialConfig sd_config  = {115200, 0, 0, 0};
static Sim8xxConfig sim_config = {&SD1, &sd_config, LINE_WAVESHARE_POWER};

static msg_t events[10];
static mailbox_t periphMailbox;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void saveBuffer(const char *data, size_t length) {
  FIL log;
  if (FR_OK == f_open(&log, "/system.nfo", FA_CREATE_ALWAYS | FA_WRITE)) {
    UINT bw = 0;
    f_write(&log, data, length, &bw);
    f_close(&log);
  }
}

static void writeSysInfo(void) {
  char line[128]             = {0};
  char buf[8 * sizeof(line)] = {0};

  chsnprintf(line, sizeof(line), "Kernel:       %s\n", CH_KERNEL_VERSION);
  strncpy(buf, line, sizeof(buf));
#ifdef PORT_COMPILER_NAME
  chsnprintf(line, sizeof(line), "Compiler:     %s\n", PORT_COMPILER_NAME);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
  chsnprintf(line, sizeof(line), "Architecture: %s\n", PORT_ARCHITECTURE_NAME);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#ifdef PORT_CORE_VARIANT_NAME
  chsnprintf(line, sizeof(line), "Core Variant: %s\n", PORT_CORE_VARIANT_NAME);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
#ifdef PORT_INFO
  chsnprintf(line, sizeof(line), "Port Info:    %s\n", PORT_INFO);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
#ifdef PLATFORM_NAME
  chsnprintf(line, sizeof(line), "Platform:     %s\n", PLATFORM_NAME);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
#ifdef BOARD_NAME
  chsnprintf(line, sizeof(line), "Board:        %s\n", BOARD_NAME);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
#ifdef __DATE__
#ifdef __TIME__
  chsnprintf(line, sizeof(line), "Build time:   %s - %s\n", __DATE__, __TIME__);
  strncpy(buf + strlen(buf), line, sizeof(buf) - strlen(buf));
#endif
#endif

  saveBuffer(buf, strlen(buf));
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(PeripheralManagerThread, arg) {
  (void)arg;
  chRegSetThreadName("peripheral");

  sdcardInit();
  debugShellInit();

  while (true) {
    PeripheralEvent_t evt;
    if (MSG_OK == chMBFetchTimeout(&periphMailbox, (msg_t *)&evt, TIME_INFINITE)) {
      switch (evt) {
      case SDC_INSERTED: {
        sdcardMount();
        writeSysInfo();
        break;
      }
      case SDC_REMOVED: {
        sdcardUnmount();
        break;
      }
      case USB_CONNECTED: {
        debugShellStart();
        break;
      }
      case USB_DISCONNECTED: {
        debugShellStop();
        break;
      }
      default: { 
        ; 
      }
      }
    }
  }
}

void PeripheralManagerThreadInit(void) {
  sim8xxInit(&SIM8D1);
  sim8xxStart(&SIM8D1, &sim_config);

  memset(events, 0, sizeof(events));
  chMBObjectInit(&periphMailbox, events, sizeof(events) / sizeof(events[0]));
}

void PeripheralManagerSdcInserted(void) {
  chSysLock();
  chMBPostI(&periphMailbox, SDC_INSERTED);
  chSysUnlock();
}

void PeripheralManagerSdcRemoved(void) {
  chSysLock();
  chMBPostI(&periphMailbox, SDC_REMOVED);
  chSysUnlock();
}

void PeripheralManagerUsbConnected(void) {
  chSysLock();
  chMBPostI(&periphMailbox, USB_CONNECTED);
  chSysUnlock();
}

void PeripheralManagerUsbDisconnected(void) {
  chSysLock();
  chMBPostI(&periphMailbox, USB_DISCONNECTED);
  chSysUnlock();
}

/******************************* END OF FILE ***********************************/
