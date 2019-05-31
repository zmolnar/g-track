/**
 * @file ShellCommands.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ShellCommands.h"
#include "shell.h"
#include "chprintf.h"

#include "Dashboard.h"
#include "SystemThread.h"
#include "GpsReaderThread.h"
#include "ChainOilerThread.h"

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
void shGetSystemStatus(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    DateTime_t dt = {0};
    dbGetTime(&dt);

    chprintf(chp, SHELL_NEWLINE_STR);
    chprintf(chp,"%02d-%02d-%d %02d:%02d:%02d",
        dt.month, dt.day, dt.year, dt.hour, dt.min, dt.sec);
    chprintf(chp, SHELL_NEWLINE_STR);        

    chprintf(chp, "%10s    %10s    %10s" SHELL_NEWLINE_STR, 
        "thread", "state", "error" );

    chprintf(chp, "%10s    %10s    %10s" SHELL_NEWLINE_STR, 
        SYSTEM_THREAD_NAME, 
        SystemThreadGetStateString(), 
        SystemThreadGetErrorString());

    chprintf(chp, "%10s    %10s    %10s" SHELL_NEWLINE_STR, 
        GPS_READER_THREAD_NAME, 
        GpsReaderGetStateString(), 
        GpsReaderGetErrorString());

    chprintf(chp, "%10s    %10s    %10s" SHELL_NEWLINE_STR, 
        CHAIN_OILER_THREAD_NAME, 
        ChainOilerGetStateString(), 
        ChainOilerGetErrorString());
}

/****************************** END OF FILE **********************************/
