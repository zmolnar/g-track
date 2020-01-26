/**
 * @file ConfigManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ConfigManagerThread.h"
#include "minIni.h"
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
static void CFM_LoadDefaultConfig(void)
{
  memcpy(&configManager.config, &defaultConfig, sizeof(Config_t));
}

static void CFM_Read(void)
{
  ini_gets(CFG_BLUETOOTH_SECTION,
           CFG_BLUETOOTH_HOSTNAME,
           CFG_BLUETOOTH_HOSTNAME_DEFAULT,
           configManager.config.bluetooth.hostname,
           sizeof(configManager.config.bluetooth.hostname),
           CFG_INI_FILE);

  ini_gets(CFG_BLUETOOTH_SECTION,
           CFG_BLUETOOTH_PIN,
           CFG_BLUETOOTH_PIN_DEFAULT,
           configManager.config.bluetooth.pin,
           sizeof(configManager.config.bluetooth.pin),
           CFG_INI_FILE);
}

static void CFM_Clear(void)
{
  CFM_LoadDefaultConfig();
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(CFM_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("config-manager");

  CFM_LoadDefaultConfig();

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&configManager.mailbox, &msg, TIME_INFINITE)) {
      CFM_Command_t cmd = (CFM_Command_t)msg;
      switch (cmd) {
      case CFM_CMD_READ: {
        CFM_Read();
        break;
      }
      case CFM_CMD_CLEAR: {
        CFM_Clear();
        break;
      }
      case CFM_CMD_INVALID:
      default: {
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

void CFM_ReadConfigI(void)
{
  chMBPostI(&configManager.mailbox, CFM_CMD_READ);
}

void CFM_ReadConfig(void) 
{
  chSysLock();
  CFM_ReadConfigI();
  chSysUnlock();
}

void CFM_ClearConfigI(void)
{
  chMBPostI(&configManager.mailbox, CFM_CMD_CLEAR);
}

void CFM_ClearConfig(void) 
{
  chSysLock();
  CFM_ClearConfigI();
  chSysUnlock();
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
