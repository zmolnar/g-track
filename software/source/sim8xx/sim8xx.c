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
Sim8xxDriver SIM8D1;

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
  sdStart(cfgp->sdp, cfgp->sdConfig);
  simp->reader = NULL;
  chThdCreateFromHeap(NULL, READER_WA_SIZE, "sim8xx",
                      NORMALPRIO + 1, sim8xxReaderThread, (void*)simp);
  
  simp->state = SIM8XX_READY;
  chMtxUnlock(&simp->lock);
}

void sim8xxCommandInit(Sim8xxCommand *cmdp) {
  memset(cmdp, 0, sizeof(Sim8xxCommand));
  cmdp->status = SIM8XX_INVALID_STATUS;
}

void sim8xxExecute(Sim8xxDriver *simp, Sim8xxCommand *cmdp) {
  chMtxLock(&simp->lock);
  chSemWait(&simp->sync);

  chprintf((BaseSequentialStream*)simp->config->sdp, "%s\r", cmdp->request);

  chSysLock();
  msg_t msg = chThdSuspendTimeoutS(&simp->writer, chTimeMS2I(5000));
  simp->writer = NULL;
  chSysUnlock();

  if (MSG_OK == msg) {
    chMtxLock(&simp->rxlock);
    strcpy(cmdp->response, simp->rxbuf);
    cmdp->status = sim8xxGetStatus(cmdp->response);
    chMtxUnlock(&simp->rxlock);
  } else {
    chSemSignal(&simp->sync);
    cmdp->status = SIM8XX_TIMEOUT;
  }

  if (simp->reader) {
    chSysLock();
    chThdResumeS(&simp->reader, MSG_OK);
    chSysUnlock();
  }

  chMtxUnlock(&simp->lock);
}

Sim8xxCommandStatus_t sim8xxGetStatus(char *data) {
  size_t length = strlen(data);
  if(length < 2)
    return SIM8XX_INVALID_STATUS;

  if (('\r' != data[length-2]) || ('\n' != data[length-1]))
    return SIM8XX_INVALID_STATUS;

  data[length-2] = '\0';

  char *crlf, *needle;
  for(crlf = needle = data; crlf; crlf = strstr(needle, "\r\n"))
    needle = crlf + strlen("\r\n");   

  Sim8xxCommandStatus_t status;
  
  if (0 == strcmp(needle, "OK"))
    status = SIM8XX_OK;
  else if (0 == strcmp(needle, "CONNECT"))
    status = SIM8XX_CONNECT;
  else if (0 == strcmp(needle, "RING"))
    status = SIM8XX_RING;
  else if (0 == strcmp(needle, "NO CARRIER"))
    status = SIM8XX_NO_CARRIER;
  else if (0 == strcmp(needle, "ERROR"))
    status = SIM8XX_ERROR;
  else if (0 == strcmp(needle, "NO DIALTONE"))
    status = SIM8XX_NO_DIALTONE;
  else if (0 == strcmp(needle, "BUSY"))
    status = SIM8XX_BUSY;
  else if (0 == strcmp(needle, "NO ANSWER"))
    status = SIM8XX_NO_ANSWER;
  else if (0 == strcmp(needle, "PROCEEDING"))
    status = SIM8XX_PROCEEDING;
  else
    status = SIM8XX_INVALID_STATUS;

  data[length-2] = '\r';

  return status;
}

bool sim8xxIsConnected(Sim8xxDriver *simp) {
  chMtxLock(&simp->lock);
  chSemWait(&simp->sync);

  chprintf((BaseSequentialStream*)simp->config->sdp, "at\r");

  chSysLock();
  msg_t msg = chThdSuspendTimeoutS(&simp->writer, chTimeMS2I(1000));
  simp->writer = NULL;
  chSysUnlock();

  bool result = FALSE;
  if (MSG_OK == msg) {
    chMtxLock(&simp->rxlock);
    Sim8xxCommandStatus_t status = sim8xxGetStatus(simp->rxbuf);
    chMtxUnlock(&simp->rxlock);
    result = (SIM8XX_OK == status) ? true : false;
  } else if (MSG_TIMEOUT == msg) {
    result = false;
    chSemSignal(&simp->sync);
  }

  if (simp->reader) {
      chSysLock();  
      chThdResumeS(&simp->reader, MSG_OK);
      chSysUnlock();
  }

  chMtxUnlock(&simp->lock);

  return result;
}

void sim8xxTogglePower(Sim8xxDriver *simp) {
  chMtxLock(&simp->lock);
  palClearLine(simp->config->powerline);
  chThdSleepMilliseconds(2500);
  palSetLine(simp->config->powerline);
  chThdSleepMilliseconds(1000);
  chSemSignal(&simp->sync);
  chMtxUnlock(&simp->lock);
}

/******************************* END OF FILE ***********************************/

