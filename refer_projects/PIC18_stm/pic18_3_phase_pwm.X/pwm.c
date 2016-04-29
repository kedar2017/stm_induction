/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <p18cxxx.h>
#include <pps.h>       
#include <stdint.h>       // Includes uint16_t definition

#include "main.h"
#include "system.h"       /* System funct/params, like osc/peripheral config */
#include "pwm.h"          /* User funct/params, such as InitApp */

/******************************************************************************/
/* Macros                                                                     */
/******************************************************************************/

#define FPWM 20000      // 20 kHz
#define _DES_FREQ 20    // 60 Hz sine wave is required
#define _DELTA_PHASE (unsigned int)((_DES_FREQ * 65536 / FPWM))
#define _120_DEGREES 0x5555
#define _240_DEGREES 0xAAAA

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

uint16_t    Phase, Delta_Phase, Phase_Offset;
uint16_t     Multiplier, Result;

int SineTable [64] = {
         0,3212,6393,9512,12539,15446,18204,20787,23170,25329,
         27245,28898,30273,31356,32137,32609,32767,32609,32137,31356,30273,28898,
         27245,25329,23170,20787,18204,15446,12539,9512,6393,3212,0,-3212,-6393,
         -9512,-12539,-15446,-18204,-20787,-23170,-25329,-27245,-28898,-30273,
         -31356,-32137,-32609,-32767,-32609,-32137,-31356,-30273,-28898,-27245,
         -25329,-23170,-20787,-18204,-15446,-12539,-9512,-6393,-3212
};

/******************************************************************************/
/* Private Functions                                                          */
/******************************************************************************/

void InitTMR2(void);

void InitPWM1(void);
void InitPWM2(void);
void InitPWM3(void);

/******************************************************************************/
/* PWM Function                                                               */
/******************************************************************************/

/**
 * Initializes three-phase PWM for IGBT control. Requires CCP6/7/8 pins and uses
 * Timer0/Timer2 timers.
 */
void InitMotorPWM(void)
{
    Phase = 0; // Reset Phase Variable
    Delta_Phase = _DELTA_PHASE; // Initialize Phase increment for 60Hz sine wave
    
    // Map pins
    TRISB = 0x00;                       // set as output
    
    PPSUnLock();
    PPSOutput(PPS_RP10,PPS_CCP1P1A);    // pin 28
    PPSOutput(PPS_RP9,PPS_CCP2P2A);     // pin 27
    PPSOutput(PPS_RP8,PPS_CCP3P3A);     // pin 26
    PPSOutput(PPS_RP7,PPS_P1B);         // pin 25
    PPSOutput(PPS_RP6,PPS_P2B);         // pin 24
    PPSOutput(PPS_RP5,PPS_P3B);         // pin 23
    PPSLock();

    // Interrupts
    INTCONbits.GIE_GIEH = 0b1;  // Enables all high-priority/unmasked interrupts
    INTCONbits.PEIE_GIEL = 0b1; // Enables all low-priority/unmasked periphereal interrupts


    // Init individual periphereals
    InitTMR2();

    InitPWM1();
    InitPWM2();
    InitPWM3();
}

/**
 *  Initializes Timer2, which controls PWM 6 / PWM 7 / PWM 8 @ 20 kHz
 */
void InitTMR2(void)
{
    // Fsys = 48 MHz
    T2CONbits.T2OUTPS = 0b0000; // 1:1 Postscale
    T2CONbits.T2CKPS = 0b01;    // Prescaler is 4
    T2CONbits.TMR2ON = 0b1;     // Set Timer2 on

    PR2 = 149; // Interreputs every 20 kHz - increasing PR2 decreases frequency

    // F = Fsys/4/(PR2 + 1)/TMR2 Prescaler
}

/**
 * Initializes Enhanced PWM 1
 */
void InitPWM1(void)
{
    CCPR1L = 0b10000000;              // eight MSB of 10-bit PWM duty cycle
    CCP1CONbits.DC1B = 0b00;          // two LSB of 10-bit PWM duty cycle

    CCP1CONbits.P1M = 0b10;           // Half-bridge operation (PxA, PxB modulated with dead-band control)
    CCP1CONbits.CCP1M = 0b1100;       // PWM mode (active = high)
    CCPTMRS0bits.C1TSEL = 0b000;      // ECCP1 PWM mode is based off of TMR2
    ECCP1DELbits.P1DC = 4;            // 2 us of deadtime (1/2/200000)
}

/**
 * Initializes Enhanced PWM 2
 */
void InitPWM2(void)
{
    CCPR2L = 0b10000000;              // eight MSB of 10-bit PWM duty cycle
    CCP2CONbits.DC2B = 0b00;          // two LSB of 10-bit PWM duty cycle

    CCP2CONbits.P2M = 0b10;           // Half-bridge operation (PxA, PxB modulated with dead-band control)
    CCP2CONbits.CCP2M = 0b1100;       // PWM mode (active = high)
    CCPTMRS0bits.C2TSEL = 0b000;      // ECCP2 PWM mode is based off of TMR2
    ECCP2DELbits.P2DC = 4;            // 2 us of deadtime (1/2/200000)
}

/**
 * Initializes Enhanced PWM 3
 */
void InitPWM3(void)
{
    CCPR3L = 0b10000000;              // eight MSB of 10-bit PWM duty cycle
    CCP3CONbits.DC3B = 0b00;          // two LSB of 10-bit PWM duty cycle

    CCP3CONbits.P3M = 0b10;           // Half-bridge operation (PxA, PxB modulated with dead-band control)
    CCP3CONbits.CCP3M = 0b1100;       // PWM mode (active = high)
    CCPTMRS0bits.C3TSEL = 0b00;       // ECCP3 PWM mode is based off of TMR2
    ECCP3DELbits.P3DC = 4;            // 2 us of deadtime (1/2/200000)
}

/**
 * Updates PWM duty-cycle
 */
void UpdatePWM1()
{
    Phase = (Phase + Delta_Phase);              // Accumulate Delta_Phase in Phase variable
    Result = 32767 + SineTable[Phase >> 10];    // Take sine info
    Result = Result >> 6;
    CCP1CONbits.DC1B = Result;            // Update Duty Cycle
    CCPR1L = Result >> 2;                       // Update Duty Cycle
    
}

/**
 * Updates Enhanced PWM 2 duty-cycle
 */
void UpdatePWM2()
{
    Phase = (Phase + Delta_Phase);  // Accumulate Delta_Phase in Phase variable
    Result = 32767 + SineTable[(Phase + _120_DEGREES) >> 10];                  // Take sine info
    Result = Result >> 6;
    CCP2CONbits.DC2B = Result;                          // Update Duty Cycle
    CCPR2L = Result >> 2;                               // Update Duty Cycle
}

/**
 * Updates Enhanced PWM 3 duty-cycle
 */
void UpdatePWM3()
{
    Phase = (Phase + Delta_Phase);  // Accumulate Delta_Phase in Phase variable
    Result = 32767 + SineTable[(Phase + _240_DEGREES) >> 10];                  // Take sine info
    Result = Result >> 6;
    CCP3CONbits.DC3B = Result;                          // Update Duty Cycle
    CCPR3L = Result >> 2;                               // Update Duty Cycle
}