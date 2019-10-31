/**
 * @file PeripheralManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "PeripheralManagerThread.h"

#include "DebugShell.h"
#include "Logger.h"
#include "Sdcard.h"
#include "sim8xx.h"

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
  PRP_CMD_SDC_INSERTED,
  PRP_CMD_SDC_REMOVED,
  PRP_CMD_USB_CONNECTED,
  PRP_CMD_USB_DISCONNECTED,
} PRP_Command_t;

typedef struct PeripheralManager_s {
  msg_t commands[10];
  mailbox_t mailbox;
} PeripheralManager_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static SerialConfig sd_config = {
    115200,
    0,
    0,
    0,
};
static Sim8xxConfig sim_config = {
    &SD1,
    &sd_config,
    LINE_WAVESHARE_POWER,
};

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

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(PRP_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("peripheral");

  SDC_Init();
  DSH_Init();

  while (true) {
    PRP_Command_t cmd;
    if (MSG_OK == chMBFetchTimeout(&manager.mailbox, (msg_t *)&cmd, TIME_INFINITE)) {
      switch (cmd) {
      case PRP_CMD_SDC_INSERTED: {
        SDC_Mount();
        PRP_writeSysInfo();
        break;
      }
      case PRP_CMD_SDC_REMOVED: {
        SDC_Unmount();
        break;
      }
      case PRP_CMD_USB_CONNECTED: {
        DSH_Start();
        break;
      }
      case PRP_CMD_USB_DISCONNECTED: {
        DSH_Stop();
        break;
      }
      default: {
        ;
      }
      }
    }
  }
}

void PRP_Init(void)
{
  SIM_Init(&SIM8D1);
  SIM_Start(&SIM8D1, &sim_config);

  memset(manager.commands, 0, sizeof(manager.commands));
  chMBObjectInit(&manager.mailbox, manager.commands, ARRAY_LENGTH(manager.commands));
}

void PRP_SdcInserted(void)
{
  chSysLock();
  chMBPostI(&manager.mailbox, PRP_CMD_SDC_INSERTED);
  chSysUnlock();
}

void PRP_SdcRemoved(void)
{
  chSysLock();
  chMBPostI(&manager.mailbox, PRP_CMD_SDC_REMOVED);
  chSysUnlock();
}

void PRP_UsbConnected(void)
{
  chSysLock();
  chMBPostI(&manager.mailbox, PRP_CMD_USB_CONNECTED);
  chSysUnlock();
}

void PRP_UsbDisconnected(void)
{
  chSysLock();
  chMBPostI(&manager.mailbox, PRP_CMD_USB_DISCONNECTED);
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
