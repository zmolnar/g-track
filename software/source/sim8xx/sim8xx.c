/**
 * @file sim8xx.c
 * @brief SIM8xx modem driver.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "sim8xx.h"
#include "sim8xxReaderThread.h"
#include "chprintf.h"
#include <string.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
#define READER_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
void sim8xxInit(Sim8xxDriver *simp) {
  simp->config = NULL;
  simp->stream = NULL;
  simp->writer = NULL;
  simp->reader = NULL;
  chMtxObjectInit(&simp->lock);
  chMtxObjectInit(&simp->rxlock);
  chSemObjectInit(&simp->sync, 1);
  memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
  simp->rxlength = 0;
  simp->state = SIM8XX_STOP;
}

void sim8xxStart(Sim8xxDriver *simp, Sim8xxConfig *cfgp) {
  chMtxLock(&simp->lock);
  simp->config = cfgp;
  simp->stream = (BaseSequentialStream*)cfgp->sdp;
  sdStart(cfgp->sdp, cfgp->sdConfig);
  if(!simp->reader) {
    simp->reader = chThdCreateFromHeap(NULL, READER_WA_SIZE, "simreader",
                                       NORMALPRIO + 1, sim8xxReaderThread, (void*)simp);
  }
  simp->state = SIM8XX_READY;
  chMtxUnlock(&simp->lock);
}

void sim8xxCommandInit(Sim8xxCommand *cmdp) {
  memset(cmdp, 0, sizeof(Sim8xxCommand));
}

void sim8xxExecute(Sim8xxDriver *simp, Sim8xxCommand *cmdp) {
  chMtxLock(&simp->lock);
  chSemWait(&simp->sync);

  chprintf(simp->stream, "%s\r", cmdp->request);

  chSysLock();
  chThdSuspendS(&simp->writer);
  chSysUnlock();
  
  chMtxLock(&simp->rxlock);
  strcpy(cmdp->response, simp->rxbuf);
  chMtxUnlock(&simp->rxlock);

  chSysLock();
  simp->writer = NULL;
  chThdResumeS(&simp->reader, MSG_OK);
  chSysUnlock();
  
  chMtxUnlock(&simp->lock);
}

/******************************* END OF FILE ***********************************/

