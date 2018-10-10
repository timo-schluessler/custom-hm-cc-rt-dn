#define __IO volatile
#include <stdint.h>

#define EEPROM_START 0x1000

/* SYSCFG remap */
#define SYSCFG_RMPCR2 (*(__IO uint8_t*)0x509F)

#define SYSCFG_RMPCR2_TIM2TRIGLSE_REMAP (1u<<3)

/* GPIO */

#define PA_ODR *(__IO uint8_t*)0x5000
#define PA_IDR *(__IO uint8_t*)0x5001
#define PA_DDR *(__IO uint8_t*)0x5002
#define PA_CR1 *(__IO uint8_t*)0x5003
#define PA_CR2 *(__IO uint8_t*)0x5004

#define PB_ODR *(__IO uint8_t*)0x5005
#define PB_IDR *(__IO uint8_t*)0x5006
#define PB_DDR *(__IO uint8_t*)0x5007
#define PB_CR1 *(__IO uint8_t*)0x5008
#define PB_CR2 *(__IO uint8_t*)0x5009

#define PC_ODR *(__IO uint8_t*)0x500A
#define PC_IDR *(__IO uint8_t*)0x500B
#define PC_DDR *(__IO uint8_t*)0x500C
#define PC_CR1 *(__IO uint8_t*)0x500D
#define PC_CR2 *(__IO uint8_t*)0x500E

#define PD_ODR (*(__IO uint8_t*)0x500F)
#define PD_IDR *(__IO uint8_t*)0x5010
#define PD_DDR *(__IO uint8_t*)0x5011
#define PD_CR1 *(__IO uint8_t*)0x5012
#define PD_CR2 *(__IO uint8_t*)0x5013

#define PE_ODR *(__IO uint8_t*)0x5014
#define PE_IDR *(__IO uint8_t*)0x5015
#define PE_DDR *(__IO uint8_t*)0x5016
#define PE_CR1 *(__IO uint8_t*)0x5017
#define PE_CR2 *(__IO uint8_t*)0x5018

#define PF_ODR *(__IO uint8_t*)0x5019
#define PF_IDR *(__IO uint8_t*)0x501A
#define PF_DDR *(__IO uint8_t*)0x501B
#define PF_CR1 *(__IO uint8_t*)0x501C
#define PF_CR2 *(__IO uint8_t*)0x501D

/* flash */
#define FLASH_CR1 *(__IO uint8_t*)0x5050
#define FLASH_CR2 *(__IO uint8_t*)0x5051
#define FLASH_PUKR *(__IO uint8_t*)0x5052
#define FLASH_DUKR *(__IO uint8_t*)0x5053
#define FLASH_IAPSR *(__IO uint8_t*)0x5054

#define FLASH_CR2_PRG (1u<<0)

#define FLASH_IAPSR_DUL (1u<<3)
#define FLASH_IAPSR_EOP (1u<<2)
#define FLASH_IAPSR_PUL (1u<<1)

/* external interrupt */

#define EXTI_CR1 *(__IO uint8_t*)0x50A0
#define EXTI_CR2 *(__IO uint8_t*)0x50A1
#define EXTI_CR3 *(__IO uint8_t*)0x50A2
#define EXTI_SR1 *(__IO uint8_t*)0x50A3
#define EXTI_SR2 *(__IO uint8_t*)0x50A4
#define EXTI_CONF1 *(__IO uint8_t*)0x50A5

#define EXTI_CR4 *(__IO uint8_t*)0x50AA
#define EXTI_CONF2 *(__IO uint8_t*)0x50AB

#define EXTI_SR1_P4F (1u<<4)

/* wait for event */
#define WFE_CR1 *(__IO uint8_t*)0x50A6
#define WFE_CR2 *(__IO uint8_t*)0x50A7
#define WFE_CR3 *(__IO uint8_t*)0x50A8
#define WFE_CR4 *(__IO uint8_t*)0x50A9

#define WFE_CR1_EXTI_EV3 (1u<<7)
#define WFE_CR1_EXTI_EV2 (1u<<6)
#define WFE_CR1_EXTI_EV1 (1u<<5)
#define WFE_CR1_EXTI_EV0 (1u<<4)
#define WFE_CR1_TIM1_EV1 (1u<<3)
#define WFE_CR1_TIM1_EV0 (1u<<2)
#define WFE_CR1_TIM2_EV1 (1u<<1)
#define WFE_CR1_TIM2_EV0 (1u<<0)

#define WFE_CR2_ADC1_COMP_EV (1u<<7)
#define WFE_CR2_EXTI_EVE_F (1u<<6)
#define WFE_CR2_EXTI_EVD_H (1u<<5)
#define WFE_CR2_EXTI_EVB_G (1u<<4)
#define WFE_CR2_EXTI_EV7 (1u<<3)
#define WFE_CR2_EXTI_EV6 (1u<<2)
#define WFE_CR2_EXTI_EV5 (1u<<1)
#define WFE_CR2_EXTI_EV4 (1u<<0)

#define wfe() __asm__("wfe")

/* RESET */
#define RST_CR (*(__IO uint8_t*)0x50B0)
#define RST_SR (*(__IO uint8_t*)0x50B1)
#define PWR_CSR1 (*(__IO uint8_t*)0x50B2)
#define PWR_CSR2 (*(__IO uint8_t*)0x50B3)

/* CLOCK */
#define CLK_CKDIVR	(*(__IO uint8_t*)0x50C0)
#define CLK_CRTCR	(*(__IO uint8_t*)0x50C1)
#define CLK_ICKCR	(*(__IO uint8_t*)0x50C2)
#define CLK_PCKENR1	(*(__IO uint8_t*)0x50C3)
#define CLK_PCKENR2	(*(__IO uint8_t*)0x50C4)
#define CLK_CCOR	(*(__IO uint8_t*)0x50C5)
#define CLK_ECKCR	(*(__IO uint8_t*)0x50C6)
#define CLK_SCSR	(*(__IO uint8_t*)0x50C7)
#define CLK_SWR 	(*(__IO uint8_t*)0x50C8)
#define CLK_SWCR	(*(__IO uint8_t*)0x50C9)
#define CLK_CSSR	(*(__IO uint8_t*)0x50CA)
#define CLK_CBEEPR	(*(__IO uint8_t*)0x50CB)
#define CLK_HSICALR	(*(__IO uint8_t*)0x50CC)
#define CLK_HSITRIMR	(*(__IO uint8_t*)0x50CD)
#define CLK_HSIUNLCKR	(*(__IO uint8_t*)0x50CE)
#define CLK_REGCSR	(*(__IO uint8_t*)0x50CF)

#define CLK_CRTCR_RTCSEL 1
#define CLK_CRTCR_RTCSWBSY (1<<0)

#define CLK_ECKCR_LSEON (1u<<2)
#define CLK_ECKCR_LSERDY (1u<<3)

#define CLK_PCKENR1_TIM2 (1u<<0)
#define CLK_PCKENR1_SPI (1u<<4)

#define CLK_PCKENR2_ADC1 (1u<<0)
#define CLK_PCKENR2_RTC (1u<<2)
#define CLK_PCKENR2_LCD (1u<<3)

#define CLK_ICKCR_HSION (1u<<0)

#define CLK_SWCR_SWEN (1u<<1)
#define CLK_SWCR_SWBSY (1u<<0)

#define CLK_SWR_LSE 0x08

/* --- RTC --- */
#define RTC_TR1 (*(__IO uint8_t*)0x5140)
#define RTC_TR2 (*(__IO uint8_t*)0x5141)
#define RTC_TR3 (*(__IO uint8_t*)0x5142)
#define RTC_DR1 (*(__IO uint8_t*)0x5144)
#define RTC_DR2 (*(__IO uint8_t*)0x5145)
#define RTC_DR3 (*(__IO uint8_t*)0x5146)
#define RTC_CR1 (*(__IO uint8_t*)0x5148)
#define RTC_CR2 (*(__IO uint8_t*)0x5149)
#define RTC_CR3 (*(__IO uint8_t*)0x514A)
#define RTC_ISR1 (*(__IO uint8_t*)0x514C)
#define RTC_ISR2 (*(__IO uint8_t*)0x514D)
#define RTC_SPRERH (*(__IO uint8_t*)0x5150)
#define RTC_SPRERL (*(__IO uint8_t*)0x5151)
#define RTC_APRER (*(__IO uint8_t*)0x5152)
#define RTC_WUTRH (*(__IO uint8_t*)0x5154)
#define RTC_WUTRL (*(__IO uint8_t*)0x5155)
#define RTC_SSRL (*(__IO uint8_t*)0x5157)
#define RTC_SSRH (*(__IO uint8_t*)0x5158)
#define RTC_WPR (*(__IO uint8_t*)0x5159)
#define RTC_SHIFTRH (*(__IO uint8_t*)0x515A)
#define RTC_SHIFTRL (*(__IO uint8_t*)0x515B)
#define RTC_ALRMAR1 (*(__IO uint8_t*)0x515C)
#define RTC_ALRMAR2 (*(__IO uint8_t*)0x515D)
#define RTC_ALRMAR3 (*(__IO uint8_t*)0x515E)
#define RTC_ALRMAR4 (*(__IO uint8_t*)0x515F)
#define RTC_ALRMASSRH (*(__IO uint8_t*)0x5164)
#define RTC_ALRMASSRL (*(__IO uint8_t*)0x5165)
#define RTC_ALRMASSMSKR (*(__IO uint8_t*)0x5166)
#define RTC_CALRH (*(__IO uint8_t*)0x516A)
#define RTC_CALRL (*(__IO uint8_t*)0x516B)
#define RTC_TCR1 (*(__IO uint8_t*)0x516C)
#define RTC_TCR2 (*(__IO uint8_t*)0x516D)

#define RTC_CR1_WUCKSEL (1u<<0)

#define RTC_CR2_WUTE (1u<<2)
#define RTC_CR2_WUTIE (1u<<6)

#define RTC_ISR1_WUTWF (1u<<2)
#define RTC_ISR1_INITF (1u<<6)
#define RTC_ISR1_INIT (1u<<7)

#define RTC_ISR2_WUTF (1u<<2)

/* --- LCD --- */
#define LCD_CR1 (*(__IO uint8_t*)0x5400)
#define LCD_CR2 (*(__IO uint8_t*)0x5401)
#define LCD_CR3 (*(__IO uint8_t*)0x5402)
#define LCD_FRQ (*(__IO uint8_t*)0x5403)
#define LCD_PM0 (*(__IO uint8_t*)0x5404)
#define LCD_PM1 (*(__IO uint8_t*)0x5405)
#define LCD_PM2 (*(__IO uint8_t*)0x5406)
#define LCD_PM3 (*(__IO uint8_t*)0x5407)
#define LCD_PM4 (*(__IO uint8_t*)0x5408)
#define LCD_PM5 (*(__IO uint8_t*)0x5409)

#define LCD_RAM ((__IO uint8_t*)0x540C)
#define LCD_RAM_SIZE 22

#define LCD_CR4 (*(__IO uint8_t*)0x542F)

#define LCD_CR3_LCDEN (1u<<6)
#define LCD_CR3_SOF (1u<<4)
#define LCD_CR3_SOFC (1u<<3)

/* ------------------- USART ------------------- */
#define USART1_SR *(unsigned char*)0x5230
#define USART1_DR *(unsigned char*)0x5231
#define USART1_BRR1 *(unsigned char*)0x5232
#define USART1_BRR2 *(unsigned char*)0x5233
#define USART1_CR1 *(unsigned char*)0x5234
#define USART1_CR2 *(unsigned char*)0x5235
#define USART1_CR3 *(unsigned char*)0x5236
#define USART1_CR4 *(unsigned char*)0x5237
#define USART1_CR5 *(unsigned char*)0x5238
#define USART1_GTR *(unsigned char*)0x5239
#define USART1_PSCR *(unsigned char*)0x523A

/* USART_CR1 bits */
#define USART_CR1_R8 (1 << 7)
#define USART_CR1_T8 (1 << 6)
#define USART_CR1_UARTD (1 << 5)
#define USART_CR1_M (1 << 4)
#define USART_CR1_WAKE (1 << 3)
#define USART_CR1_PCEN (1 << 2)
#define USART_CR1_PS (1 << 1)
#define USART_CR1_PIEN (1 << 0)

/* USART_CR2 bits */
#define USART_CR2_TIEN (1 << 7)
#define USART_CR2_TCIEN (1 << 6)
#define USART_CR2_RIEN (1 << 5)
#define USART_CR2_ILIEN (1 << 4)
#define USART_CR2_TEN (1 << 3)
#define USART_CR2_REN (1 << 2)
#define USART_CR2_RWU (1 << 1)
#define USART_CR2_SBK (1 << 0)

/* USART_CR3 bits */
#define USART_CR3_LINEN (1 << 6)
#define USART_CR3_STOP2 (1 << 5)
#define USART_CR3_STOP1 (1 << 4)
#define USART_CR3_CLKEN (1 << 3)
#define USART_CR3_CPOL (1 << 2)
#define USART_CR3_CPHA (1 << 1)
#define USART_CR3_LBCL (1 << 0)

/* USART_SR bits */
#define USART_SR_TXE (1 << 7)
#define USART_SR_TC (1 << 6)
#define USART_SR_RXNE (1 << 5)
#define USART_SR_IDLE (1 << 4)
#define USART_SR_OR (1 << 3)
#define USART_SR_NF (1 << 2)
#define USART_SR_FE (1 << 1)
#define USART_SR_PE (1 << 0)


/* ------------------- TIMERS ------------------- */
#define TIM1_CR1 (*(__IO unsigned char*)0x52B0)
#define TIM1_CR2 (*(__IO unsigned char*)0x52B1)
#define TIM1_SMCR (*(__IO unsigned char*)0x52B2)
#define TIM1_ETR (*(__IO unsigned char*)0x52B3)
#define TIM1_DER (*(__IO unsigned char*)0x52B4)
#define TIM1_IER (*(__IO unsigned char*)0x52B5)
#define TIM1_SR1 (*(__IO unsigned char*)0x52B6)
#define TIM1_SR2 (*(__IO unsigned char*)0x52B7)
#define TIM1_EGR (*(__IO unsigned char*)0x52B8)
#define TIM1_CCMR1 (*(__IO unsigned char*)0x52B9)
#define TIM1_CCMR2 (*(__IO unsigned char*)0x52BA)
#define TIM1_CCMR3 (*(__IO unsigned char*)0x52BB)
#define TIM1_CCMR4 (*(__IO unsigned char*)0x52BC)
#define TIM1_CCER1 (*(__IO unsigned char*)0x52BD)
#define TIM1_CCER2 (*(__IO unsigned char*)0x52BE)
#define TIM1_CNTRH (*(__IO unsigned char*)0x52BF)
#define TIM1_CNTRL (*(__IO unsigned char*)0x52C0)
#define TIM1_PSCRH (*(__IO unsigned char*)0x52C1)
#define TIM1_PSCRL (*(__IO unsigned char*)0x52C2)
#define TIM1_ARRH (*(__IO unsigned char*)0x52C3)
#define TIM1_ARRL (*(__IO unsigned char*)0x52C4)
#define TIM1_RCR (*(__IO unsigned char*)0x52C5)
#define TIM1_CCR1H (*(__IO unsigned char*)0x52C6)
#define TIM1_CCR1L (*(__IO unsigned char*)0x52C7)
#define TIM1_CCR2H (*(__IO unsigned char*)0x52C8)
#define TIM1_CCR2L (*(__IO unsigned char*)0x52C9)
#define TIM1_CCR3H (*(__IO unsigned char*)0x52CA)
#define TIM1_CCR3L (*(__IO unsigned char*)0x52CB)
#define TIM1_CCR4H (*(__IO unsigned char*)0x52CC)
#define TIM1_CCR4L (*(__IO unsigned char*)0x52CD)
#define TIM1_BKR (*(__IO unsigned char*)0x52CE)
#define TIM1_DTR (*(__IO unsigned char*)0x52CF)
#define TIM1_OISR (*(__IO unsigned char*)0x52D0)
#define TIM1_DCR1 (*(__IO unsigned char*)0x52D1)
#define TIM1_DCR2 (*(__IO unsigned char*)0x52D2)
#define TIM1_DMA1R (*(__IO unsigned char*)0x52D3)

/* TIM_IER bits */
#define TIM_IER_BIE (1 << 7)
#define TIM_IER_TIE (1 << 6)
#define TIM_IER_COMIE (1 << 5)
#define TIM_IER_CC4IE (1 << 4)
#define TIM_IER_CC3IE (1 << 3)
#define TIM_IER_CC2IE (1 << 2)
#define TIM_IER_CC1IE (1 << 1)
#define TIM_IER_UIE (1 << 0)

/* TIM_CR1 bits */
#define TIM_CR1_APRE (1 << 7)
#define TIM_CR1_CMSH (1 << 6)
#define TIM_CR1_CMSL (1 << 5)
#define TIM_CR1_DIR (1 << 4)
#define TIM_CR1_OPM (1 << 3)
#define TIM_CR1_URS (1 << 2)
#define TIM_CR1_UDIS (1 << 1)
#define TIM_CR1_CEN (1 << 0)

/* TIM_SR1 bits */
#define TIM_SR1_BIF (1 << 7)
#define TIM_SR1_TIF (1 << 6)
#define TIM_SR1_COMIF (1 << 5)
#define TIM_SR1_CC4IF (1 << 4)
#define TIM_SR1_CC3IF (1 << 3)
#define TIM_SR1_CC2IF (1 << 2)
#define TIM_SR1_CC1IF (1u << 1)
#define TIM_SR1_UIF (1 << 0)


#define TIM2_CR1 (*(__IO uint8_t*)0x5250)
#define TIM2_CR2 (*(__IO uint8_t*)0x5251)
#define TIM2_SMCR (*(__IO uint8_t*)0x5252)
#define TIM2_ETR (*(__IO uint8_t*)0x5253)
#define TIM2_DER (*(__IO uint8_t*)0x5254)
#define TIM2_IER (*(__IO uint8_t*)0x5255)
#define TIM2_SR1 (*(__IO uint8_t*)0x5256)
#define TIM2_SR2 (*(__IO uint8_t*)0x5257)
#define TIM2_EGR (*(__IO uint8_t*)0x5258)
#define TIM2_CCMR1 (*(__IO uint8_t*)0x5259)
#define TIM2_CCMR2 (*(__IO uint8_t*)0x525A)
#define TIM2_CCER1 (*(__IO uint8_t*)0x525B)
#define TIM2_CNTRH (*(__IO uint8_t*)0x525C)
#define TIM2_CNTRL (*(__IO uint8_t*)0x525D)
#define TIM2_PSCR (*(__IO uint8_t*)0x525E)
#define TIM2_ARRH (*(__IO uint8_t*)0x525F)
#define TIM2_ARRL (*(__IO uint8_t*)0x5260)
#define TIM2_CCR1H (*(__IO uint8_t*)0x5261)
#define TIM2_CCR1L (*(__IO uint8_t*)0x5262)
#define TIM2_CCR2H (*(__IO uint8_t*)0x5263)
#define TIM2_CCR2L (*(__IO uint8_t*)0x5264)
#define TIM2_BKR (*(__IO uint8_t*)0x5265)
#define TIM2_OISR (*(__IO uint8_t*)0x5266)

#define TIMx_CR1_CEN 1
#define TIMx_SMCR_TS 4
#define TIMx_SMCR_SMS 0
#define TIMx_ETR_ETPS 4
#define TIMx_IER_CC2IE (1u<<2)
#define TIMx_IER_CC1IE (1u<<1)
#define TIMx_SR1_CC1IF (1u<<1)


/* *** SPI *** */
#define SPI1_CR1 (*(__IO uint8_t*)0x5200)
#define SPI1_CR2 (*(__IO uint8_t*)0x5201)
#define SPI1_ICR (*(__IO uint8_t*)0x5202)
#define SPI1_SR (*(__IO uint8_t*)0x5203)
#define SPI1_DR (*(__IO uint8_t*)0x5204)
#define SPI1_CRCPR (*(__IO uint8_t*)0x5205)
#define SPI1_RXCRCR (*(__IO uint8_t*)0x5206)
#define SPI1_TXCRCR (*(__IO uint8_t*)0x5207)

#define SPI_CR1_MSTR (1u<<2)
#define SPI_CR1_BR (1u<<3)
#define SPI_CR1_SPE (1u<<6)
#define SPI_CR1_LSBFIRST (1u<<7)

#define SPI_CR2_SSI (1u<<0)
#define SPI_CR2_SSM (1u<<1)
#define SPI_CR2_RXOnly (1u<<2)
#define SPI_CR2_CRCNEXT (1u<<4)
#define SPI_CR2_CRCEN (1u<<5)
#define SPI_CR2_BDOE (1u<<6)
#define SPI_CR2_BDM (1u<<7)

#define SPI_SR_RXNE (1u<<0)
#define SPI_SR_TXE (1u<<1)
#define SPI_SR_WKUP (1u<<3)
#define SPI_SR_CRCERR (1u<<4)
#define SPI_SR_MODF (1u<<5)
#define SPI_SR_OVR (1u<<6)
#define SPI_SR_BSY (1u<<7)

/* *** ADC1 *** */
#define ADC1_CR1 (*(__IO uint8_t*)0x5340)
#define ADC1_CR2 (*(__IO uint8_t*)0x5341)
#define ADC1_CR3 (*(__IO uint8_t*)0x5342)
#define ADC1_SR (*(__IO uint8_t*)0x5343)
#define ADC1_DRH (*(__IO uint8_t*)0x5344)
#define ADC1_DRL (*(__IO uint8_t*)0x5345)
#define ADC1_HTRH (*(__IO uint8_t*)0x5346)
#define ADC1_HTRL (*(__IO uint8_t*)0x5347)
#define ADC1_LTRH (*(__IO uint8_t*)0x5348)
#define ADC1_LTRL (*(__IO uint8_t*)0x5349)
#define ADC1_SQR1 (*(__IO uint8_t*)0x534A)
#define ADC1_SQR2 (*(__IO uint8_t*)0x534B)
#define ADC1_SQR3 (*(__IO uint8_t*)0x534C)
#define ADC1_SQR4 (*(__IO uint8_t*)0x534D)
#define ADC1_TRIGR1 (*(__IO uint8_t*)0x534E)
#define ADC1_TRIGR2 (*(__IO uint8_t*)0x534F)
#define ADC1_TRIGR3 (*(__IO uint8_t*)0x5350)
#define ADC1_TRIGR4 (*(__IO uint8_t*)0x5351)

#define ADC_CR1_ADON (1u<<0)
#define ADC_CR1_START (1u<<1)
#define ADC_CR1_CONT (1u<<2)
#define ADC_CR1_EOCIE (1u<<3)
#define ADC_CR1_AWDIE (1u<<4)
#define ADC_CR1_RES0 (1u<<5)
#define ADC_CR1_RES1 (1u<<6)
#define ADC_CR1_OVERIE (1u<<7)

#define ADC_CR2_SMTP1 (1u<<0)
#define ADC_CR2_EXTSEL0 (1u<<3)
#define ADC_CR2_EXTSEL1 (1u<<4)
#define ADC_CR2_TRIG_EDGE0 (1u<<5)
#define ADC_CR2_TRIG_EDGE1 (1u<<6)
#define ADC_CR2_PRESC (1u<<7)

#define ADC_CR3_CHSEL (1u<<0)
#define ADC_CR3_SMTP2 (1u<<5)

#define ADC_SR_EOC (1u<<0)
#define ADC_SR_AWD (1u<<1)
#define ADC_SR_OVER (1u<<2)

#define ADC_SQR1_CHSEL_S24 (1u<<0)
#define ADC_SQR1_CHSEL_S25 (1u<<1)
#define ADC_SQR1_CHSEL_S26 (1u<<2)
#define ADC_SQR1_CHSEL_S27 (1u<<3)
#define ADC_SQR1_CHSEL_SVREFINT (1u<<4)
#define ADC_SQR1_CHSEL_STS (1u<<5)
#define ADC_SQR1_DMAOFF (1u<<7)

#define ADC_TRIGR1_TRIG24 (1u<<0)
#define ADC_TRIGR1_TRIG25 (1u<<1)
