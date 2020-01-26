/**
 * @file Config.h
 * @brief
 */

#ifndef CONFIG_H
#define CONFIG_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/
#define CFG_INI_FILE "/gtrack.ini"
#define CFG_BLUETOOTH_SECTION "bluetooth"
#define CFG_BLUETOOTH_HOSTNAME "hostname"
#define CFG_BLUETOOTH_HOSTNAME_DEFAULT "gtrack"
#define CFG_BLUETOOTH_PIN "pin"
#define CFG_BLUETOOTH_PIN_DEFAULT "2020"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct BluetoothConfig_s {
  char hostname[20];
  char pin[5];
} BluetoothConfig_t;

typedef struct Config_s {
  BluetoothConfig_t bluetooth;
} Config_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/
extern const Config_t defaultConfig;

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void CFG_Init(Config_t *cfg);

#endif /* CONFIG_H */

/****************************** END OF FILE **********************************/
