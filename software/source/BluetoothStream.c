/**
 * @file BluetoothStream.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "BluetoothStream.h"
#include "BluetoothManagerThread.h"
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

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
static size_t writes(void *ip, const uint8_t *bp, size_t n) {
  (void)n;
  BluetoothStream_t *bsp = (BluetoothStream_t *)ip;

  chMtxLock(&bsp->txlock);

  bsp->obuf = bp;
  BLT_SendStreamData();
  chSemWait(&bsp->txsync);
  bsp->obuf = bsp->txbuf;

  chMtxUnlock(&bsp->txlock);

  return strlen(bp);
}

static size_t reads(void *ip, uint8_t *bp, size_t n)
{
  BluetoothStream_t *bsp = ip;

  chMtxLock(&bsp->rxlock);

  if (0 == bsp->rxlength) {
    chSysLock();
    msg_t msg = chThdSuspendTimeoutS(&bsp->reader, TIME_INFINITE);
    bsp->reader = NULL;
    chSysUnlock();
  } 
  
  size_t datalength = bsp->rxlength - bsp->rdindex;
  size_t length = (n < datalength) ? n : datalength;
  memcpy(bp, &bsp->rxbuf[bsp->rdindex], length);


  // Clear rxbuffer
  memset(bsp->rxbuf, 0 sizeof(bsp->rxbuf));
  bsp->rxlength = 0;
  bsp->rdindex = 0;

  chMtxUnlock(&bsp->rxlock);

  return length;
}
 
static msg_t put(void *ip, uint8_t b) {
  BluetoothStream_t *bsp = ip;

  chMtxLock(&bsp->txlock);


  chMtxUnlock(&bsp->txlock);

  return MSG_OK;
}
 
static msg_t get(void *ip) {
  BluetoothStream_t *bsp = ip;

  chMtxLock(&bsp->rxlock);

  if (0 == bsp->rxlength) {
    chSysLock();
    msg_t msg = chThdSuspendTimeoutS(&bsp->reader, TIME_INFINITE);
    bsp->reader = NULL;
    chSysUnlock();
  } 
  
  uint8_t data = bsp->rxbuf[bsp->rdindex];
  bsp->rdindex++;

  if (bsp->rdindex == bsp->rxlength) {
    // Clear rxbuffer
    memset(bsp->rxbuf, 0 sizeof(bsp->rxbuf));
    bsp->rxlength = 0;
    bsp->rdindex = 0;
  }

  chMtxUnlock(&bsp->rxlock);

  return MSG_OK;
}

static const struct BluetoothStreamVMT vmt = {
    .write = writes,
    .read = reads,
    .put = put,
    .get = get,
};

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
void BLS_ObjectInit(BluetoothStream_t *bsp)
{
    bsp->vmt = &vmt;
    memset(bsp->rxbuf, 0, sizeof(bsp->rxbuf));
    memset(bsp->txbuf, 0, sizeof(bsp->txbuf));
    bsp->ibuf = bsp->rxbuf;
    bsp->obuf = bsp->txbuf;
    bsp->rxlength = 0;
    bsp->txlength = 0;
    chSemObjectInit(&bsp->rxsync, 0);
    chSemObjectInit(&bsp->txsync, 0);
    chMtxObjectInit(&bsp->rxlock);
    chMtxObjectInit(&bsp->txlock);
    bsp->reader = NULL;
}

void BLS_ProcessRxData(BluetoothStream_t *bsp, const char *data, size_t length)
{
  uint8_t *buf = bsp->rxbuf + bsp->rxlength;
  size_t buflength = sizeof(bsp->rxbuf) - bsp->rxlength;

  size_t datalength = (length < buflength) ? length : buflength;

  memcpy(buf, data, datalength);
  bsp->rxlength += datalength;

  if (bsp->reader) {
    chThdResume(&bsp->reader, MSG_OK);
  }
}

/****************************** END OF FILE **********************************/
