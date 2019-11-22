/**
 * @file AtClip.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "AtClip.h"
#include "hal.h"
#include "chprintf.h"

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
bool AT_ClipCreateOn(uint8_t buf[], size_t length)
{
  chsnprintf(buf, length, "AT+CLIP=1");
  return true;
}

bool AT_ClipCreateOff(uint8_t buf[], size_t length)
{
  chsnprintf(buf, length, "AT+CLIP=0");
  return true;
}

/****************************** END OF FILE **********************************/
