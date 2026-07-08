/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA0
#define PWM_0_INST_IRQHandler                                   TIMA0_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA0_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_0
#define GPIO_PWM_0_C0_IOMUX                                       (IOMUX_PINCM1)
#define GPIO_PWM_0_C0_IOMUX_FUNC                      IOMUX_PINCM1_PF_TIMA0_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_1
#define GPIO_PWM_0_C1_IOMUX                                       (IOMUX_PINCM2)
#define GPIO_PWM_0_C1_IOMUX_FUNC                      IOMUX_PINCM2_PF_TIMA0_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)





/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_0                                      DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_0_REF                     DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_0_REF_VOLTAGE_V                                       3.3
#define GPIO_ADC12_0_C2_PORT                                               GPIOA
#define GPIO_ADC12_0_C2_PIN                                       DL_GPIO_PIN_25
#define GPIO_ADC12_0_IOMUX_C2                                    (IOMUX_PINCM55)
#define GPIO_ADC12_0_IOMUX_C2_FUNC                (IOMUX_PINCM55_PF_UNCONNECTED)



/* Port definition for Pin Group RUN_LED */
#define RUN_LED_PORT                                                     (GPIOB)

/* Defines for LED: GPIOB.27 with pinCMx 58 on package pin 29 */
#define RUN_LED_LED_PIN                                         (DL_GPIO_PIN_27)
#define RUN_LED_LED_IOMUX                                        (IOMUX_PINCM58)
/* Port definition for Pin Group TB6612_PORTB */
#define TB6612_PORTB_PORT                                                (GPIOB)

/* Defines for AIN2: GPIOB.20 with pinCMx 48 on package pin 19 */
#define TB6612_PORTB_AIN2_PIN                                   (DL_GPIO_PIN_20)
#define TB6612_PORTB_AIN2_IOMUX                                  (IOMUX_PINCM48)
/* Defines for STBY: GPIOB.26 with pinCMx 57 on package pin 28 */
#define TB6612_PORTB_STBY_PIN                                   (DL_GPIO_PIN_26)
#define TB6612_PORTB_STBY_IOMUX                                  (IOMUX_PINCM57)
/* Port definition for Pin Group TB6612_PORTA */
#define TB6612_PORTA_PORT                                                (GPIOA)

/* Defines for AIN1: GPIOA.15 with pinCMx 37 on package pin 8 */
#define TB6612_PORTA_AIN1_PIN                                   (DL_GPIO_PIN_15)
#define TB6612_PORTA_AIN1_IOMUX                                  (IOMUX_PINCM37)
/* Defines for BIN1: GPIOA.28 with pinCMx 3 on package pin 35 */
#define TB6612_PORTA_BIN1_PIN                                   (DL_GPIO_PIN_28)
#define TB6612_PORTA_BIN1_IOMUX                                   (IOMUX_PINCM3)
/* Defines for BIN2: GPIOA.16 with pinCMx 38 on package pin 9 */
#define TB6612_PORTA_BIN2_PIN                                   (DL_GPIO_PIN_16)
#define TB6612_PORTA_BIN2_IOMUX                                  (IOMUX_PINCM38)
/* Port definition for Pin Group TRACKING_SEL */
#define TRACKING_SEL_PORT                                                (GPIOA)

/* Defines for S0: GPIOA.24 with pinCMx 54 on package pin 25 */
#define TRACKING_SEL_S0_PIN                                     (DL_GPIO_PIN_24)
#define TRACKING_SEL_S0_IOMUX                                    (IOMUX_PINCM54)
/* Defines for S1: GPIOA.26 with pinCMx 59 on package pin 30 */
#define TRACKING_SEL_S1_PIN                                     (DL_GPIO_PIN_26)
#define TRACKING_SEL_S1_IOMUX                                    (IOMUX_PINCM59)
/* Defines for S2: GPIOA.27 with pinCMx 60 on package pin 31 */
#define TRACKING_SEL_S2_PIN                                     (DL_GPIO_PIN_27)
#define TRACKING_SEL_S2_IOMUX                                    (IOMUX_PINCM60)
/* Port definition for Pin Group ENCODER_PORTB */
#define ENCODER_PORTB_PORT                                               (GPIOB)

/* Defines for LEFT_A: GPIOB.6 with pinCMx 23 on package pin 58 */
// pins affected by this interrupt request:["LEFT_A","RIGHT_A"]
#define ENCODER_PORTB_INT_IRQN                                  (GPIOB_INT_IRQn)
#define ENCODER_PORTB_INT_IIDX                  (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENCODER_PORTB_LEFT_A_IIDX                            (DL_GPIO_IIDX_DIO6)
#define ENCODER_PORTB_LEFT_A_PIN                                 (DL_GPIO_PIN_6)
#define ENCODER_PORTB_LEFT_A_IOMUX                               (IOMUX_PINCM23)
/* Defines for LEFT_B: GPIOB.7 with pinCMx 24 on package pin 59 */
#define ENCODER_PORTB_LEFT_B_PIN                                 (DL_GPIO_PIN_7)
#define ENCODER_PORTB_LEFT_B_IOMUX                               (IOMUX_PINCM24)
/* Defines for RIGHT_A: GPIOB.8 with pinCMx 25 on package pin 60 */
#define ENCODER_PORTB_RIGHT_A_IIDX                           (DL_GPIO_IIDX_DIO8)
#define ENCODER_PORTB_RIGHT_A_PIN                                (DL_GPIO_PIN_8)
#define ENCODER_PORTB_RIGHT_A_IOMUX                              (IOMUX_PINCM25)
/* Defines for RIGHT_B: GPIOB.9 with pinCMx 26 on package pin 61 */
#define ENCODER_PORTB_RIGHT_B_PIN                                (DL_GPIO_PIN_9)
#define ENCODER_PORTB_RIGHT_B_IOMUX                              (IOMUX_PINCM26)
/* Port definition for Pin Group OLED_I2C */
#define OLED_I2C_PORT                                                    (GPIOB)

/* Defines for SCL: GPIOB.10 with pinCMx 27 on package pin 62 */
#define OLED_I2C_SCL_PIN                                        (DL_GPIO_PIN_10)
#define OLED_I2C_SCL_IOMUX                                       (IOMUX_PINCM27)
/* Defines for SDA: GPIOB.11 with pinCMx 28 on package pin 63 */
#define OLED_I2C_SDA_PIN                                        (DL_GPIO_PIN_11)
#define OLED_I2C_SDA_IOMUX                                       (IOMUX_PINCM28)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_ADC12_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
