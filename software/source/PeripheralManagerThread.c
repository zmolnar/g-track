/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "PeripheralManagerThread.h"
#include "BoardEvents.h"
#include "Sdcard.h"
#include "DebugShell.h"
#include "sim8xx.h"
#include <string.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
static SerialConfig sd_config = {115200,0,0,0};
static Sim8xxConfig sim_config = {&SD1, &sd_config, LINE_WAVESHARE_POWER};

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
  char line[128] = {0};
  char buf[8*sizeof(line)] = {0};

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

static void sdcardInsertedHandler(eventid_t id) {
  (void)id;
  sdcardMount();
  writeSysInfo();
}

static void sdcardRemovedHandler(eventid_t id) {
  (void)id;
  sdcardUnmount();
}

static void usbConnectedHandler(eventid_t id) {
  (void)id;
  debugShellStart();
}

static void usbDisconnectedHandler(eventid_t id) {
  (void)id;
  debugShellStop();
}

static void debugShellTerminatedHandler(eventid_t id) {
  (void)id;
  debugShellTerminated();
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
THD_FUNCTION(PeripheralManagerThread, arg) {
  (void)arg;
  chRegSetThreadName("peripheral");

  static const evhandler_t eventHandlers[] = {
    sdcardInsertedHandler,
    sdcardRemovedHandler,
    usbConnectedHandler,
    usbDisconnectedHandler,
    debugShellTerminatedHandler
  };
  
  event_listener_t sdcardInsertedListener;
  event_listener_t sdcardRemovedListener;
  event_listener_t usbConnectedListener;
  event_listener_t usbDisconnectedListener;
  event_listener_t debugShellTerminatedListener;

  chEvtRegister(&besSdcardInserted, &sdcardInsertedListener, 0);
  chEvtRegister(&besSdcardRemoved, &sdcardRemovedListener, 1);
  chEvtRegister(&besUsbConnected, &usbConnectedListener, 2);
  chEvtRegister(&besUsbDisconnected, &usbDisconnectedListener, 3);
  chEvtRegister(&shell_terminated, &debugShellTerminatedListener, 4);

  sdcardInit();
  debugShellInit();

  while(true) {
    chEvtDispatch(eventHandlers, chEvtWaitOne(ALL_EVENTS));
  }
} 

void PeripheralManagerThreadInit(void) {
  sim8xxInit(&SIM8D1);
  sim8xxStart(&SIM8D1, &sim_config);
}

/******************************* END OF FILE ***********************************/
