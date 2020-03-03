/**
 * @file SimHandlerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "SimHandlerThread.h"
#include "hal.h"

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define PARSER_STACK_SIZE THD_WORKING_AREA_SIZE(2048)
#define READER_STACK_SIZE THD_WORKING_AREA_SIZE(128)

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  SHD_STATE_INIT,
  SHD_STATE_CONNECTED,
  SHD_STATE_DISCONNECTED,
  SHD_STATE_ERROR,
} SHD_State_t;

typedef struct SimHandler_s {
  SHD_State_t state;
  semaphore_t parserSync;
  thread_reference_t reader;
  thread_reference_t parser;
  semaphore_t pinRequested;
  semaphore_t SIMUnlocked;
} SimHandler_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Sim8xx_t SIM868;

SimHandler_t simHandler;

static SerialConfig sdConfig = {
    115200,
    0,
    USART_CR2_STOP1_BITS,
    0,
};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void SHD_serialPut(char c)
{
  ITM_SendChar(c);
  sdPut(&SD1, c);
}

static void SHD_PowerPulse(void)
{
  palClearLine(LINE_WAVESHARE_POWER);
  chThdSleepMilliseconds(3000);
  palSetLine(LINE_WAVESHARE_POWER);
}

static void SHD_ModemCallback(GSM_ModemEvent_t *event)
{
  switch(event->type) {
    case MODEM_EVENT_SIM_UNLOCKED: {
      chSemSignal(&simHandler.SIMUnlocked);
      break;
    }
    case MODEM_EVENT_CPIN: {
      if (CPIN_PIN_REQUIRED == event->payload.cpin.status) 
        chSemSignal(&simHandler.pinRequested);
      break;
    }
    case MODEM_NO_EVENT:
    default: {
      break;
    }
  }
}

bool SHD_waitRequestForPinCode(void)
{
  bool result = false;
  if (MSG_OK == chSemWaitTimeout(&simHandler.pinRequested, TIME_S2I(10))) {
    chSemSignal(&simHandler.pinRequested);
    result = true;
  }

  return result;
}

bool SHD_waitForSimUnlock(void)
{
  bool result = false;
  if (MSG_OK == chSemWaitTimeout(&simHandler.SIMUnlocked, TIME_S2I(10))) {
    chSemSignal(&simHandler.SIMUnlocked);
    result = true;
  }

  return result;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SHD_Init(void)
{
  palSetLine(LINE_WAVESHARE_POWER);
  simHandler.state = SHD_STATE_INIT;
  chSemObjectInit(&simHandler.parserSync, 0);
  simHandler.reader = NULL;
  simHandler.parser = NULL;
  chSemObjectInit(&simHandler.SIMUnlocked, 0);
  chSemObjectInit(&simHandler.pinRequested, 0);

  static Sim8xxConfig_t simConfig = {
      .put = SHD_serialPut,
  };

  SIM_Init(&SIM868, &simConfig);
  SIM_RegisterModemCallback(&SIM868, SHD_ModemCallback);
  sdStart(&SD1, &sdConfig);
  simHandler.parser = chThdCreateFromHeap(
      NULL, PARSER_STACK_SIZE, "simparser", NORMALPRIO + 1, SimParserThread, &SIM868);
  simHandler.reader = chThdCreateFromHeap(
      NULL, READER_STACK_SIZE, "simreader", NORMALPRIO + 1, SimReaderThread, &SIM868);
}

bool SHD_ResetModem(void)
{
  bool isAlive = SIM_IsAlive(&SIM868);
  if (isAlive) {
    SHD_PowerPulse();
    chThdSleepMilliseconds(1000);
  }

  SHD_PowerPulse();

  isAlive = SIM_IsAlive(&SIM868);
  simHandler.state = isAlive ? SHD_STATE_DISCONNECTED : SHD_STATE_ERROR;

  return isAlive;
}

bool SHD_ResetAndConnectModem(const char *pin)
{
  bool success = false;

  if (SHD_ResetModem()) {
    if (SIM_Start(&SIM868)) {
      if (SHD_waitRequestForPinCode()) {
        if (SIM_UnlockSIMCard(&SIM868, pin)) {
          if (SHD_waitForSimUnlock()) {
            success          = true;
            simHandler.state = SHD_STATE_CONNECTED;
          }
        }
      }
    }
  }

  if (!success)
    simHandler.state = SHD_STATE_ERROR;

  return success;
}

bool SHD_DisconnectModem(void)
{
  simHandler.state = SHD_STATE_DISCONNECTED;
  return true;
}

THD_FUNCTION(SimReaderThread, arg)
{
  Sim8xx_t *modem = (Sim8xx_t *)arg;

  while (!chThdShouldTerminateX()) {
    msg_t c = sdGetTimeout(&SD1, chTimeMS2I(100));

    if ((MSG_TIMEOUT != c) && (MSG_RESET != c)) {
      ITM_SendChar(c);
      SIM_ProcessChar(modem, (char)c);

      do {
        c = sdGetTimeout(&SD1, chTimeMS2I(10));
        if (c != MSG_TIMEOUT) {
          ITM_SendChar(c);
          SIM_ProcessChar(modem, (char)c);
        }
      } while (c != MSG_TIMEOUT);

      chSemSignal(&simHandler.parserSync);
    }
  }

  chThdExit(MSG_OK);
}

THD_FUNCTION(SimParserThread, arg)
{
  Sim8xx_t *modem = (Sim8xx_t *)arg;

  while (!chThdShouldTerminateX()) {
    msg_t c = chSemWaitTimeout(&simHandler.parserSync, chTimeMS2I(100));
    if (MSG_OK == c)
      SIM_Parse(modem);
  }

  chThdExit(MSG_OK);
}

/****************************** END OF FILE **********************************/
