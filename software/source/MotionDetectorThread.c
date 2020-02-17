/**
 * @file MotionDetectorThread.c
 * @brief
 */

/*****************************************************************************/
/* INCLUDES                                                                  */
/*****************************************************************************/
#include "hal.h"

#include "MotionDetectorThread.h"
#include "lis3dsh.h"

#include <string.h>

/*****************************************************************************/
/* DEFINED CONSTANTS                                                         */
/*****************************************************************************/
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
typedef enum {
  MDT_STATE_INIT,
  MDT_STATE_ENABLED,
  MDT_STATE_DISABLED,
} MDT_State_t;

typedef enum {
  MDT_CMD_START,
  MDT_CMD_STOP,
  MDT_CMD_LISTEN,
} MDT_Command_t;

typedef struct MotionDetector_s {
  MDT_State_t state;
  msg_t commands[10];
  mailbox_t mailbox; 
  LIS3DSHDriver lis3dsh; 
} MotionDetector_t;

/*****************************************************************************/
/* MACRO DEFINITIONS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF GLOBAL CONSTANTS AND VARIABLES                              */
/*****************************************************************************/
MotionDetector_t detector;

static SPIConfig lis3dshSpiConfig = {
    .circular = false,
    .end_cb   = NULL,
    .ssline   = LINE_ACC_CS,
    .cr1      = SPI_CR1_BR_1 | SPI_CR1_BR_0 | SPI_CR1_CPOL | SPI_CR1_CPHA,
    .cr2      = 0,
};

static LIS3DSHConfig lis3dshConfig = {
    .spip              = &SPID1,
    .spicfg            = &lis3dshSpiConfig,
    .accsensitivity    = NULL,
    .accbias           = NULL,
    .accfullscale      = LIS3DSH_ACC_FS_2G,
    .accoutputdatarate = LIS3DSH_ACC_ODR_50HZ,
};

static PWMConfig beepConfig = {
    .frequency = 1000000,
    .period    = 1000,
    .callback  = NULL,
    .channels =
        {
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_ACTIVE_HIGH, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
        },
    .cr2  = 0,
    .dier = 0,
};

/*****************************************************************************/
/* DECLARATION OF LOCAL FUNCTIONS                                            */
/*****************************************************************************/

/*****************************************************************************/
/* DEFINITION OF LOCAL FUNCTIONS                                             */
/*****************************************************************************/
void MDT_writeReg(uint8_t address, uint8_t data)
{
  lis3dshSPIWriteRegister(detector.lis3dsh.config->spip, address, 1, &data); 
}

uint8_t MDT_readReg(uint8_t address)
{
  uint8_t r = 0;
  lis3dshSPIReadRegister(detector.lis3dsh.config->spip, address, 1, &r);
  return r;
}

void MDT_interrupt(void *arg)
{
  (void)arg;

  chSysLockFromISR();
  MDT_ListenI();
  chSysUnlockFromISR();
}

#if 0
void MDT_configureInterrupt_1(void)
{
  palSetLineMode(LINE_ACC_INT1, PAL_MODE_INPUT_PULLUP);
  palSetLineCallback(LINE_ACC_INT1, MDT_interrupt, NULL);
  palEnableLineEvent(LINE_ACC_INT1, PAL_EVENT_MODE_FALLING_EDGE);
}

void MDT_loadStateMachine_1(void)
{
  MDT_writeReg(LIS3DSH_AD_CTRL_REG1, LIS3DSH_CTRL_REG1_SM1_EN);
  MDT_writeReg(LIS3DSH_AD_CTRL_REG3, LIS3DSH_CTRL_REG3_INT1_EN);

  MDT_writeReg(LIS3DSH_AD_THRS1_1, 0x45);
  MDT_writeReg(LIS3DSH_AD_ST1_1, 0x05);
  MDT_writeReg(LIS3DSH_AD_ST1_2, 0x11);
  MDT_writeReg(LIS3DSH_AD_MASK1_B, 0xfc);
  MDT_writeReg(LIS3DSH_AD_MASK1_A, 0xfc);
  MDT_writeReg(LIS3DSH_AD_SETT1, 0x01);
}
#endif

void MDT_configureInterrupt_2(void)
{
  palSetLineMode(LINE_ACC_INT2, PAL_MODE_INPUT_PULLUP);
  palSetLineCallback(LINE_ACC_INT2, MDT_interrupt, NULL);
  palEnableLineEvent(LINE_ACC_INT2, PAL_EVENT_MODE_FALLING_EDGE);
}

void MDT_loadStateMachine_2(void)
{
  MDT_writeReg(LIS3DSH_AD_CTRL_REG1, 0x00);

  MDT_writeReg(LIS3DSH_AD_CTRL_REG2, LIS3DSH_CTRL_REG2_SM2_PIN | LIS3DSH_CTRL_REG2_SM2_EN);
  MDT_writeReg(LIS3DSH_AD_CTRL_REG3, LIS3DSH_CTRL_REG3_INT2_EN);

  MDT_writeReg(LIS3DSH_AD_THRS1_1, 0x20);
  MDT_writeReg(LIS3DSH_AD_ST2_1, 0x05);
  MDT_writeReg(LIS3DSH_AD_ST2_2, 0x11);
  MDT_writeReg(LIS3DSH_AD_MASK2_B, 0xfc);
  MDT_writeReg(LIS3DSH_AD_MASK2_A, 0xfc);

  MDT_writeReg(LIS3DSH_AD_SETT2, 0x11);
}

void MDT_beep(void)
{
  pwmEnableChannel(&PWMD2, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, 5000));
  chThdSleepMilliseconds(20);
  pwmDisableChannel(&PWMD2, 2);
}

MDT_State_t MDT_initStateHandler(MDT_Command_t cmd) {
  MDT_State_t newState = MDT_STATE_INIT;

  switch(cmd) {
  case MDT_CMD_START: {
    pwmStart(&PWMD2, &beepConfig);
    lis3dshStart(&detector.lis3dsh, &lis3dshConfig);
    MDT_loadStateMachine_2();
    MDT_configureInterrupt_2();
    newState = MDT_STATE_ENABLED;
    break;
  }
  case MDT_CMD_STOP: {
    newState = MDT_STATE_DISABLED;
    break;
  }
  case MDT_CMD_LISTEN:
  default: {
    break;
  }
  }

  return newState;
}

MDT_State_t MDT_enabledStateHandler(MDT_Command_t cmd) {
  MDT_State_t newState = MDT_STATE_ENABLED;

  switch(cmd) {
  case MDT_CMD_STOP: {
    pwmStop(&PWMD2);
    lis3dshStop(&detector.lis3dsh);
    newState = MDT_STATE_DISABLED;
    break;
  }
  case MDT_CMD_LISTEN: {
    MDT_beep();
    chThdSleepMilliseconds(500);
    MDT_loadStateMachine_2();
    break;
  }
  case MDT_CMD_START:
  default: {
    break;
  }
  }

  return newState;
}

MDT_State_t MDT_disabledStateHandler(MDT_Command_t cmd) {
  MDT_State_t newState = MDT_STATE_DISABLED;

  switch (cmd) {
  case MDT_CMD_START: {
    pwmStart(&PWMD2, &beepConfig);
    lis3dshStart(&detector.lis3dsh, &lis3dshConfig);
    MDT_loadStateMachine_2();
    MDT_configureInterrupt_2();
    newState = MDT_STATE_ENABLED;
    break;
  }
  case MDT_CMD_LISTEN:
  case MDT_CMD_STOP:
  default: {
    break;
  }
  }

  return newState;
}

/*****************************************************************************/
/* DEFINITION OF GLOBAL FUNCTIONS                                            */
/*****************************************************************************/
THD_FUNCTION(MDT_Thread, arg) {
  (void)arg;
  chRegSetThreadName("motion-detector");

  MDT_Start();

  while(true) {
    msg_t msg;
    if (MSG_OK == chMBFetchTimeout(&detector.mailbox, &msg, TIME_INFINITE)) {
      MDT_Command_t cmd = (MDT_Command_t)msg;
      switch(detector.state) {
      case MDT_STATE_INIT: {
        detector.state = MDT_initStateHandler(cmd);
        break;
      }
      case MDT_STATE_ENABLED: {
        detector.state = MDT_enabledStateHandler(cmd);
        break;
      }
      case MDT_STATE_DISABLED: {
        detector.state = MDT_disabledStateHandler(cmd);
        break;
      }
      default: {
        break;
      }
      }
    }
  }
} 

void MDT_Init(void)
{
  detector.state = MDT_STATE_INIT;
  memset(detector.commands, 0, sizeof(detector.commands));
  chMBObjectInit(&detector.mailbox, detector.commands, ARRAY_LENGTH(detector.commands));
  lis3dshObjectInit(&detector.lis3dsh);
}

void MDT_StartI(void)
{
  chMBPostI(&detector.mailbox, MDT_CMD_START);
}

void MDT_Start(void)
{
  chSysLock();
  MDT_StartI();
  chSysUnlock();
}

void MDT_StopI(void)
{
  chMBPostI(&detector.mailbox, MDT_CMD_STOP);
}

void MDT_Stop(void)
{
  chSysLock();
  MDT_StopI();
  chSysUnlock();
}

void MDT_ListenI(void)
{
  chMBPostI(&detector.mailbox, MDT_CMD_LISTEN);
}

void MDT_Listen(void)
{
  chSysLock();
  MDT_ListenI();
  chSysUnlock();
}

/****************************** END OF FILE **********************************/
