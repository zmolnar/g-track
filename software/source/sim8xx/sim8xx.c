/**
 * @file sim8xx.c
 * @brief SIM8xx modem driver.
 * @author Molnar Zoltan
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "sim8xx.h"

#include "chprintf.h"
#include "sim8xxReaderThread.h"
#include "sim8xxUrcThread.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define READER_WA_SIZE THD_WORKING_AREA_SIZE(2048)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Sim8xxDriver SIM8D1;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SIM_Init(Sim8xxDriver *simp)
{
  simp->config = NULL;
  simp->writer = NULL;
  simp->reader = NULL;
  simp->urcprocessor = NULL;
  chMtxObjectInit(&simp->lock);
  chMtxObjectInit(&simp->rxlock);
  chSemObjectInit(&simp->guardSync, 0);
  chSemObjectInit(&simp->atSync, 0);
  chSemObjectInit(&simp->urcSync, 0);
  memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
  simp->rxlength = 0;
  memset(simp->urcbuf, 0, sizeof(simp->urcbuf));
  simp->urclength = 0;
  chSemObjectInit(&simp->urcsema, 0);
  simp->state    = SIM8XX_STOP;
}

void SIM_Start(Sim8xxDriver *simp, Sim8xxConfig *cfgp)
{
  chMtxLock(&simp->lock);
  simp->config = cfgp;
  sdStart(cfgp->sdp, cfgp->sdConfig);
  simp->reader = NULL;
  chThdCreateFromHeap(NULL,
                      READER_WA_SIZE,
                      "sim8xx",
                      NORMALPRIO + 1,
                      SIM_ReaderThread,
                      (void *)simp);
  simp->urcprocessor = NULL;
  chThdCreateFromHeap(NULL, 
                      URCPROCESSOR_WA_SIZE, 
                      "sim8xx-urc", 
                      NORMALPRIO + 1, 
                      SIM_UrcThread, 
                      (void *)simp);
  simp->state = SIM8XX_READY;
  chSemSignal(&simp->guardSync);
  chMtxUnlock(&simp->lock);
}

void SIM_CommandInit(Sim8xxCommand *cmdp)
{
  memset(cmdp, 0, sizeof(Sim8xxCommand));
  cmdp->status = SIM8XX_INVALID_STATUS;
}

void SIM_ExecuteCommand(Sim8xxDriver *simp, Sim8xxCommand *cmdp)
{
  chMtxLock(&simp->lock);
  chSemWait(&simp->guardSync);

  chprintf((BaseSequentialStream *)simp->config->sdp, "%s\r", cmdp->request);

  chSysLock();
  msg_t msg    = chThdSuspendTimeoutS(&simp->writer, chTimeS2I(5));
  simp->writer = NULL;
  chSysUnlock();

  if (MSG_OK == msg) {
    chMtxLock(&simp->rxlock);
    strcpy(cmdp->response, simp->rxbuf);
    chSemSignal(&simp->atSync);
    chMtxUnlock(&simp->rxlock);
    cmdp->status = SIM_GetCommandStatus(cmdp->response);
  } else {
    cmdp->status = SIM8XX_TIMEOUT;
  }

  if (SIM8XX_WAITING_FOR_INPUT == cmdp->status) {
    chprintf((BaseSequentialStream *)simp->config->sdp, "%s\x1A", cmdp->data);

    chSysLock();
    msg_t msg    = chThdSuspendTimeoutS(&simp->writer, chTimeS2I(5));
    simp->writer = NULL;
    chSysUnlock();

    if (MSG_OK == msg) {
      chMtxLock(&simp->rxlock);
      strcpy(cmdp->response, simp->rxbuf);
      chSemSignal(&simp->atSync);
      chMtxUnlock(&simp->rxlock);
      cmdp->status = SIM_GetCommandStatus(cmdp->response);
    } else {
      cmdp->status = SIM8XX_TIMEOUT;
    }
  }

  chMtxUnlock(&simp->lock);
}

char *SIM_GetUrcMessage(Sim8xxDriver *simp)
{
  return simp->urcbuf;
}

void SIM_ClearUrcMessage(Sim8xxDriver *simp)
{
  chSemSignal(&simp->urcsema);
}

Sim8xxCommandStatus_t SIM_GetCommandStatus(char *data)
{
  size_t length = strlen(data);
  if (length < 2)
    return SIM8XX_INVALID_STATUS;

  if (('>' == data[length-2]) && (' ' == data[length-1]))
    return SIM8XX_WAITING_FOR_INPUT;

  char *needle = strrchr(data, '\n');
  if (!needle)
    return SIM8XX_INVALID_STATUS;

  ++needle;

  Sim8xxCommandStatus_t status;

  if (0 == strcmp(needle, "OK"))
    status = SIM8XX_OK;
  else if (0 == strcmp(needle, "CONNECT"))
    status = SIM8XX_CONNECT;
  else if (0 == strcmp(needle, "SEND OK"))
    status = SIM8XX_SEND_OK;
  else if (0 == strcmp(needle, "SEND FAIL"))
    status = SIM8XX_SEND_FAIL;
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
  else if (0 == strcmp(needle, "> "))
    status = SIM8XX_WAITING_FOR_INPUT;
  else
    status = SIM8XX_INVALID_STATUS;

  return status;
}

bool SIM_IsConnected(Sim8xxDriver *simp)
{
  chMtxLock(&simp->lock);
  chSemWait(&simp->guardSync);

  chprintf((BaseSequentialStream *)simp->config->sdp, "at\r");

  chSysLock();
  msg_t msg    = chThdSuspendTimeoutS(&simp->writer, chTimeMS2I(1000));
  simp->writer = NULL;
  chSysUnlock();

  bool result = FALSE;
  if (MSG_OK == msg) {
    chMtxLock(&simp->rxlock);
    Sim8xxCommandStatus_t status = SIM_GetCommandStatus(simp->rxbuf);
    chMtxUnlock(&simp->rxlock);
    chSemSignal(&simp->atSync);
    result = (SIM8XX_OK == status) ? true : false;
  } else if (MSG_TIMEOUT == msg) {
    result = false;
    chSemSignal(&simp->guardSync);
  }
#if 0
  if (simp->reader) {
    chSysLock();
    chThdResumeS(&simp->reader, MSG_OK);
    chSysUnlock();
  }
#endif
  chMtxUnlock(&simp->lock);

  return result;
}

void SIM_TogglePower(Sim8xxDriver *simp)
{
  chMtxLock(&simp->lock);
  palClearLine(simp->config->powerline);
  chThdSleepMilliseconds(2500);
  palSetLine(simp->config->powerline);
  chThdSleepMilliseconds(1000);
  chMtxUnlock(&simp->lock);
}

/****************************** END OF FILE **********************************/
