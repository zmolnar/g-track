/**
 * @file UrcBluetooth.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "UrcBluetooth.h"
#include "utils/Utils.h"
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
bool URC_IsBtUrc(const char str[])
{
    bool result = (URC_IsBtConnect(str) ||
                   URC_IsBtConnecting(str) || 
                   URC_IsBtSppData(str) ||
                   URC_IsBtDisconnect(str));

    return result;
}

bool URC_IsBtConnect(const char str[])
{
  return UTL_BeginsWith(str, "+BTCONNECT:");
}

bool URC_BtConnectParse(char str[], URC_BtConnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnect(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  ++start;

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->id, ','))
      return false;
  }

  // Skip '"' before the string
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->name, sizeof(urc->name), '"'))
      return false;
  }

  // Skip ',' after the string ending '"' character.
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->address, sizeof(urc->address), ','))
      return false;
  }

  // Skip '"' before the string
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->profile, sizeof(urc->profile), '"'))
      return false;
  }

  return true;
}

bool URC_IsBtDisconnect(const char str[])
{
  return UTL_BeginsWith(str, "+BTDISCONN:");
}

bool URC_BtDisconnectParse(char str[], URC_BtDisconnect_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtDisconnect(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  ++start;

  // Skip '"' before the string
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->name, sizeof(urc->name), '"'))
      return false;
  }

  // Skip ',' after the string ending '"' character.
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->address, sizeof(urc->address), ','))
      return false;
  }

  // Skip '"' before the string
  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->profile, sizeof(urc->profile), '"'))
      return false;
  }

  return true;
}

bool URC_IsBtConnecting(const char str[])
{
  return UTL_BeginsWith(str, "+BTCONNECTING:");
}

bool URC_BtConnectingParse(char str[], URC_BtConnecting_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtConnecting(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, '"');
  if (!start)
    return false;

  ++start;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->address, sizeof(urc->address), '"'))
      return false;
  }

  start = strchr(str, '"');
  if (!start)
    return false;

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->profile, sizeof(urc->profile), '"'))
      return false;
  }

  return true;
}

bool URC_IsBtSppData(const char str[])
{
  return UTL_BeginsWith(str, "+BTSPPDATA:");
}

bool URC_BtSppDataParse(char str[], URC_BtSppData_t *urc)
{
  memset(urc, 0, sizeof(*urc));

  if (!URC_IsBtSppData(str))
    return false;

  const char *end = str + strlen(str);

  char *start = strchr(str, ' ');
  if (!start)
    return false;

  ++start;

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->id, ','))
      return false;
  }

  if (start < end) {
    if (!UTL_GetNextInt(&start, &urc->length, ','))
      return false;
  }

  if (start < end) {
    if (!UTL_GetNextString(&start, urc->data, sizeof(urc->data), '\n'))
      return false;
  }

  return true;
}

/****************************** END OF FILE **********************************/
