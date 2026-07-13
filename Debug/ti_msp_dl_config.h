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



/* Defines for PWM_LEFT */
#define PWM_LEFT_INST                                                      TIMG7
#define PWM_LEFT_INST_IRQHandler                                TIMG7_IRQHandler
#define PWM_LEFT_INST_INT_IRQN                                  (TIMG7_INT_IRQn)
#define PWM_LEFT_INST_CLK_FREQ                                           4000000
/* GPIO defines for channel 1 */
#define GPIO_PWM_LEFT_C1_PORT                                              GPIOA
#define GPIO_PWM_LEFT_C1_PIN                                       DL_GPIO_PIN_7
#define GPIO_PWM_LEFT_C1_IOMUX                                   (IOMUX_PINCM14)
#define GPIO_PWM_LEFT_C1_IOMUX_FUNC                  IOMUX_PINCM14_PF_TIMG7_CCP1
#define GPIO_PWM_LEFT_C1_IDX                                 DL_TIMER_CC_1_INDEX

/* Defines for PWM_RIGHT */
#define PWM_RIGHT_INST                                                    TIMG12
#define PWM_RIGHT_INST_IRQHandler                              TIMG12_IRQHandler
#define PWM_RIGHT_INST_INT_IRQN                                (TIMG12_INT_IRQn)
#define PWM_RIGHT_INST_CLK_FREQ                                          4000000
/* GPIO defines for channel 1 */
#define GPIO_PWM_RIGHT_C1_PORT                                             GPIOB
#define GPIO_PWM_RIGHT_C1_PIN                                     DL_GPIO_PIN_14
#define GPIO_PWM_RIGHT_C1_IOMUX                                  (IOMUX_PINCM31)
#define GPIO_PWM_RIGHT_C1_IOMUX_FUNC                IOMUX_PINCM31_PF_TIMG12_CCP1
#define GPIO_PWM_RIGHT_C1_IDX                                DL_TIMER_CC_1_INDEX



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





/* Defines for TRACKING_ADC0 */
#define TRACKING_ADC0_INST                                                  ADC0
#define TRACKING_ADC0_INST_IRQHandler                            ADC0_IRQHandler
#define TRACKING_ADC0_INST_INT_IRQN                              (ADC0_INT_IRQn)
#define TRACKING_ADC0_ADCMEM_0                                DL_ADC12_MEM_IDX_0
#define TRACKING_ADC0_ADCMEM_0_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC0_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC0_ADCMEM_1                                DL_ADC12_MEM_IDX_1
#define TRACKING_ADC0_ADCMEM_1_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC0_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC0_ADCMEM_2                                DL_ADC12_MEM_IDX_2
#define TRACKING_ADC0_ADCMEM_2_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC0_ADCMEM_2_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC0_ADCMEM_3                                DL_ADC12_MEM_IDX_3
#define TRACKING_ADC0_ADCMEM_3_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC0_ADCMEM_3_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC0_ADCMEM_4                                DL_ADC12_MEM_IDX_4
#define TRACKING_ADC0_ADCMEM_4_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC0_ADCMEM_4_REF_VOLTAGE_V                                     3.3
#define GPIO_TRACKING_ADC0_C12_PORT                                        GPIOA
#define GPIO_TRACKING_ADC0_C12_PIN                                DL_GPIO_PIN_14
#define GPIO_TRACKING_ADC0_IOMUX_C12                             (IOMUX_PINCM36)
#define GPIO_TRACKING_ADC0_IOMUX_C12_FUNC         (IOMUX_PINCM36_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC0_C6_PORT                                         GPIOB
#define GPIO_TRACKING_ADC0_C6_PIN                                 DL_GPIO_PIN_20
#define GPIO_TRACKING_ADC0_IOMUX_C6                              (IOMUX_PINCM48)
#define GPIO_TRACKING_ADC0_IOMUX_C6_FUNC          (IOMUX_PINCM48_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC0_C4_PORT                                         GPIOB
#define GPIO_TRACKING_ADC0_C4_PIN                                 DL_GPIO_PIN_25
#define GPIO_TRACKING_ADC0_IOMUX_C4                              (IOMUX_PINCM56)
#define GPIO_TRACKING_ADC0_IOMUX_C4_FUNC          (IOMUX_PINCM56_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC0_C2_PORT                                         GPIOA
#define GPIO_TRACKING_ADC0_C2_PIN                                 DL_GPIO_PIN_25
#define GPIO_TRACKING_ADC0_IOMUX_C2                              (IOMUX_PINCM55)
#define GPIO_TRACKING_ADC0_IOMUX_C2_FUNC          (IOMUX_PINCM55_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC0_C0_PORT                                         GPIOA
#define GPIO_TRACKING_ADC0_C0_PIN                                 DL_GPIO_PIN_27
#define GPIO_TRACKING_ADC0_IOMUX_C0                              (IOMUX_PINCM60)
#define GPIO_TRACKING_ADC0_IOMUX_C0_FUNC          (IOMUX_PINCM60_PF_UNCONNECTED)

/* Defines for TRACKING_ADC1 */
#define TRACKING_ADC1_INST                                                  ADC1
#define TRACKING_ADC1_INST_IRQHandler                            ADC1_IRQHandler
#define TRACKING_ADC1_INST_INT_IRQN                              (ADC1_INT_IRQn)
#define TRACKING_ADC1_ADCMEM_0                                DL_ADC12_MEM_IDX_0
#define TRACKING_ADC1_ADCMEM_0_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC1_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC1_ADCMEM_1                                DL_ADC12_MEM_IDX_1
#define TRACKING_ADC1_ADCMEM_1_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC1_ADCMEM_1_REF_VOLTAGE_V                                     3.3
#define TRACKING_ADC1_ADCMEM_2                                DL_ADC12_MEM_IDX_2
#define TRACKING_ADC1_ADCMEM_2_REF               DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define TRACKING_ADC1_ADCMEM_2_REF_VOLTAGE_V                                     3.3
#define GPIO_TRACKING_ADC1_C6_PORT                                         GPIOB
#define GPIO_TRACKING_ADC1_C6_PIN                                 DL_GPIO_PIN_19
#define GPIO_TRACKING_ADC1_IOMUX_C6                              (IOMUX_PINCM45)
#define GPIO_TRACKING_ADC1_IOMUX_C6_FUNC          (IOMUX_PINCM45_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC1_C4_PORT                                         GPIOB
#define GPIO_TRACKING_ADC1_C4_PIN                                 DL_GPIO_PIN_17
#define GPIO_TRACKING_ADC1_IOMUX_C4                              (IOMUX_PINCM43)
#define GPIO_TRACKING_ADC1_IOMUX_C4_FUNC          (IOMUX_PINCM43_PF_UNCONNECTED)
#define GPIO_TRACKING_ADC1_C1_PORT                                         GPIOA
#define GPIO_TRACKING_ADC1_C1_PIN                                 DL_GPIO_PIN_16
#define GPIO_TRACKING_ADC1_IOMUX_C1                              (IOMUX_PINCM38)
#define GPIO_TRACKING_ADC1_IOMUX_C1_FUNC          (IOMUX_PINCM38_PF_UNCONNECTED)



/* Port definition for Pin Group RUN_LED */
#define RUN_LED_PORT                                                     (GPIOA)

/* Defines for LED: GPIOA.15 with pinCMx 37 on package pin 8 */
#define RUN_LED_LED_PIN                                         (DL_GPIO_PIN_15)
#define RUN_LED_LED_IOMUX                                        (IOMUX_PINCM37)
/* Port definition for Pin Group TB6612_PORTA */
#define TB6612_PORTA_PORT                                                (GPIOA)

/* Defines for AIN1: GPIOA.13 with pinCMx 35 on package pin 6 */
#define TB6612_PORTA_AIN1_PIN                                   (DL_GPIO_PIN_13)
#define TB6612_PORTA_AIN1_IOMUX                                  (IOMUX_PINCM35)
/* Port definition for Pin Group TB6612_PORTB */
#define TB6612_PORTB_PORT                                                (GPIOB)

/* Defines for AIN2: GPIOB.10 with pinCMx 27 on package pin 62 */
#define TB6612_PORTB_AIN2_PIN                                   (DL_GPIO_PIN_10)
#define TB6612_PORTB_AIN2_IOMUX                                  (IOMUX_PINCM27)
/* Defines for BIN1: GPIOB.9 with pinCMx 26 on package pin 61 */
#define TB6612_PORTB_BIN1_PIN                                    (DL_GPIO_PIN_9)
#define TB6612_PORTB_BIN1_IOMUX                                  (IOMUX_PINCM26)
/* Defines for BIN2: GPIOB.6 with pinCMx 23 on package pin 58 */
#define TB6612_PORTB_BIN2_PIN                                    (DL_GPIO_PIN_6)
#define TB6612_PORTB_BIN2_IOMUX                                  (IOMUX_PINCM23)
/* Port definition for Pin Group ENCODER_PORTB */
#define ENCODER_PORTB_PORT                                               (GPIOB)

/* Defines for LEFT_A: GPIOB.4 with pinCMx 17 on package pin 52 */
// pins affected by this interrupt request:["LEFT_A","RIGHT_A"]
#define ENCODER_PORTB_INT_IRQN                                  (GPIOB_INT_IRQn)
#define ENCODER_PORTB_INT_IIDX                  (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENCODER_PORTB_LEFT_A_IIDX                            (DL_GPIO_IIDX_DIO4)
#define ENCODER_PORTB_LEFT_A_PIN                                 (DL_GPIO_PIN_4)
#define ENCODER_PORTB_LEFT_A_IOMUX                               (IOMUX_PINCM17)
/* Defines for LEFT_B: GPIOB.5 with pinCMx 18 on package pin 53 */
#define ENCODER_PORTB_LEFT_B_PIN                                 (DL_GPIO_PIN_5)
#define ENCODER_PORTB_LEFT_B_IOMUX                               (IOMUX_PINCM18)
/* Defines for RIGHT_A: GPIOB.11 with pinCMx 28 on package pin 63 */
#define ENCODER_PORTB_RIGHT_A_IIDX                          (DL_GPIO_IIDX_DIO11)
#define ENCODER_PORTB_RIGHT_A_PIN                               (DL_GPIO_PIN_11)
#define ENCODER_PORTB_RIGHT_A_IOMUX                              (IOMUX_PINCM28)
/* Defines for RIGHT_B: GPIOB.12 with pinCMx 29 on package pin 64 */
#define ENCODER_PORTB_RIGHT_B_PIN                               (DL_GPIO_PIN_12)
#define ENCODER_PORTB_RIGHT_B_IOMUX                              (IOMUX_PINCM29)
/* Port definition for Pin Group OLED_I2C */
#define OLED_I2C_PORT                                                    (GPIOA)

/* Defines for SCL: GPIOA.1 with pinCMx 2 on package pin 34 */
#define OLED_I2C_SCL_PIN                                         (DL_GPIO_PIN_1)
#define OLED_I2C_SCL_IOMUX                                        (IOMUX_PINCM2)
/* Defines for SDA: GPIOA.0 with pinCMx 1 on package pin 33 */
#define OLED_I2C_SDA_PIN                                         (DL_GPIO_PIN_0)
#define OLED_I2C_SDA_IOMUX                                        (IOMUX_PINCM1)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_LEFT_init(void);
void SYSCFG_DL_PWM_RIGHT_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_TRACKING_ADC0_init(void);
void SYSCFG_DL_TRACKING_ADC1_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
