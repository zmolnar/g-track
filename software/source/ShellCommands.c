/**
 * @file ShellCommands.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ShellCommands.h"

#include "ChainOilerThread.h"
#include "Dashboard.h"
#include "GpsReaderThread.h"
#include "SystemThread.h"
#include "chprintf.h"
#include "shell.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void shGetSystemStatus(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  DSB_DateTime_t dt = {0};
  DSB_GetTime(&dt);

  chprintf(chp, SHELL_NEWLINE_STR);
  chprintf(chp,
           "RTC: %02d-%02d-%d %02d:%02d:%02d",
           dt.month,
           dt.day,
           dt.year,
           dt.hour,
           dt.min,
           dt.sec);
  chprintf(chp, SHELL_NEWLINE_STR SHELL_NEWLINE_STR);

  chprintf(chp,
           "%10s    %10s    %10s" SHELL_NEWLINE_STR SHELL_NEWLINE_STR,
           "thread",
           "state",
           "error");

  chprintf(chp,
           "%10s    %10s    %10s" SHELL_NEWLINE_STR,
           SYSTEM_THREAD_NAME,
           SYS_GetStateString(),
           SYS_GetErrorString());

  chprintf(chp,
           "%10s    %10s    %10s" SHELL_NEWLINE_STR,
           GPS_READER_THREAD_NAME,
           GPS_GetStateString(),
           GPS_GetErrorString());

  chprintf(chp,
           "%10s    %10s    %10s" SHELL_NEWLINE_STR,
           CHAIN_OILER_THREAD_NAME,
           COT_GetStateString(),
           COT_GetErrorString());
}

/****************************** END OF FILE **********************************/
