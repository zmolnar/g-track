/**
 * @file ReporterThread.h
 * @brief
 */

#ifndef REPORTER_THREAD_H
#define REPORTER_THREAD_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ch.h"

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
THD_FUNCTION(RPT_Thread, arg);

void RPT_Init(void);

void RPT_StartI(void);

void RPT_Start(void);

void RPT_StopI(void);

void RPT_Stop(void);

void RPT_CreateRecordI(void);

void RPT_CreateRecord(void);

void RPT_EraseSentRecordsI(void);

void RPT_EraseSentRecords(void);

void RPT_CancelLastTransactionI(void);

void RPT_CancelLastTransaction(void);

void RPT_ReconnectI(void);

void RPT_Reconnect(void);

#endif /* REPORTER_THREAD_H */

/****************************** END OF FILE **********************************/
