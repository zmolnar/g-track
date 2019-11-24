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
  chMtxObjectInit(&simp->lock);
  chMtxObjectInit(&simp->rxlock);
  chSemObjectInit(&simp->guardSync, 0);
  chSemObjectInit(&simp->atSync, 0);
  chSemObjectInit(&simp->urcSync, 0);
  memset(simp->rxbuf, 0, sizeof(simp->rxbuf));
  simp->rxlength = 0;
  SIM_ParserReset(&simp->parser);
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
    SIM_ParserGetAtMessage(&simp->parser, cmdp->response, sizeof(cmdp->response));  
    cmdp->status = SIM_ParserGetStatus(&simp->parser);
    chMtxUnlock(&simp->rxlock);
    chSemSignal(&simp->atSync);
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
      size_t buflength = sizeof(cmdp->response) - strlen(cmdp->response);
      if (buflength) {
        chMtxLock(&simp->rxlock);
        char *buf = cmdp->response + strlen(cmdp->response);
        size_t length = sizeof(cmdp->response) - strlen(cmdp->response);
        SIM_ParserGetAtMessage(&simp->parser, buf, length);
        cmdp->status = SIM_ParserGetStatus(&simp->parser);
        chMtxUnlock(&simp->rxlock);
        chSemSignal(&simp->atSync);
      } else {
        cmdp->status = SIM8XX_BUFFER_OVERFLOW;
      }
    } else {
      cmdp->status = SIM8XX_TIMEOUT;
    }
  }

  SIM_startGuardTimer(simp);
  chMtxUnlock(&simp->lock);
}

size_t SIM_GetAndClearUrc(Sim8xxDriver *simp, char *urc, size_t length)
{
  chMtxLock(&simp->rxlock);
  SIM_ParserGetUrc(&simp->parser, urc, length);
  chMtxUnlock(&simp->rxlock);
  chSemSignal(&simp->urcSync);

  return strlen(urc);
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
