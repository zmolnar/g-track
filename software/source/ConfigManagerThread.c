/**
 * @file ConfigManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ConfigManagerThread.h"
#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof((a)[0]))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct ConfigManager_s {
  Config_t config;
  msg_t commands[10];
  mailbox_t mailbox;
} ConfigManager_t;

typedef enum {
  CFM_CMD_INVALID,
  CFM_CMD_READ,
  CFM_CMD_CLEAR,
} CFM_Command_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static ConfigManager_t configManager;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static void CFM_ReadConfig(void)
{

}

static void CFM_ClearConfig(void)
{
  memcpy(&configManager.config, &defaultConfig, sizeof(Config_t));
}
/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(CFM_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("config-manager");

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&configManager.mailbox, &msg, TIME_INFINITE)) {
      CFM_Command_t cmd = (CFM_Command_t)msg;
      switch (cmd) {
        case CFM_CMD_READ: {
          CFM_ReadConfig();
          break;
        }
        case CFM_CMD_CLEAR: {
          CFM_ClearConfig();
          break;
        }
        case CFM_CMD_INVALID :
        default : {
          break;
        }
      }
    }
  }
}

void CFM_Init(void)
{
  memcpy(&configManager.config, &defaultConfig, sizeof(Config_t));
  chMBObjectInit(
      &configManager.mailbox, configManager.commands, ARRAY_LENGTH(configManager.commands));
}

const char *CFM_GetBluetoothHostName(void)
{
  return configManager.config.bluetooth.hostname;
}

const char *CFM_GetBluetoothPin(void)
{
  return configManager.config.bluetooth.pin;
}

/****************************** END OF FILE **********************************/
