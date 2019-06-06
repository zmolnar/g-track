/**
 * @file Sdcard.c
 * @brief
 */

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/
#include "Sdcard.h"
#include "ch.h"
#include "hal.h"
#include "ff.h"
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
/* Filesystem object.*/
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fsReady = FALSE;

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

static mutex_t sdcardMutex;

/*******************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                              */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                               */
/*******************************************************************************/
static FRESULT scanFiles(BaseSequentialStream *chp, char *path) {
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
        res = scanFiles(chp, path);
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
void sdcardInit(void) {
  mmcObjectInit(&MMCD1);
  chMtxObjectInit(&sdcardMutex);
}

void sdcardLock(void) {
  chMtxLock(&sdcardMutex);
}

void sdcardUnlock(void) {
  chMtxUnlock(&sdcardMutex);
}

void sdcardMount(void) {
  sdcardLock();
  if (!fsReady) {
    mmcStart(&MMCD1, &mmccfg);
  
    if (mmcConnect(&MMCD1)) 
      return;

    if (FR_OK != f_mount(&SDC_FS, "/", 1)) {
      mmcDisconnect(&MMCD1);
      mmcStop(&MMCD1);
      return;
    }

    fsReady = TRUE;
    palClearLine(LINE_LED_2_RED);
  }

  sdcardUnlock();
}

void sdcardUnmount(void) {
  sdcardLock();
  
  if (fsReady) {
    mmcDisconnect(&MMCD1);
    mmcStop(&MMCD1);
  
    fsReady = FALSE;
    palSetLine(LINE_LED_2_RED);
  }

  sdcardUnlock();
}

void sdcardCmdTree(BaseSequentialStream *chp, int argc, char *argv[]) {
  sdcardLock();
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)argv;
  
  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
    sdcardUnlock();
    return;
  }
  if (!fsReady) {
    chprintf(chp, "File System not mounted\r\n");    
    sdcardUnlock();
    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed\r\n");
    sdcardUnlock();
    return;
  }
  chprintf(chp,
           "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
           clusters, (uint32_t)SDC_FS.csize,
           clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
  fbuff[0] = 0;
  scanFiles(chp, (char *)fbuff);
  sdcardUnlock();
}

/******************************* END OF FILE ***********************************/
