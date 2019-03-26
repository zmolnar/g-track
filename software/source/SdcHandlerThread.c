/**
 * @file SdcHandlerThread.c
 * @brief Thread that handles SD-card events.
 * @author Molnar Zoltan
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "SdcHandlerThread.h"

#include "chprintf.h"
#include <string.h>

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                                */
/*******************************************************************************/
event_source_t sdc_inserted_event;
event_source_t sdc_removed_event;

/* Filesystem object.*/
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fs_ready = FALSE;

/* Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig hs_spicfg = {false,NULL,
                              LINE_SDC_CS,
                              0,
                              0};

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig ls_spicfg = {false,NULL,
                              LINE_SDC_CS,
                              SPI_CR1_BR_2 | SPI_CR1_BR_1,
                              0};

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {&SPID2, &ls_spicfg, &hs_spicfg};

/* Generic large buffer.*/
static uint8_t fbuff[1024];

MMCDriver MMCD1;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static void sdc_insert_handler(eventid_t id) {
  FRESULT err;

  (void)id;

  if (mmcConnect(&MMCD1))
    return;

  err = f_mount(&SDC_FS, "/", 1);
  if (err != FR_OK) {
    mmcDisconnect(&MMCD1);
    return;
  }

  fs_ready = TRUE;
  palClearLine(LINE_LED_2_RED);
}

static void sdc_remove_handler(eventid_t id) {

  (void)id;

  mmcDisconnect(&MMCD1);
  fs_ready = FALSE;

  palSetLine(LINE_LED_2_RED);
}

static FRESULT scan_files(BaseSequentialStream *chp, char *path) {
  static FILINFO fno;
  FRESULT res;
  DIR dir;
  size_t i;
  char *fn;

  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    while (((res = f_readdir(&dir, &fno)) == FR_OK) && fno.fname[0]) {
      if (FF_FS_RPATH && fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        *(path + i) = '/';
        strcpy(path + i + 1, fn);
        res = scan_files(chp, path);
        *(path + i) = '\0';
        if (res != FR_OK)
          break;
      }
      else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

/*******************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                              */
/*******************************************************************************/
void SdcCmdTree(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)argv;
  
  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed\r\n");
    return;
  }
  chprintf(chp,
           "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
           clusters, (uint32_t)SDC_FS.csize,
           clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
  fbuff[0] = 0;
  scan_files(chp, (char *)fbuff);
}

THD_FUNCTION(SdcHandlerThread, arg) {
  (void)arg;
  chRegSetThreadName("sdchandler");
  
  static const evhandler_t eventHandlers[] = {
    sdc_insert_handler,
    sdc_remove_handler
  };
  event_listener_t sdc_insert_listener;
  event_listener_t sdc_remove_listener;

  mmcObjectInit(&MMCD1);
  mmcStart(&MMCD1, &mmccfg);

  chEvtObjectInit(&sdc_inserted_event);
  chEvtObjectInit(&sdc_removed_event);
  
  chEvtRegister(&sdc_inserted_event, &sdc_insert_listener, 0);
  chEvtRegister(&sdc_removed_event, &sdc_remove_listener, 1);

  while(true) {
    chEvtDispatch(eventHandlers, chEvtWaitOneTimeout(ALL_EVENTS, TIME_MS2I(500)));
  }
}

/******************************* END OF FILE ***********************************/

