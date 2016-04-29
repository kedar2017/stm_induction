/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>        /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>       /* HiTech General Include File */
#elif defined(__18CXX)
    #include <p18cxxx.h>   /* C18 General Include File */
#endif

#if defined(__XC) || defined(HI_TECH_C)

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#endif

#include "main.h"
#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "pwm.h"           /* User funct/params, such as InitApp */

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

/* i.e. uint8_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

void main(void)
{
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitMotorPWM();

    /* TODO <INSERT USER APPLICATION CODE HERE> */

    while(1)
    {
        if (TMR2IF) {

            PORTBbits.RB1 = 1;

            UpdatePWM1(); /* Updates Echanced PWM 1 duty-cycle */
            UpdatePWM2(); /* Updates Echanced PWM 2 duty-cycle */
            UpdatePWM3(); /* Updates Echanced PWM 3 duty-cycle */

            TMR2IF = 0; /* Clear Interrupt Flag */

            PORTBbits.RB1 = 0;
        }
    }
}