/// 2023 guus. Very redementry arduino like adc analogRead.
// does not use dma, itc or anything fancy, so probably not (by far) as fast or effecient as could be.


#if defined(CH32V00X)
#include <ch32v00x.h>
#include <ch32v00x_gpio.h>
#include <ch32v00x_rcc.h>
#include <ch32v00x_adc.h>
#endif

#if defined(CH32X035)
#include <ch32x035.h>
#include <ch32x035_gpio.h>
#include <ch32x035_rcc.h>
#include <ch32x035_adc.h>
#endif


#if defined(CH32V10X)
#include <ch32v10x.h>
#include <ch32v10x_gpio.h>
#include <ch32v10x_rcc.h>
#include <ch32v10x_adc.h>
#endif

#if defined(CH32V20X)
#include <ch32v20x.h>
#include <ch32v20x_gpio.h>
#include <ch32v20x_rcc.h>
#include <ch32v20x_adc.h>
#endif

#if defined(CH32V30X)
#include <ch32v30x.h>
#include <ch32v30x_gpio.h>
#include <ch32v30x_rcc.h>
#include <ch32v30x_adc.h>
#endif


void analogSetConf(unsigned char chan)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;
    GPIO_TypeDef *port;
    unsigned long pin;
    unsigned long perclk;

#if defined(CH32V00X)
    switch (chan)
    {
    case 0:
        perclk = RCC_APB2Periph_GPIOA;
        port = GPIOA;
        pin = GPIO_Pin_2;
        chan = 0x01 << chan;
        break;

    case 1:
        perclk = RCC_APB2Periph_GPIOA;
        port = GPIOA;
        pin = GPIO_Pin_1;
        chan = 0x01 << chan;
        break;

    case 2:
        perclk = RCC_APB2Periph_GPIOC;
        port = GPIOC;
        pin = GPIO_Pin_4;
        chan = 0x01 << chan;
        break;

    case 3:
        perclk = RCC_APB2Periph_GPIOD;
        port = GPIOD;
        pin = GPIO_Pin_2;
        chan = 0x01 << chan;
        break;

    case 4:
        perclk = RCC_APB2Periph_GPIOD;
        port = GPIOD;
        pin = GPIO_Pin_3;
        chan = 0x01 << chan;
        break;

    case 5:
        perclk = RCC_APB2Periph_GPIOD;
        port = GPIOD;
        pin = GPIO_Pin_5;
        chan = 0x01 << chan;
        break;

    case 6:
        perclk = RCC_APB2Periph_GPIOD;
        port = GPIOD;
        pin = GPIO_Pin_6;
        chan = 0x01 << chan;
        break;

    case 7:
        perclk = RCC_APB2Periph_GPIOD;
        port = GPIOD;
        pin = GPIO_Pin_4;
        chan = 0x01 << chan;
        break;
    default:
        perclk = RCC_APB2Periph_GPIOA;
        port = GPIOA;
        pin = GPIO_Pin_2;
        chan = 0x01 << chan;
        break;
    }

#else
    // gpio A
    if (chan < 8)
    {
        port = GPIOA;
        perclk = RCC_APB2Periph_GPIOA;
        pin = 0x01 << chan;
        chan = pin;
    }
    else if (chan < 10)
    {
        port = GPIOB;
        perclk = RCC_APB2Periph_GPIOB;
        pin = 0x01 << (chan - 8);
        chan = 0x01 << chan;
    }
    else
    {
        port = GPIOC;
        perclk = RCC_APB2Periph_GPIOC;
        pin = 0x01 << (chan - 10);
        chan = 0x01 << chan;
    }

#endif

    gpio.GPIO_Mode = GPIO_Mode_AIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = pin;

    RCC_APB2PeriphClockCmd(perclk, ENABLE);
    GPIO_Init(port, &gpio);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_DeInit(ADC1);

    ADC_CLKConfig(ADC1, ADC_CLK_Div4);

    adc.ADC_DataAlign = ADC_DataAlign_Right; /* Right-alignment for converted data */
    adc.ADC_ScanConvMode = DISABLE;          /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */

    adc.ADC_Mode = ADC_Mode_Independent; /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_NbrOfChannel = 1;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_OutputBuffer = ENABLE;

    ADC_Init(ADC1, &adc);
    ADC_Cmd(ADC1, ENABLE);
    ADC_RegularChannelConfig(ADC1, chan, 1, ADC_SampleTime_4Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

unsigned short analogRead12(unsigned char chan)
{
    unsigned short val;
    analogSetConf(chan);

    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
        ;
    val = ADC1->RDATAR;
    ADC_Cmd(ADC1, DISABLE);
    ADC_DeInit(ADC1);
    return (val);
}

unsigned short analogRead(unsigned char chan)
{
#if defined(CH32V00X)
    return (analogRead12(chan));
#else
    return (analogRead12(chan) >> 2);
#endif
}