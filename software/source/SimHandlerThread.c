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
  chSemObjectInit(&simHandler.parserSync, 0);
  simHandler.state = SHD_STATE_INIT;

  static Sim8xxConfig_t simConfig = {
      .put = SHD_serialPut,
  };

  SIM_Init(&SIM868, &simConfig);
}

bool SHD_ConnectModem(void)
{
  bool result = false;

  if (SHD_STATE_CONNECTED != simHandler.state) {
    sdStart(&SD1, &sdConfig);

    bool isAlive = SIM_IsAlive(&SIM868);

    if (!isAlive) {
      SHD_PowerPulse();
      isAlive = SIM_IsAlive(&SIM868);
    }

    if (isAlive) {
      if (SIM_Start(&SIM868)) {
        simHandler.state = SHD_STATE_CONNECTED;
        result = true;
      }
      else 
        simHandler.state = SHD_STATE_ERROR;
    } else {
      simHandler.state = SHD_STATE_ERROR;
    }
  } else {
    result = true;
  }

  return result;
}

bool SHD_DisconnectModem(void)
{
  bool result = false;

  if (SHD_STATE_DISCONNECTED != simHandler.state) {
    bool isAlive = SIM_IsAlive(&SIM868);

    if (isAlive) {
      SHD_PowerPulse();
      isAlive = SIM_IsAlive(&SIM868);
    }

    if (!isAlive) {
      if (SIM_Stop(&SIM868)) {
        simHandler.state = SHD_STATE_DISCONNECTED;
        result     = true;
      } else
        simHandler.state = SHD_STATE_ERROR;
    } else {
      simHandler.state = SHD_STATE_ERROR;
    }

  } else {
    result = true;
  }

  return result;
}

THD_FUNCTION(SimReaderThread, arg)
{
  (void)arg;
  chRegSetThreadName("simreader");

  while (true) {
    msg_t c = sdGet(&SD1);
    SIM_ProcessChar(&SIM868, (char)c);

    do {
      c = sdGetTimeout(&SD1, chTimeMS2I(10));
      SIM_ProcessChar(&SIM868, (char)c);
    } while (c != MSG_TIMEOUT);

    chSemSignal(&simHandler.parserSync);
  }
}

THD_FUNCTION(SimParserThread, arg)
{
  (void)arg;
  chRegSetThreadName("simparser");

  while (true) {
    chSemWait(&simHandler.parserSync);
    SIM_Parse(&SIM868);
  }
}

/****************************** END OF FILE **********************************/
