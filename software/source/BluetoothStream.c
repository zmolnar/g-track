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
  BluetoothStream *bsp = ip;

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
  BluetoothStream *bsp = ip;

  chMtxLock(&bsp->rxlock);


  chMtxUnlock(&bsp->rxlock);

  return 0;
}
 
static msg_t put(void *ip, uint8_t b) {
  BluetoothStream *bsp = ip;

  chMtxLock(&bsp->txlock);


  chMtxUnlock(&bsp->txlock);

  return MSG_OK;
}
 
static msg_t get(void *ip) {
  BluetoothStream *bsp = ip;

  chMtxLock(&bsp->rxlock);


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
void BLS_ObjectInit(BluetoothStream *bsp)
{
    bsp->vmt = &vmt;
    memset(bsp->rxbuf, 0, sizeof(bsp->rxbuf));
    memset(bsp->txbuf, 0, sizeof(bsp->txbuf));
    bsp->ibuf = bsp->rxbuf;
    bsp->obuf = bsp->txbuf;
    chSemObjectInit(&bsp->rxsync, 0);
    chSemObjectInit(&bsp->txsync, 0);
    chMtxObjectInit(&bsp->rxlock);
    chMtxObjectInit(&bsp->txlock);
}

/****************************** END OF FILE **********************************/
