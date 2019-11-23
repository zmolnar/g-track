/**
 * @file Sdcard.h
 * @brief
 */

#ifndef SDCARD_H
#define SDCARD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ff.h"
#include "hal.h"
#include "chprintf.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
/**
 * @brief Initialize SD card resources.
 */
void SDC_Init(void);

/**
 * @brief Lock SD card.
 */
void SDC_Lock(void);

/**
 * @brief Unlock SD card.
 */
void SDC_Unlock(void);

/**
 * @brief Mount filesystem on SD card.
 */
void SDC_Mount(void);

/**
 * @brief Unmount filesystem on SD card.
 */
void SDC_Unmount(void);

/**
 * @brief Recursively list all files in the filesystem.
 */
void SDC_Tree(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* SDCARD_H */

/****************************** END OF FILE **********************************/
