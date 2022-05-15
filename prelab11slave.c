/* 
 * File:   cateto.c
 * Author: juane
 *
 * Created on 9 de mayo de 2022, 06:31 PM
 */
// Configuracion 1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// Configuracion 2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

/* 
 *Librerías
 */
#include <xc.h>
#include <stdint.h>

/* 
 *Constantes
 */
#define _XTAL_FREQ 1000000

/* 
 *Variables
 */

//Es el contador que se envia el maestro al esclavo
uint8_t contador = 0;         
uint8_t potenciometro = 0;
uint8_t flags = 0;

/* 
 *Prototipo de Funciones
 */
void setup(void);

/* 
 *Interrupciones
 */
void __interrupt() isr (void)
{
    //Bandera de Interrupcion
    if(PORTAbits.RA1)
    {
        if(PIR1bits.SSPIF)
        { 
            potenciometro = SSPBUF; 
            PORTD = potenciometro;
            PIR1bits.SSPIF = 0;         // Limpiamos bandera de interrupción
        }
        if(INTCONbits.RBIF){
            if(!PORTBbits.RB0)
            {
                //Incrementa el PORTE
                PORTE++;
                //Incrementa el c
                contador++;
            }
            if(!PORTBbits.RB1)
            {
                //Decrementa el PORTE
                PORTE--;
                //Decrementa
                contador--;
            }
            INTCONbits.RBIF = 0;
        }
    }
    return;
}

/* 
 *Ciclo Principal
 */
void main(void) 
{
    setup();
    while(1)
    {
        //Se comprueba que no hay comunicación en proceso
        if(SSPSTATbits.BF)
        {     
            flags = 1;
            //Se mueve el valor del contador para poderlo enviar
            SSPBUF = potenciometro;    
        }
    }
    return;
}

/* 
 *Configuracion
 */
void setup(void){
    ANSEL = 0;
    ANSELH = 0;                 // I/O digitales

    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno

    TRISA = 0b00000111;         // SS y RA0 como entradas
    PORTA = 0;

    TRISD = 0;
    PORTD = 0;
    
    TRISE = 0;
    PORTE = 0;


    if(PORTAbits.RA1){
        TRISC = 0b00011000; // -> SDI y SCK entradas, SD0 como salida
        PORTC = 0;

        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0101;   // -> SPI Esclavo, SS hablitado
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj

        PIR1bits.SSPIF = 0;         // Limpiamos bandera de SPI
        PIE1bits.SSPIE = 1;         // Habilitamos int. de SPI
        INTCONbits.PEIE = 1;
        INTCONbits.GIE = 1;
        
        TRISBbits.TRISB0 = 1;       // RB0 como entrada (configurada con control de bits)
        TRISBbits.TRISB1 = 1;       // RB0 como entrada (configurada con control de bits)

        OPTION_REGbits.nRBPU = 0;   // Habilitamos resistencias de pull-up del PORTB
        WPUBbits.WPUB0 = 1;         // Habilitamos resistencia de pull-up de RB0
        WPUBbits.WPUB1 = 1;         // Habilitamos resistencia de pull-up de RB1

        INTCONbits.RBIE = 1;        // Habilitamos interrupciones del PORTB
        IOCBbits.IOCB0 = 1;         // Habilitamos interrupción por cambio de estado para RB0
        IOCBbits.IOCB1 = 1;         // Habilitamos interrupción por cambio de estado para RB1
        INTCONbits.RBIF = 0;        // Limpiamos bandera de interrupción
    }
}