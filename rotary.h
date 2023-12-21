#if defined(CH32X035)
#include <ch32x035.h>
#include <ch32x035_rcc.h>
#include <ch32x035_gpio.h>
#include <ch32x035_tim.h>
#endif
#if defined(CH32V10X)
#include <ch32v10x.h>
#include <ch32v10x_rcc.h>
#include <ch32x10x_gpio.h>
#include <ch32x10x_tim.h>
#endif
#if defined(CH32V20X)
#include <ch32v20x.h>
#include <ch32v20x_rcc.h>
#include <ch32x20x_gpio.h>
#include <ch32x20x_tim.h>
#endif
#if defined(CH32V30X)
#include <ch32v30x.h>
#include <ch32v30x_rcc.h>
#include <ch32x30x_gpio.h>
#include <ch32x30x_tim.h>
#endif

#if defined(CH32V00X)
#include <ch32v00x.h>
#include <ch32v00x_rcc.h>
#include <ch32x00x_gpio.h>
#include <ch32x00x_tim.h>
#endif

/*
states:
0 =b0, a0,
1 =b0, a1,
2 =b1, a0,
3 =b3, a3,
*/

enum
{
    SINGLE,
    DOUBLE,
};
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

volatile signed long _rotar = 0;
unsigned char _rotary_stable;
unsigned char _rotary_inverse;
// unsigned char _rotary_accel = false;
unsigned char _rotary_accel = true;
volatile unsigned long _rotary_lastpos;
volatile unsigned long _rotary_ticktime = 0;
volatile unsigned long _rotary_lasttick = 0;
// unsigned char _rotary_mode = DOUBLE;
unsigned char _rotary_mode = SINGLE;

GPIO_TypeDef *_portA;
unsigned long _pinA;
GPIO_TypeDef *_portB;
unsigned long _pinB;

void rotary_begin(GPIO_TypeDef *portA, unsigned long pinA, GPIO_TypeDef *portB, unsigned long pinB)
{
    GPIO_InitTypeDef pin;
    TIM_TimeBaseInitTypeDef tim;
    // set up the pins
    _portA = portA;
    _pinA = pinA;
    _portB = portB;
    _pinB = pinB;

    pin.GPIO_Mode = GPIO_Mode_IPU;
    pin.GPIO_Speed = GPIO_Speed_50MHz;
    pin.GPIO_Pin = pinA;
    // enable them all....

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_Init(_portA, &pin);

    pin.GPIO_Pin = pinB;
    GPIO_Init(_portB, &pin);

    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Period = SystemCoreClock / 1000; // 500x p/s
    tim.TIM_Prescaler = 0;
    tim.TIM_RepetitionCounter = 0;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInit(TIM3, &tim);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    TIM_Cmd(TIM3, ENABLE);

    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM1_UP_IRQn, 0xf0); // lowest prio?

    _rotary_stable = 3;

    _rotary_inverse = (~(_rotary_stable) & 0b00000011);

    _rotary_lastpos = _rotary_stable;
}
unsigned char rotary_hasturned()
{
    if (_rotar)
        return true;
    else
        return false;
}

signed long rotary_rotations_cl(void)
{
    signed long r = _rotar;
    // reset the rotary
    _rotar = 0;
    return r;
}

signed long rotary_rotations(void)
{
    return _rotar;
}

void rotary_pause()
{   
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
    rotary_rotations_cl();
}

void rotary_play()
{
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    // TIM_Cmd(TIM3, ENABLE);

    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM1_UP_IRQn, 0xf0); // lowest prio
}

extern void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM3_IRQHandler(void)
{
    // TIM3->INTFR &= ~(TIM_IT_Update);
    TIM3->INTFR &= 0xfffffffe;

    _rotary_ticktime++;
    // _rotar++;
    volatile unsigned long cur = 0;
    // if (!GPIO_ReadInputPin(_portA, _pinA))
    // if (!GPIO_ReadInputDataBit(_portA, _pinA))
    if (_portA->INDR & _pinA)
        cur = 1;
    // if (!GPIO_ReadInputPin(_portB, _pinB))
    // if (!GPIO_ReadInputDataBit(_portB, _pinB))
    if (_portB->INDR & _pinB)
        cur += 2;
    if (cur != _rotary_lastpos)
    {
        unsigned long add = 1;
        if ((_rotary_accel) && (_rotar))
        { // calc increment between rotations
            unsigned long tickdelta = _rotary_ticktime - _rotary_lasttick;
            // set accel vals
            // if (tickdelta < 16)
            //     add = 2;
            if (tickdelta < 100)
                add++;
            if (tickdelta < 50)
                add++;
            if (tickdelta < 20)
                add++;
            if (tickdelta < 10)
                add++;
            if (tickdelta < 5)
                add++;
        }
        if (cur == _rotary_stable) // depending on bumps in rotary??
        // if (cur == _rotary_inverse)
        {
            // increment
            // if pina is different
            if ((_rotary_lastpos & 1) != (cur & 1))
                _rotar -= add;
            // if pinb differs
            // else if (_rotary_lastpos & 2)
            else if ((_rotary_lastpos & 2) != (cur & 2))
                _rotar += add;
            // else something went wrong horrably....
        }

        // if (_rotary_mode == DOUBLE)
        // {
        //     if (cur == _rotary_inverse) // depending on bumps in rotary??
        //     {
        //         //     // increment
        //         //     // if pina is different
        //         if ((_rotary_lastpos & 1) != (cur & 1))
        //             _rotar -= add;
        //         //     // if pinb differs
        //         // else if (_rotary_lastpos & 2)
        //         else if ((_rotary_lastpos & 2) != (cur & 2))
        //             _rotar += add;
        //         //     // else something went wrong horrably....
        //     }
        // }
        _rotary_lastpos = cur;
        _rotary_lasttick = _rotary_ticktime;
    }
}