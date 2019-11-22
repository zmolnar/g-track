/**
 * @file sim8xxUrcThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "sim8xxUrcThread.h"

#include "sim8xx.h"
#include "source/CallManagerThread.h"
#include "source/BluetoothManagerThread.h"

#include "urc/urc.h"
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
static bool SIM_notifyUrcListener(char urc[])
{
  bool result = false;
#if 0
  if (SIM_beginsWith(urc, "\r\n+cpin:") || 
      SIM_beginsWith(urc, "\r\ncall") ||
      SIM_beginsWith(urc, "\r\nsms")) {
    CLL_UrcReceived();
    result = true;
  } else {
    result = false;
  }
#endif

  if (URC_IsBtUrc(urc)) {
    BLT_ProcessUrc();
    result = true;
  }

  return result;
}

static void SIM_waitForClear(Sim8xxDriver *simp)
{
  chSemWait(&simp->urcClear);
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(SIM_UrcThread, arg) {
  Sim8xxDriver *simp = (Sim8xxDriver *)arg;

  while(true) {
    memset(simp->urcbuf, 0, sizeof(simp->urcbuf));
    
    chSysLock();
    msg_t msg = chThdSuspendS(&simp->urcprocessor);
    simp->urcprocessor = NULL;
    chSysUnlock();

    if (MSG_OK == msg) {
      chMtxLock(&simp->rxlock);
      if (sizeof(simp->urcbuf) - 1< simp->urclength)
        simp->urclength = sizeof(simp->urcbuf) - 1;
      memcpy(simp->urcbuf, simp->urc, simp->urclength;
      chMtxUnlock(&simp->rxlock);
      chSemSignal(&simp->urcSync);

      if (SIM_notifyUrcListener(simp->urcbuf)) 
        SIM_waitForClear(simp);
    } else {
      // TODO something....
    }
  }
} 

/****************************** END OF FILE **********************************/
