/**
 * @file ConfigManagerThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "ConfigManagerThread.h"
#include "SystemThread.h"
#include "Sdcard.h"
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
  semaphore_t configRead;
  bool isInitialized;
} ConfigManager_t;

typedef enum {
  CFM_CMD_INVALID,
  CFM_CMD_LOAD,
  CFM_CMD_STORE,
} CFM_Command_t;

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
static void CFM_LoadIniFile(void)
{
  SDC_Lock();

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

  ini_gets(CFG_GPRS_SECTION,
          CFG_GPRS_APN,
          CFG_GPRS_APN_DEFAULT, configManager.config.gprs.apn,
          sizeof(configManager.config.gprs.apn),
          CFG_INI_FILE);

  ini_gets(CFG_SIM_SECTION,
           CFG_SIM_PIN,
           CFG_SIM_PIN_DEFAULT,
           configManager.config.sim.pin,
           sizeof(configManager.config.sim.pin),
           CFG_INI_FILE);

  SDC_Unlock();        
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
        CFM_LoadIniFile();
        if (!configManager.isInitialized) {
          chSemSignal(&configManager.configRead);
          configManager.isInitialized = true;
        }
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
  memset(&configManager.config, 0, sizeof(configManager.config));
  memset(configManager.commands, 0, sizeof(configManager.commands));
  chMBObjectInit(&configManager.mailbox, configManager.commands, ARRAY_LENGTH(configManager.commands));
  chSemObjectInit(&configManager.configRead, 0);
  configManager.isInitialized = false;
}

void CFM_WaitForValidConfig(void)
{
  while (MSG_OK != chSemWait(&configManager.configRead))
    ;

  chSemSignal(&configManager.configRead);  
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

const BluetoothConfig_t *CFM_GetBluetoothConfig(void)
{
  return &configManager.config.bluetooth;
}

const GprsConfig_t * CFM_GetGprsConfig(void)
{
  return &configManager.config.gprs;
}

/****************************** END OF FILE **********************************/
