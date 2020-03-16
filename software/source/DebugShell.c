/**
 * @file DebugShell.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "DebugShell.h"

#include "Sdcard.h"
#include "ShellCommands.h"
#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "usbcfg.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2048)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static thread_reference_t shelltp;

static const ShellCommand commands[] = {
    {"tree", SDC_Tree},
    {NULL, NULL},
};

static const ShellConfig usb_shell_cfg = {
    (BaseSequentialStream *)&SDU1,
    commands,
};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void DSH_Init(void)
{
  sduObjectInit(&SDU1);
  shellInit();
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
}

void DSH_Start(void)
{
  sduStart(&SDU1, &serusbcfg);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(500);

  if (!shelltp) {
    shelltp = chThdCreateFromHeap(NULL,
                                  SHELL_WA_SIZE,
                                  "usbshell",
                                  NORMALPRIO + 1,
                                  shellThread,
                                  (void *)&usb_shell_cfg);
  }
}

void DSH_Stop()
{
  if (shelltp && chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);
    shelltp = NULL;
  }

  usbDisconnectBus(serusbcfg.usbp);
  usbStop(serusbcfg.usbp);
  sduStop(&SDU1);
}

/**************************** END OF FILE ************************************/
