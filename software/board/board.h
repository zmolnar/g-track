/**
 * @file board.h
 * @brief Board definition file for G-track hardware v1.0.
 * @author Zoltan, Molnar
 */

#ifndef BOARD_H
#define BOARD_H

/*******************************************************************************/
/* INCLUDES                                                                    */
/*******************************************************************************/

/*******************************************************************************/
/* DEFINED CONSTANTS                                                           */
/*******************************************************************************/
/*
 * Board identifier.
 */
#define BOARD_G_TRACK_V_1_0
#define BOARD_NAME                  "G-track GPS tracker v1.0"

/*
 * Board oscillators-related settings.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                32768U
#endif

#define STM32_LSEDRV                (3U << 3U)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                8000000U
#endif

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   300U

/*
 * MCU type as defined in the ST header.
 */
#define STM32L452xx

/*
 * IO pins assignments.
 */
#define GPIOA_DCM_AIN1              0U
#define GPIOA_DCM_AIN2              1U
#define GPIOA_DCM_BIN2              2U
#define GPIOA_DCM_BIN1              3U
#define GPIOA_ACC_SPI1_CS           4U
#define GPIOA_ACC_SPI1_SCK          5U
#define GPIOA_ACC_SPI1_MISO         6U
#define GPIOA_ACC_SPI1_MOSI         7U
#define GPIOA_BT0                   8U
#define GPIOA_USB_VBUS_SENSE        9U
#define GPIOA_PA10_NC               10U
#define GPIOA_USB_DM                11U
#define GPIOA_USB_DP                12U
#define GPIOA_SWDIO                 13U
#define GPIOA_SWCLK                 14U
#define GPIOA_LED_2_RED             15U

#define GPIOB_ACC_INT1              0U
#define GPIOB_ACC_INT2              1U
#define GPIOB_PB2_NC                2U
#define GPIOB_SWD_SWO               3U
#define GPIOB_PB4_NC                4U
#define GPIOB_WAVESHARE_POWER       5U
#define GPIOB_RPI_UART_TX           6U
#define GPIOB_RPI_UART_RX           7U
#define GPIOB_PB8_NC                8U
#define GPIOB_EXT_SW1               9U
#define GPIOB_BUZZER_PWM            10U
#define GPIOB_PB11_NC               11U
#define GPIOB_SDC_SPI2_CS           12U
#define GPIOB_SDC_SPI2_SCK          13U
#define GPIOB_SDC_SPI2_MISO         14U
#define GPIOB_SDC_SPI2_MOSI         15U

#define GPIOC_PC0_NC                0U
#define GPIOC_ADC_VBAT              1U
#define GPIOC_DCM_FAULT             2U
#define GPIOC_DCM_SLEEP             3U
#define GPIOC_PC4_NC                4U
#define GPIOC_PC5_NC                5U
#define GPIOC_SDC_CARD_DETECT       6U
#define GPIOC_PC7_NC                7U
#define GPIOC_PC8_NC                8U
#define GPIOC_PC9_NC                9U
#define GPIOC_LED_3_GREEN           10U
#define GPIOC_EXT_IGNITION          11U
#define GPIOC_EXT_SW2               12U
#define GPIOC_EXT_LED               13U
#define GPIOC_OSC32_IN              14U
#define GPIOC_OSC32_OUT             15U

/*
 * IO lines assignments.
 */
#define LINE_DCM_AIN1               PAL_LINE(GPIOA, GPIOA_DCM_AIN1)
#define LINE_DCM_AIN2               PAL_LINE(GPIOA, GPIOA_DCM_AIN2)
#define LINE_DCM_BIN2               PAL_LINE(GPIOA, GPIOA_DCM_BIN2)
#define LINE_DCM_BIN1               PAL_LINE(GPIOA, GPIOA_DCM_BIN1)
#define LINE_ACC_CS                 PAL_LINE(GPIOA, GPIOA_ACC_SPI1_CS)
#define LINE_ACC_SCK                PAL_LINE(GPIOA, GPIOA_ACC_SPI1_SCK)
#define LINE_ACC_MISO               PAL_LINE(GPIOA, GPIOA_ACC_SPI1_MISO)
#define LINE_ACC_MOSI               PAL_LINE(GPIOA, GPIOA_ACC_SPI1_MOSI)
#define LINE_BT0                    PAL_LINE(GPIOA, GPIOA_BT0)
#define LINE_USB_VBUS_SENSE         PAL_LINE(GPIOA, GPIOA_USB_VBUS_SENSE)
#define LINE_USB_DM                 PAL_LINE(GPIOA, GPIOA_USB_DM)
#define LINE_USB_DP                 PAL_LINE(GPIOA, GPIOA_USB_DP)
#define LINE_USB_LED                PAL_LINE(GPIOA, GPIOA_SWDIO)
#define LINE_SWD_SWCLK              PAL_LINE(GPIOA, GPIOA_SWCLK)
#define LINE_LED_2_RED              PAL_LINE(GPIOA, GPIOA_LED_2_RED)

#define LINE_ACC_INT1               PAL_LINE(GPIOB, GPIOB_ACC_INT1)
#define LINE_ACC_INT2               PAL_LINE(GPIOB, GPIOB_ACC_INT2)  
#define LINE_SWD_SWO                PAL_LINE(GPIOB, GPIOB_SWD_SWO)
#define LINE_WAVESHARE_POWER        PAL_LINE(GPIOB, GPIOB_WAVESHARE_POWER)
#define LINE_RPI_UART_TX            PAL_LINE(GPIOB, GPIOB_RPI_UART_TX)
#define LINE_RPI_UART_RX            PAL_LINE(GPIOB, GPIOB_RPI_UART_RX)
#define LINE_EXT_SW1                PAL_LINE(GPIOB, GPIOB_EXT_SW1)
#define LINE_BUZZER_PWM             PAL_LINE(GPIOB, GPIOB_BUZZER_PWM)
#define LINE_SDC_CS                 PAL_LINE(GPIOB, GPIOB_SDC_SPI2_CS)
#define LINE_SDC_SCK                PAL_LINE(GPIOB, GPIOB_SDC_SPI2_SCK)
#define LINE_SDC_MISO               PAL_LINE(GPIOB, GPIOB_SDC_SPI2_MISO)
#define LINE_SDC_MOSI               PAL_LINE(GPIOB, GPIOB_SDC_SPI2_MOSI)

#define LINE_ADC_VBAT               PAL_LINE(GPIOC, GPIOC_ADC_VBAT)
#define LINE_DCM_FAULT              PAL_LINE(GPIOC, GPIOC_DCM_FAULT)
#define LINE_DCM_SLEEP              PAL_LINE(GPIOC, GPIOC_DCM_SLEEP)
#define LINE_SDC_CARD_DETECT        PAL_LINE(GPIOC, GPIOC_SDC_CARD_DETECT)
#define LINE_LED_3_GREEN            PAL_LINE(GPIOC, GPIOC_LED_3_GREEN)
#define LINE_EXT_IGNITION           PAL_LINE(GPIOC, GPIOC_EXT_IGNITION)
#define LINE_EXT_SW2                PAL_LINE(GPIOC, GPIOC_EXT_SW2)
#define LINE_EXT_LED                PAL_LINE(GPIOC, GPIOC_EXT_LED)

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))

/*
 * GPIOA setup:
 *
 * PA0  - DCM_AIN1                  (output push-pull).
 * PA1  - DCM_AIN2                  (output push-pull).
 * PA2  - DCM_BIN2                  (input pullup).
 * PA3  - DCM_BIN1                  (input pullup).
 * PA4  - ACC_CS                    (input pullup).
 * PA5  - ACC_SCK                   (input pullup).
 * PA6  - ACC_MISO                  (input pullup).
 * PA7  - ACC_MOSI                  (input pullup).
 * PA8  - BT0                       (input pullup).
 * PA9  - USB_VBUS_SENSE            (input pulldown).
 * PA10 - PA10_NC                   (input pullup).
 * PA11 - USB_DM                    (input pullup).
 * PA12 - USB_DP                    (input pullup).
 * PA13 - SWDIO_USB_LED             (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - LED_2_RED                 (output push-pull).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_OUTPUT(GPIOA_DCM_AIN1)          | \
                                     PIN_MODE_OUTPUT(GPIOA_DCM_AIN2)          | \
                                     PIN_MODE_INPUT(GPIOA_DCM_BIN2)           | \
                                     PIN_MODE_INPUT(GPIOA_DCM_BIN1)           | \
                                     PIN_MODE_OUTPUT(GPIOA_ACC_SPI1_CS)       | \
                                     PIN_MODE_ALTERNATE(GPIOA_ACC_SPI1_SCK)   | \
                                     PIN_MODE_ALTERNATE(GPIOA_ACC_SPI1_MISO)  | \
                                     PIN_MODE_ALTERNATE(GPIOA_ACC_SPI1_MOSI)  | \
                                     PIN_MODE_INPUT(GPIOA_BT0)                | \
                                     PIN_MODE_INPUT(GPIOA_USB_VBUS_SENSE)     | \
                                     PIN_MODE_INPUT(GPIOA_PA10_NC)            | \
                                     PIN_MODE_INPUT(GPIOA_USB_DM)             | \
                                     PIN_MODE_INPUT(GPIOA_USB_DP)             | \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO)          | \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK)          | \
                                     PIN_MODE_OUTPUT(GPIOA_LED_2_RED))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_DCM_AIN1)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_DCM_AIN2)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_DCM_BIN2)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_DCM_BIN1)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACC_SPI1_CS)    | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACC_SPI1_SCK)   | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACC_SPI1_MISO)  | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ACC_SPI1_MOSI)  | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_BT0)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_VBUS_SENSE) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PA10_NC)        | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DM)         | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_DP)         | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO)          | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK)          | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LED_2_RED))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_HIGH(GPIOA_DCM_AIN1)          | \
                                     PIN_OSPEED_HIGH(GPIOA_DCM_AIN2)          | \
                                     PIN_OSPEED_HIGH(GPIOA_DCM_BIN2)          | \
                                     PIN_OSPEED_HIGH(GPIOA_DCM_BIN1)          | \
                                     PIN_OSPEED_HIGH(GPIOA_ACC_SPI1_CS)       | \
                                     PIN_OSPEED_HIGH(GPIOA_ACC_SPI1_SCK)      | \
                                     PIN_OSPEED_HIGH(GPIOA_ACC_SPI1_MISO)     | \
                                     PIN_OSPEED_HIGH(GPIOA_ACC_SPI1_MOSI)     | \
                                     PIN_OSPEED_HIGH(GPIOA_BT0)               | \
                                     PIN_OSPEED_HIGH(GPIOA_USB_VBUS_SENSE)    | \
                                     PIN_OSPEED_HIGH(GPIOA_PA10_NC)           | \
                                     PIN_OSPEED_HIGH(GPIOA_USB_DM)            | \
                                     PIN_OSPEED_HIGH(GPIOA_USB_DP)            | \
                                     PIN_OSPEED_HIGH(GPIOA_SWDIO)             | \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK)             | \
                                     PIN_OSPEED_HIGH(GPIOA_LED_2_RED))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_PULLDOWN(GPIOA_DCM_AIN1)       | \
                                     PIN_PUPDR_PULLDOWN(GPIOA_DCM_AIN2)       | \
                                     PIN_PUPDR_PULLUP(GPIOA_DCM_BIN2)         | \
                                     PIN_PUPDR_PULLUP(GPIOA_DCM_BIN1)         | \
                                     PIN_PUPDR_PULLUP(GPIOA_ACC_SPI1_CS)      | \
                                     PIN_PUPDR_PULLUP(GPIOA_ACC_SPI1_SCK)     | \
                                     PIN_PUPDR_FLOATING(GPIOA_ACC_SPI1_MISO)    | \
                                     PIN_PUPDR_FLOATING(GPIOA_ACC_SPI1_MOSI)    | \
                                     PIN_PUPDR_PULLUP(GPIOA_BT0)              | \
                                     PIN_PUPDR_PULLDOWN(GPIOA_USB_VBUS_SENSE) | \
                                     PIN_PUPDR_PULLUP(GPIOA_PA10_NC)          | \
                                     PIN_PUPDR_PULLUP(GPIOA_USB_DM)           | \
                                     PIN_PUPDR_PULLUP(GPIOA_USB_DP)           | \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO)            | \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK)          | \
                                     PIN_PUPDR_PULLUP(GPIOA_LED_2_RED))
#define VAL_GPIOA_ODR               (PIN_ODR_LOW(GPIOA_DCM_AIN1)              | \
                                     PIN_ODR_LOW(GPIOA_DCM_AIN2)              | \
                                     PIN_ODR_HIGH(GPIOA_DCM_BIN2)             | \
                                     PIN_ODR_HIGH(GPIOA_DCM_BIN1)             | \
                                     PIN_ODR_HIGH(GPIOA_ACC_SPI1_CS)          | \
                                     PIN_ODR_HIGH(GPIOA_ACC_SPI1_SCK)         | \
                                     PIN_ODR_HIGH(GPIOA_ACC_SPI1_MISO)        | \
                                     PIN_ODR_HIGH(GPIOA_ACC_SPI1_MOSI)        | \
                                     PIN_ODR_HIGH(GPIOA_BT0)                  | \
                                     PIN_ODR_HIGH(GPIOA_USB_VBUS_SENSE)       | \
                                     PIN_ODR_HIGH(GPIOA_PA10_NC)              | \
                                     PIN_ODR_HIGH(GPIOA_USB_DM)               | \
                                     PIN_ODR_HIGH(GPIOA_USB_DP)               | \
                                     PIN_ODR_HIGH(GPIOA_SWDIO)                | \
                                     PIN_ODR_HIGH(GPIOA_SWCLK)                | \
                                     PIN_ODR_HIGH(GPIOA_LED_2_RED))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_DCM_AIN1, 0U)          | \
                                     PIN_AFIO_AF(GPIOA_DCM_AIN2, 0U)          | \
                                     PIN_AFIO_AF(GPIOA_DCM_BIN2, 0U)          | \
                                     PIN_AFIO_AF(GPIOA_DCM_BIN1, 0U)          | \
                                     PIN_AFIO_AF(GPIOA_ACC_SPI1_CS, 0U)       | \
                                     PIN_AFIO_AF(GPIOA_ACC_SPI1_SCK, 5U)      | \
                                     PIN_AFIO_AF(GPIOA_ACC_SPI1_MISO, 5U)     | \
                                     PIN_AFIO_AF(GPIOA_ACC_SPI1_MOSI, 5U))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_BT0, 0U)               | \
                                     PIN_AFIO_AF(GPIOA_USB_VBUS_SENSE, 0U)    | \
                                     PIN_AFIO_AF(GPIOA_PA10_NC, 0U)           | \
                                     PIN_AFIO_AF(GPIOA_USB_DM, 0U)            | \
                                     PIN_AFIO_AF(GPIOA_USB_DP, 0U)            | \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0U)             | \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0U)             | \
                                     PIN_AFIO_AF(GPIOA_LED_2_RED, 0U))

/*
 * GPIOB setup:
 *
 * PB0  - ACC_INT1                  (input pullup).
 * PB1  - ACC_INT2                  (input pullup).
 * PB2  - PB2_NC                    (input pullup).
 * PB3  - SWD_SWO                   (input pullup).
 * PB4  - PB4_NC                    (input pullup).
 * PB5  - WAVESHARE_POWER           (output push-pull).
 * PB6  - RPI_UART_TX               (input pullup).
 * PB7  - RPI_UART_RX               (input pullup).
 * PB8  - PB8_NC                    (input pullup).
 * PB9  - EXT_SW1                   (input push-pull pull-down).
 * PB10 - BUZZER_PWM                (output push-pull).
 * PB11 - PB11_NC                   (input pullup).
 * PB12 - SDC_SPI2_CS               (alternate 5).
 * PB13 - SDC_SPI2_SCK              (alternate 5).
 * PB14 - SDC_SPI2_MISO             (alternate 5).
 * PB15 - SDC_SPI2_MOSI             (alternate 5).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_INPUT(GPIOB_ACC_INT1)           | \
                                     PIN_MODE_INPUT(GPIOB_ACC_INT2)           | \
                                     PIN_MODE_INPUT(GPIOB_PB2_NC)             | \
                                     PIN_MODE_ALTERNATE(GPIOB_SWD_SWO)        | \
                                     PIN_MODE_INPUT(GPIOB_PB4_NC)             | \
                                     PIN_MODE_OUTPUT(GPIOB_WAVESHARE_POWER)   | \
                                     PIN_MODE_ALTERNATE(GPIOB_RPI_UART_TX)    | \
                                     PIN_MODE_ALTERNATE(GPIOB_RPI_UART_RX)    | \
                                     PIN_MODE_INPUT(GPIOB_PB8_NC)             | \
                                     PIN_MODE_INPUT(GPIOB_EXT_SW1)            | \
                                     PIN_MODE_ALTERNATE(GPIOB_BUZZER_PWM)     | \
                                     PIN_MODE_INPUT(GPIOB_PB11_NC)            | \
                                     PIN_MODE_ALTERNATE(GPIOB_SDC_SPI2_CS)    | \
                                     PIN_MODE_ALTERNATE(GPIOB_SDC_SPI2_SCK)   | \
                                     PIN_MODE_ALTERNATE(GPIOB_SDC_SPI2_MISO)  | \
                                     PIN_MODE_ALTERNATE(GPIOB_SDC_SPI2_MOSI))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_ACC_INT1)           | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_ACC_INT2)           | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PB2_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SWD_SWO)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PB4_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_WAVESHARE_POWER)    | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_RPI_UART_TX)        | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_RPI_UART_RX)        | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PB8_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_EXT_SW1)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_BUZZER_PWM)         | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PB11_NC)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SDC_SPI2_CS)        | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SDC_SPI2_SCK)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SDC_SPI2_MISO)      | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SDC_SPI2_MOSI))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_HIGH(GPIOB_ACC_INT1)           | \
                                     PIN_OSPEED_HIGH(GPIOB_ACC_INT2)           | \
                                     PIN_OSPEED_HIGH(GPIOB_PB2_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOB_SWD_SWO)            | \
                                     PIN_OSPEED_HIGH(GPIOB_PB4_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOB_WAVESHARE_POWER)    | \
                                     PIN_OSPEED_HIGH(GPIOB_RPI_UART_TX)        | \
                                     PIN_OSPEED_HIGH(GPIOB_RPI_UART_RX)        | \
                                     PIN_OSPEED_HIGH(GPIOB_PB8_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOB_EXT_SW1)            | \
                                     PIN_OSPEED_HIGH(GPIOB_BUZZER_PWM)         | \
                                     PIN_OSPEED_HIGH(GPIOB_PB11_NC)            | \
                                     PIN_OSPEED_HIGH(GPIOB_SDC_SPI2_CS)        | \
                                     PIN_OSPEED_HIGH(GPIOB_SDC_SPI2_SCK)       | \
                                     PIN_OSPEED_HIGH(GPIOB_SDC_SPI2_MISO)      | \
                                     PIN_OSPEED_HIGH(GPIOB_SDC_SPI2_MOSI))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_PULLUP(GPIOB_ACC_INT1)           | \
                                     PIN_PUPDR_PULLUP(GPIOB_ACC_INT2)           | \
                                     PIN_PUPDR_PULLUP(GPIOB_PB2_NC)             | \
                                     PIN_PUPDR_FLOATING(GPIOB_SWD_SWO)          | \
                                     PIN_PUPDR_PULLUP(GPIOB_PB4_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOB_WAVESHARE_POWER)    | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_RPI_UART_TX)      | \
                                     PIN_PUPDR_PULLUP(GPIOB_RPI_UART_RX)        | \
                                     PIN_PUPDR_PULLUP(GPIOB_PB8_NC)             | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_EXT_SW1)          | \
                                     PIN_PUPDR_FLOATING(GPIOB_BUZZER_PWM)       | \
                                     PIN_PUPDR_PULLUP(GPIOB_PB11_NC)            | \
                                     PIN_PUPDR_PULLUP(GPIOB_SDC_SPI2_CS)        | \
                                     PIN_PUPDR_PULLDOWN(GPIOB_SDC_SPI2_SCK)     | \
                                     PIN_PUPDR_PULLUP(GPIOB_SDC_SPI2_MISO)      | \
                                     PIN_PUPDR_PULLUP(GPIOB_SDC_SPI2_MOSI))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_ACC_INT1)           | \
                                     PIN_ODR_HIGH(GPIOB_ACC_INT2)           | \
                                     PIN_ODR_HIGH(GPIOB_PB2_NC)             | \
                                     PIN_ODR_HIGH(GPIOB_SWD_SWO)            | \
                                     PIN_ODR_HIGH(GPIOB_PB4_NC)             | \
                                     PIN_ODR_HIGH(GPIOB_WAVESHARE_POWER)    | \
                                     PIN_ODR_HIGH(GPIOB_RPI_UART_TX)        | \
                                     PIN_ODR_HIGH(GPIOB_RPI_UART_RX)        | \
                                     PIN_ODR_HIGH(GPIOB_PB8_NC)             | \
                                     PIN_ODR_HIGH(GPIOB_EXT_SW1)            | \
                                     PIN_ODR_HIGH(GPIOB_BUZZER_PWM)         | \
                                     PIN_ODR_HIGH(GPIOB_PB11_NC)            | \
                                     PIN_ODR_HIGH(GPIOB_SDC_SPI2_CS)        | \
                                     PIN_ODR_LOW(GPIOB_SDC_SPI2_SCK)        | \
                                     PIN_ODR_HIGH(GPIOB_SDC_SPI2_MISO)      | \
                                     PIN_ODR_HIGH(GPIOB_SDC_SPI2_MOSI))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_ACC_INT1, 0U)           | \
                                     PIN_AFIO_AF(GPIOB_ACC_INT2, 0U)           | \
                                     PIN_AFIO_AF(GPIOB_PB2_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOB_SWD_SWO, 0U)            | \
                                     PIN_AFIO_AF(GPIOB_PB4_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOB_WAVESHARE_POWER, 0U)    | \
                                     PIN_AFIO_AF(GPIOB_RPI_UART_TX, 7U)        | \
                                     PIN_AFIO_AF(GPIOB_RPI_UART_RX, 7U))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_PB8_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOB_EXT_SW1, 0U)            | \
                                     PIN_AFIO_AF(GPIOB_BUZZER_PWM, 1U)         | \
                                     PIN_AFIO_AF(GPIOB_PB11_NC, 0U)            | \
                                     PIN_AFIO_AF(GPIOB_SDC_SPI2_CS, 5U)        | \
                                     PIN_AFIO_AF(GPIOB_SDC_SPI2_SCK, 5U)       | \
                                     PIN_AFIO_AF(GPIOB_SDC_SPI2_MISO, 5U)      | \
                                     PIN_AFIO_AF(GPIOB_SDC_SPI2_MOSI, 5U))

/*
 * GPIOC setup:
 *
 * PC0  - PC0_NC                    (input pullup).
 * PC1  - ADC_VBAT                  (input pullup).
 * PC2  - DCM_FAULT                 (input pullup).
 * PC3  - DCM_SLEEP                 (output pullup).
 * PC4  - PC4_NC                    (input pullup).
 * PC5  - PC5_NC                    (input pullup).
 * PC6  - SDC_CARD_DETECT           (input pulldown).
 * PC7  - PC7_NC                    (input pullup).
 * PC8  - PC8_NC                    (input pullup).
 * PC9  - PC9_NC                    (input pullup).
 * PC10 - LED_3_GREEN               (output push-pull).
 * PC11 - EXT_IGNITION              (input push-pull pull-down).
 * PC12 - EXT_SW2                   (input push-pull pull-down).
 * PC13 - EXT_LED                   (output push-pull).
 * PC14 - OSC32_IN                  (input pullup).
 * PC15 - OSC32_OUT                 (input pullup).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_INPUT(GPIOC_PC0_NC)             | \
                                     PIN_MODE_INPUT(GPIOC_ADC_VBAT)           | \
                                     PIN_MODE_INPUT(GPIOC_DCM_FAULT)          | \
                                     PIN_MODE_OUTPUT(GPIOC_DCM_SLEEP)         | \
                                     PIN_MODE_INPUT(GPIOC_PC4_NC)             | \
                                     PIN_MODE_INPUT(GPIOC_PC5_NC)             | \
                                     PIN_MODE_INPUT(GPIOC_SDC_CARD_DETECT)    | \
                                     PIN_MODE_INPUT(GPIOC_PC7_NC)             | \
                                     PIN_MODE_INPUT(GPIOC_PC8_NC)             | \
                                     PIN_MODE_INPUT(GPIOC_PC9_NC)             | \
                                     PIN_MODE_OUTPUT(GPIOC_LED_3_GREEN)       | \
                                     PIN_MODE_INPUT(GPIOC_EXT_IGNITION)       | \
                                     PIN_MODE_INPUT(GPIOC_EXT_SW2)            | \
                                     PIN_MODE_OUTPUT(GPIOC_EXT_LED)           | \
                                     PIN_MODE_INPUT(GPIOC_OSC32_IN)           | \
                                     PIN_MODE_INPUT(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_PC0_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_ADC_VBAT)           | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_DCM_FAULT)          | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_DCM_SLEEP)          | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PC4_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PC5_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDC_CARD_DETECT)    | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PC7_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PC8_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PC9_NC)             | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_LED_3_GREEN)        | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_EXT_IGNITION)       | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_EXT_SW2)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_EXT_LED)            | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_IN)           | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_HIGH(GPIOC_PC0_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_ADC_VBAT)           | \
                                     PIN_OSPEED_HIGH(GPIOC_DCM_FAULT)          | \
                                     PIN_OSPEED_HIGH(GPIOC_DCM_SLEEP)          | \
                                     PIN_OSPEED_HIGH(GPIOC_PC4_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_PC5_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_SDC_CARD_DETECT)    | \
                                     PIN_OSPEED_HIGH(GPIOC_PC7_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_PC8_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_PC9_NC)             | \
                                     PIN_OSPEED_HIGH(GPIOC_LED_3_GREEN)        | \
                                     PIN_OSPEED_HIGH(GPIOC_EXT_IGNITION)       | \
                                     PIN_OSPEED_HIGH(GPIOC_EXT_SW2)            | \
                                     PIN_OSPEED_HIGH(GPIOC_EXT_LED)            | \
                                     PIN_OSPEED_HIGH(GPIOC_OSC32_IN)           | \
                                     PIN_OSPEED_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_PULLUP(GPIOC_PC0_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOC_ADC_VBAT)           | \
                                     PIN_PUPDR_PULLUP(GPIOC_DCM_FAULT)          | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_DCM_SLEEP)        | \
                                     PIN_PUPDR_PULLUP(GPIOC_PC4_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOC_PC5_NC)             | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_SDC_CARD_DETECT)  | \
                                     PIN_PUPDR_PULLUP(GPIOC_PC7_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOC_PC8_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOC_PC9_NC)             | \
                                     PIN_PUPDR_PULLUP(GPIOC_LED_3_GREEN)        | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_EXT_IGNITION)     | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_EXT_SW2)          | \
                                     PIN_PUPDR_PULLDOWN(GPIOC_EXT_LED)          | \
                                     PIN_PUPDR_PULLUP(GPIOC_OSC32_IN)           | \
                                     PIN_PUPDR_PULLUP(GPIOC_OSC32_OUT))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_PC0_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_ADC_VBAT)           | \
                                     PIN_ODR_HIGH(GPIOC_DCM_FAULT)          | \
                                     PIN_ODR_LOW(GPIOC_DCM_SLEEP)           | \
                                     PIN_ODR_HIGH(GPIOC_PC4_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_PC5_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_SDC_CARD_DETECT)    | \
                                     PIN_ODR_HIGH(GPIOC_PC7_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_PC8_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_PC9_NC)             | \
                                     PIN_ODR_HIGH(GPIOC_LED_3_GREEN)        | \
                                     PIN_ODR_HIGH(GPIOC_EXT_IGNITION)       | \
                                     PIN_ODR_HIGH(GPIOC_EXT_SW2)            | \
                                     PIN_ODR_LOW(GPIOC_EXT_LED)             | \
                                     PIN_ODR_HIGH(GPIOC_OSC32_IN)           | \
                                     PIN_ODR_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_PC0_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOC_ADC_VBAT, 0U)           | \
                                     PIN_AFIO_AF(GPIOC_DCM_FAULT, 0U)          | \
                                     PIN_AFIO_AF(GPIOC_DCM_SLEEP, 0U)          | \
                                     PIN_AFIO_AF(GPIOC_PC4_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOC_PC5_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOC_SDC_CARD_DETECT, 0U)    | \
                                     PIN_AFIO_AF(GPIOC_PC7_NC, 0U))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_PC8_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOC_PC9_NC, 0U)             | \
                                     PIN_AFIO_AF(GPIOC_LED_3_GREEN, 0U)        | \
                                     PIN_AFIO_AF(GPIOC_EXT_IGNITION, 0U)       | \
                                     PIN_AFIO_AF(GPIOC_EXT_SW2, 0U)            | \
                                     PIN_AFIO_AF(GPIOC_EXT_LED, 0U)            | \
                                     PIN_AFIO_AF(GPIOC_OSC32_IN, 0U)           | \
                                     PIN_AFIO_AF(GPIOC_OSC32_OUT, 0U))

/*******************************************************************************/
/* MACRO DEFINITIONS                                                           */
/*******************************************************************************/

/*******************************************************************************/
/* TYPE DEFINITIONS                                                            */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL VARIABLES                                             */
/*******************************************************************************/

/*******************************************************************************/
/* DECLARATION OF GLOBAL FUNCTIONS                                             */
/*******************************************************************************/
#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */

/*******************************************************************************/
