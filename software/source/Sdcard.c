/**
 * @file Sdcard.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "Sdcard.h"

#include "ch.h"
#include "ff.h"
#include "hal.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
/* Filesystem object.*/
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fsReady = FALSE;

/* Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig hs_spicfg = {
    false,
    NULL,
    LINE_SDC_CS,
    0,
    0,
};

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig ls_spicfg = {
    false,
    NULL,
    LINE_SDC_CS,
    SPI_CR1_BR_2 | SPI_CR1_BR_1,
    0,
};

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {
    &SPID2,
    &ls_spicfg,
    &hs_spicfg,
};

/* Generic large buffer.*/
static char fbuff[1024];

MMCDriver MMCD1;

static mutex_t sdcardMutex;

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static FRESULT SDC_scanFiles(BaseSequentialStream *chp, char *path)
{
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
        res         = SDC_scanFiles(chp, path);
        *(path + i) = '\0';
        if (res != FR_OK)
          break;
      } else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void SDC_Init(void)
{
  mmcObjectInit(&MMCD1);
  chMtxObjectInit(&sdcardMutex);
}

void SDC_Lock(void)
{
  chMtxLock(&sdcardMutex);
}

void SDC_Unlock(void)
{
  chMtxUnlock(&sdcardMutex);
}

bool SDC_Mount(void)
{
  SDC_Lock();

  if (!fsReady) {
    
    mmcStart(&MMCD1, &mmccfg);

    if (HAL_SUCCESS == mmcConnect(&MMCD1)) {
      if (FR_OK == f_mount(&SDC_FS, "/", 1)) {
        fsReady = true;
      } else {
        mmcDisconnect(&MMCD1);
        mmcStop(&MMCD1);
        fsReady = false;
      }
    } else {
      fsReady = false;
    }
  }

  SDC_Unlock();

  return fsReady;
}

bool SDC_Unmount(void)
{
  SDC_Lock();

  if (fsReady) {
    mmcDisconnect(&MMCD1);
    mmcStop(&MMCD1);

    fsReady = FALSE;
  }

  SDC_Unlock();

  return !fsReady;
}

void SDC_Tree(BaseSequentialStream *chp, int argc, char *argv[])
{
  SDC_Lock();
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)argv;

  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
    SDC_Unlock();
    return;
  }
  if (!fsReady) {
    chprintf(chp, "File System not mounted\r\n");
    SDC_Unlock();
    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed\r\n");
    SDC_Unlock();
    return;
  }
  chprintf(chp,
           "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
           clusters,
           (uint32_t)SDC_FS.csize,
           clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
  fbuff[0] = 0;
  SDC_scanFiles(chp, (char *)fbuff);
  SDC_Unlock();
}

/****************************** END OF FILE **********************************/
