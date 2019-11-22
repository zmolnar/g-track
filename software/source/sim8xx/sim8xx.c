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
#define GUARD_TIME_IN_MS 100

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
static void SIM_timerCallback(void *p)
{
  Sim8xxDriver *simp = (Sim8xxDriver *)p;
  chSysLockFromISR();
  chSemResetI(&simp->guardSync, 1);
  chSysUnlockFromISR();
}

static void SIM_startGuardTimer(Sim8xxDriver *simp)
{
  chSysLock();
  chVTResetI(&simp->guardTimer);
  chVTSetI(&simp->guardTimer, TIME_MS2I(GUARD_TIME_IN_MS), SIM_timerCallback, simp);
  chSysUnlock();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SIM_Init(Sim8xxDriver *simp)
{
  simp->state    = SIM8XX_STOP;
  simp->config = NULL;
  simp->writer = NULL;
  simp->reader = NULL;
  simp->urcprocessor = NULL;
  chMtxObjectInit(&simp->lock);
  chMtxObjectInit(&simp->rxlock);
  chSemObjectInit(&simp->guardSync, 0);
  chSemObjectInit(&simp->atSync, 0);
  chSemObjectInit(&simp->urcSync, 0);
  chSemObjectInit(&simp->urcClear, 0);
  memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
  simp->rxlength = 0;
  memset(simp->urcbuf, 0, sizeof(simp->urcbuf));
  simp->at = simp->rxbuf;
  simp->atlength = 0;
  simp->urc = simp->rxbuf;
  simp->urclength = 0;
  simp->processend = 0;
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
    size_t length = simp->atlength;
    if (sizeof(cmdp->response) <= length)
      length = sizeof(cmdp->response) - 1;
    memcpy(cmdp->response, simp->at, length);
    chMtxUnlock(&simp->rxlock);
    chSemSignal(&simp->atSync);
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
      size_t length = simp->atlength;
      size_t buflength = sizeof(cmdp->response) - strlen(cmdp->response);
      if (buflength <= length)
        length = buflength - 1;
      memcpy(cmdp->response + strlen(cmdp->response), simp->at, length);
      chMtxUnlock(&simp->rxlock);
      chSemSignal(&simp->atSync);
      cmdp->status = SIM_GetCommandStatus(cmdp->response);
    } else {
      cmdp->status = SIM8XX_TIMEOUT;
    }
  }

  SIM_startGuardTimer(simp);
  chMtxUnlock(&simp->lock);
}

char *SIM_GetUrcMessage(Sim8xxDriver *simp)
{
  return simp->urcbuf;
}

void SIM_ClearUrcMessage(Sim8xxDriver *simp)
{
  chSemSignal(&simp->urcClear);
}

Sim8xxCommandStatus_t SIM_GetCommandStatus(char *data)
{
  size_t length = strlen(data);

  if (length < strlen(CRLF))
    return SIM8XX_INVALID_STATUS;

  if (('>' == data[length-2]) && (' ' == data[length-1]))
    return SIM8XX_WAITING_FOR_INPUT;

  // Remove last CRLF
  char *needle = strrstr(data, CRLF);
  if (!needle)
    return SIM8XX_INVALID_STATUS;

  char *end = needle;
  *end = '\0';

  // Search CRLF before the status
  needle = strrstr(data, CRLF);
  if (!needle)
    return SIM8XX_INVALID_STATUS;

  needle += strlen(CRLF);

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
  else
    status = SIM8XX_INVALID_STATUS;

  *end = '\0';

  return status;
}

bool SIM_IsConnected(Sim8xxDriver *simp)
{
  Sim8xxCommand cmd;
  SIM_CommandInit(&cmd);
  strncpy(cmd.request, "at", sizeof(cmd.request));
  SIM_ExecuteCommand(simp, &cmd);
  return SIM8XX_OK == cmd.status;
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
