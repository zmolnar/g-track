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

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
Sim8xx_t SIM868;

static SHD_State_t modemState = SHD_STATE_INIT;

static SerialConfig sdConfig = {
    19200,
    0,
    USART_CR2_STOP1_BITS,
    0,
};

static semaphore_t parserSync;

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

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SHD_Init(void)
{
  static Sim8xxConfig_t simConfig = {
      .put = SHD_serialPut,
  };

  modemState = SHD_STATE_INIT;
  SIM_Init(&SIM868, &simConfig);
  chSemObjectInit(&parserSync, 0);
}

bool SHD_ConnectModem(void)
{
  bool result = false;

  if (SHD_STATE_CONNECTED != modemState) {
    sdStart(&SD1, &sdConfig);

    bool isAlive = SIM_IsAlive(&SIM868);

    if (!isAlive) {
      palToggleLine(LINE_WAVESHARE_POWER);
      isAlive = SIM_IsAlive(&SIM868);
    }

    if (isAlive) {
      if (SIM_Start(&SIM868) {
        modemState = SHD_STATE_CONNECTED;
        success = true;
      }
      else 
        modemState = SHD_STATE_ERROR;
    } else {
      modemState = SHD_STATE_ERROR;
    }
  } else {
    result = true;
  }

  return result;
}

bool SHD_DisconnectModem(void)
{
  bool result = false;

  if (SHD_STATE_DISCONNECTED != modemState) {
    bool isAlive = SIM_IsAlive(&SIM868);

    if (isAlive) {
      palToggleLine(LINE_WAVESHARE_POWER);
      isAlive = SIM_IsAlive(&SIM868);
    }

    if (!isAlive) {
      if (SIM_Stop(&SIM868)) {
        modemState = SHD_STATE_DISCONNECTED;
        result    = true;
      } else
        modemState = SHD_STATE_ERROR;
    } else {
      modemState = SHD_STATE_ERROR;
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
    char c = sdGet(&SD1);
    SIM_ProcessChar(&SIM868, c);

    do {
      c = sdGetTimeout(&SD1, chTimeMS2I(10));
      SIM_ProcessChar(&SIM868, c);
    } while (c != MSG_TIMEOUT);

    chSemSignal(&parserSync);
  }
}

THD_FUNCTION(SimParserThread, arg)
{
  (void)arg;
  chRegSetThreadName("simparser");

  while (true) {
    chSemWait(&parserSync);
    SIM_Parse(&SIM868);
  }
}

/****************************** END OF FILE **********************************/
