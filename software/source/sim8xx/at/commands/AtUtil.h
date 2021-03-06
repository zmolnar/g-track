/**
 * @file AtUtil.h
 * @brief
 */

#ifndef ATUTIL_H
#define ATUTIL_H

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
bool atGetNextInt(char **start, int *value, char delim);

double atAsciiToDouble(char str[]);

bool atGetNextDouble(char **start, double *value, char delim);

bool atGetNextString(char **start, char *buf, size_t length, char delim);

bool atSkipReserved(char **start, size_t num, char delim);

void atExchangeChar(char *c, char *tmp);

void atRestoreChar(char *c, char *tmp);

#endif /* ATUTIL_H */

/****************************** END OF FILE **********************************/
