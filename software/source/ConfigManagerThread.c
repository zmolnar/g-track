/**
 * @file ConfigManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ConfigManagerThread.h"
#include "SystemThread.h"
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
  CFM_CMD_LOAD,
  CFM_CMD_STORE,
} CFM_Command_t;

typedef enum {
  CONFIG_LOADED = (1 << 0),
} ConfigEvent_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
static ConfigManager_t configManager;

EVENTSOURCE_DECL(ConfigEventFactory);

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

static bool CFM_LoadIniFile(void)
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

  return true;         
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(CFM_Thread, arg)
{
  (void)arg;
  chRegSetThreadName("config-manager");

  SYS_WaitForSuccessfulInit();

  while (true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&configManager.mailbox, &msg, TIME_INFINITE)) {
      CFM_Command_t cmd = (CFM_Command_t)msg;
      switch (cmd) {
      case CFM_CMD_LOAD: {
        if (CFM_LoadIniFile())
          chEvtBroadcastFlags(&ConfigEventFactory, CONFIG_LOADED);
        break;
      }
      case CFM_CMD_STORE: {
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

void CFM_WaitForValidConfig(void)
{
  event_listener_t listener;
  chEvtRegisterMaskWithFlags(&ConfigEventFactory, &listener, EVENT_MASK(0), SYSTEM_INITIALIZED);

  bool isinitialized = false;
  while (!isinitialized) {
    eventmask_t emask = chEvtWaitAny(ALL_EVENTS);
    if (emask & EVENT_MASK(0)) {
      eventflags_t flags = chEvtGetAndClearFlags(&listener);
      if (SYSTEM_INITIALIZED & flags)
        isinitialized = true;
    }
  }
}

void CFM_LoadConfigI(void)
{
  chMBPostI(&configManager.mailbox, CFM_CMD_LOAD);
}

void CFM_LoadConfig(void) 
{
  chSysLock();
  CFM_LoadConfigI();
  chSysUnlock();
}

void CFM_StoreConfigI(void)
{
  chMBPostI(&configManager.mailbox, CFM_CMD_STORE);
}

void CFM_StoreConfig(void) 
{
  chSysLock();
  CFM_StoreConfigI();
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
