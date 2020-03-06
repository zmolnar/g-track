/**
 * @file Config.h
 * @brief
 */

#ifndef CONFIG_H
#define CONFIG_H

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include <stdint.h>

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

#define CFG_GPRS_SECTION "gprs"
#define CFG_GPRS_APN "apn"
#define CFG_GPRS_APN_DEFAULT ""

#define CFG_SIM_SECTION "sim"
#define CFG_SIM_PIN "pin"
#define CFG_SIM_PIN_DEFAULT "0000"

#define CFG_GPS_SECTION "gps"
#define CFG_GPS_TIMEZONE "timezone"

#define CFG_MOTION_DETECTOR_SECTION "motiondetector"
#define CFG_MOTION_DETECTOR_THRESHOLD "threshold"

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef struct BluetoothConfig_s {
  char hostname[20];
  char pin[5];
} BluetoothConfig_t;

typedef struct GprsConfig_s {
  char apn[50];
} GprsConfig_t;

typedef struct SimConfig_s {
  char pin[5];
} SimConfig_t;

typedef struct GpsConfig_s {
  int8_t utcOffset;
} GpsConfig_t;

typedef struct MotionDetectorConfig_s {
  uint8_t threshold;
} MotionDetectorConfig_t;

typedef struct Config_s {
  BluetoothConfig_t bluetooth;
  GprsConfig_t gprs;
  SimConfig_t sim;
  GpsConfig_t gps;
  MotionDetectorConfig_t motionDetector;
} Config_t;

/*****************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                           */
/*****************************************************************************/

/*****************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                           */
/*****************************************************************************/
void CFG_Init(Config_t *cfg);

#endif /* CONFIG_H */

/****************************** END OF FILE **********************************/
