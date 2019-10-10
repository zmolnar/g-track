/**
 * @file Logger.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Logger.h"

#include "Dashboard.h"
#include "Sdcard.h"
#include "ch.h"
#include "chprintf.h"

#include <string.h>

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
void LOG_Write(const char *file, const char *entry)
{
  char timestamp[25] = {0};
  dbCreateTimestamp(timestamp, sizeof(timestamp));
  
  sdcardLock();
  FIL logfile;
  if (FR_OK == f_open(&logfile, file, FA_OPEN_APPEND | FA_WRITE)) {
    UINT bw = 0;
    f_write(&logfile, timestamp, strlen(timestamp), &bw);
    f_write(&logfile, entry, strlen(entry), &bw);
    f_write(&logfile, "\n", 1, &bw);
    f_close(&logfile);
  }
  sdcardUnlock();
}

/****************************** END OF FILE **********************************/
