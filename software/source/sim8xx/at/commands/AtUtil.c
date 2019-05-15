/**
 * @file AtUtil.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "AtUtil.h"
#include "ch.h"
#include "string.h"
#include "stdlib.h"

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
bool atGetNextInt(char **start, int *value, char delim) {
  char *end = strchr(*start, delim);
  if (!end) return false;
  *end = '\0';
  *value = atoi(*start);
  *end = delim;
  *start = end + 1;
  return true;
}

double atAsciiToDouble(char str[]) {
  size_t len = strlen(str);
  if (0 == len) return 0.0;

  double val = 0.0;
  size_t i;
  for (i = 0; i < len && str[i] != '.'; ++i) {
    val = 10 * val + (str[i] - '0');
  }

  if (i == len) return val;
  i++;

  double f = 1.0;
  while (i < len) {
    f *= 0.1;
    val += f * (str[i++] - '0');
  }

  return val;
}

bool atGetNextDouble(char **start, double *value, char delim) {
  char *end = strchr(*start, delim);
  if (!end) return false;
  *end = '\0';
  *value = atAsciiToDouble(*start);
  *end = delim;
  *start = end + 1;
  return true;
}

bool atGetNextString(char **start, char *buf, size_t length, char delim) {
  char *end = strchr(*start, delim);
  if (!end) return false;
  *end = '\0';
  strncpy(buf, *start, length);
  *end = delim;
  *start = end + 1;
  return true;
}

bool atSkipReserved(char **start, size_t num, char delim) {
  while (num--) {
    char *end = strchr(*start, delim);
    if (!end) return false;
    *start = end + 1;
  }
  return true;
}

void atExchangeChar(char *c, char *tmp) {
  *tmp = *c;
  *c = '\0';
}

void atRestoreChar(char *c, char *tmp) { *c = *tmp; }

/***************************** END OF FILE * *********************************/
