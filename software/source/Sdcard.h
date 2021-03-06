/**
 * @file Sdcard.h
 * @brief
 */

#ifndef SDCARD_H
#define SDCARD_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "hal.h"
#include "chprintf.h"
#include "ff.h"

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                             */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                             */
/*******************************************************************************/
void sdcardInit(void);
void sdcardMount(void);
void sdcardUnmount(void);

void sdcardCmdTree(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* SDCARD_H */

/******************************* END OF FILE ***********************************/
