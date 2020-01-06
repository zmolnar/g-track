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
} SimHandler_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Sim8xx_t SIM868;

static SimHandler_t simHandler;

static SerialConfig sdConfig = {
    19200,
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
  sdPut(&SD1, c);
}

static void SHD_PowerPulse(void)
{
  palClearLine(LINE_WAVESHARE_POWER);
  chThdSleepMilliseconds(2500);
  palSetLine(LINE_WAVESHARE_POWER);
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

  static Sim8xxConfig_t simConfig = {
      .put = SHD_serialPut,
  };

  SIM_Init(&SIM868, &simConfig);
}

bool SHD_ConnectModem(void)
{
  if (SHD_STATE_CONNECTED != simHandler.state) {
    sdStart(&SD1, &sdConfig);
    simHandler.parser = chThdCreateFromHeap(
        NULL, PARSER_STACK_SIZE, "simparser", NORMALPRIO + 1, SimParserThread, &SIM868);
    simHandler.reader = chThdCreateFromHeap(
        NULL, READER_STACK_SIZE, "simreader", NORMALPRIO + 1, SimReaderThread, &SIM868);

    bool isAlive = SIM_Start(&SIM868);
    if (!isAlive) {
      SHD_PowerPulse();
      isAlive = SIM_Start(&SIM868);
    }

    if (isAlive)
      simHandler.state = SHD_STATE_CONNECTED;
    else
      simHandler.state = SHD_STATE_ERROR;
  }

  return SHD_STATE_CONNECTED == simHandler.state;
}

bool SHD_DisconnectModem(void)
{
  if (SHD_STATE_DISCONNECTED != simHandler.state) {
    bool isAlive = SIM_IsAlive(&SIM868);

    if (isAlive) {
      SHD_PowerPulse();
      isAlive = SIM_IsAlive(&SIM868);
    }

    if (!isAlive) {
      if (SIM_Stop(&SIM868)) {
        simHandler.state = SHD_STATE_DISCONNECTED;
        if (simHandler.reader) {
          chThdTerminate(simHandler.parser);
          chThdWait(simHandler.parser);
          simHandler.parser = NULL;
        }
        if (simHandler.reader) {
          chThdTerminate(simHandler.reader);
          chThdWait(simHandler.reader);
          simHandler.reader = NULL;
        }
      } else
        simHandler.state = SHD_STATE_ERROR;
    } else {
      simHandler.state = SHD_STATE_ERROR;
    }
  }

  return SHD_STATE_DISCONNECTED == simHandler.state;
}

THD_FUNCTION(SimReaderThread, arg)
{
  Sim8xx_t *modem = (Sim8xx_t *)arg;

  while (!chThdShouldTerminateX()) {
    msg_t c = sdGetTimeout(&SD1, chTimeMS2I(100));

    if ((MSG_TIMEOUT != c) && (MSG_RESET != c)) {
      SIM_ProcessChar(modem, (char)c);

      do {
        c = sdGetTimeout(&SD1, chTimeMS2I(10));
        if (c != MSG_TIMEOUT)
          SIM_ProcessChar(modem, (char)c);
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
