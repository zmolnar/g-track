/**
 * @file AtUtil.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "utils/Utils.h"

#include "ch.h"
#include "stdlib.h"
#include "string.h"

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
bool UTL_BeginsWith(const char str[], const char pre[])
{
  return strncasecmp(pre, str, strlen(pre)) == 0;
}

bool UTL_GetNextInt(char **start, int *value, char delim)
{
  char *end = strchr(*start, delim);
  if (!end)
    return false;
  *end   = '\0';
  *value = atoi(*start);
  *start = end + 1;
  return true;
}

double UTL_AsciiToDouble(char str[])
{
  size_t len = strlen(str);
  if (0 == len)
    return 0.0;

  double val = 0.0;
  size_t i;
  for (i = 0; i < len && str[i] != '.'; ++i) {
    val = 10 * val + (str[i] - '0');
  }

  if (i == len)
    return val;
  i++;

  double f = 1.0;
  while (i < len) {
    f *= 0.1;
    val += f * (str[i++] - '0');
  }

  return val;
}

bool UTL_GetNextDouble(char **start, double *value, char delim)
{
  char *end = strchr(*start, delim);
  if (!end)
    return false;
  *end   = '\0';
  *value = UTL_AsciiToDouble(*start);
  *start = end + 1;
  return true;
}

bool UTL_GetNextString(char **start, char **buf, char delim)
{
  char *end = strchr(*start, delim);
  if (!end)
    return false;
  *end = '\0';
  *buf = *start;
  *start = end + 1;
  return true;
}

bool UTL_SkipReserved(char **start, size_t num, char delim)
{
  while (num--) {
    char *end = strchr(*start, delim);
    if (!end)
      return false;
    *end = '\0';
    *start = end + 1;
  }
  return true;
}

/***************************** END OF FILE * *********************************/
