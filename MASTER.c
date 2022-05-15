/* 
 * File:   LAB Y POSTLAB.c
 * Author: juane
 *
 * Created on 13 de mayo de 2022, 04:16 PM
 */
// CONFIG1
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

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000           // Frecuencia del oscilador
#define FLAG_SPI   0x52              // Variable para PMW

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
char temp = 0;     //Variable temporal que recibe datos y sirve para la comparacion
char valADC = 0;   //Valor del ADC
int  cont;         //Variable para el contador
/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if (PIR1bits.SSPIF){ //Se habilita la bandera de interrupcion SPI, para el esclavo   
        
        temp = SSPBUF;
        if (temp != FLAG_SPI){ // PMW
            
            valADC = temp;  //El valor del ADC se iguala a la varible temporal
            PORTD = temp;    // En el PORTD se muestra el valor de la variable temporal
            CCPR1L = (valADC>>3) + 7;  // Que tenga el mismo valor del registro
            
            SSPBUF = cont;   // Se guarda el valor en el buffer
        }
        else {
            SSPBUF = cont;
        }
                
        PIR1bits.SSPIF = 0;             // Clear a la interrupcion
    }
    
    if(INTCONbits.RBIF){  //Interrupt On Change (IOC)
        if(!PORTBbits.RB0){  
            cont++;}                // Incrementa
        
        if(!PORTBbits.RB1){
            cont--;}                //Decrementa
        
        INTCONbits.RBIF = 0;    // Clear a la interrupcion
    }
    
     if(PIR1bits.ADIF){ //ADC interrupcion
        if(ADCON0bits.CHS == 1){   
            valADC = ADRESH;     //El valor de ADRESH se guarda en la variable valADC
        }
        PIR1bits.ADIF = 0;          // Clear a la interrupcion
    }
    
    
    
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        // MASTER envío y recepcion  
        if (PORTAbits.RA0){                         // A0 = 1, maestro 
            if(ADCON0bits.GO == 0){ADCON0bits.GO = 1;}  
            
            //Contador master-slave
            PORTAbits.RA6 = 1;     
            __delay_ms(10);      
            PORTAbits.RA6 = 0;    
            __delay_ms(10);
            
            //ADC
            SSPBUF = valADC;                     // Se guarda el valor del ADC en el buffer
            while(!SSPSTATbits.BF){}    // Esperamos a que termine el envio
            PORTAbits.RA6 = 1;        // Deshabilitar el ss del PIC esclavo 2
            __delay_ms(10);
            
            
            //Contador
            PORTAbits.RA7 = 1;   
            __delay_ms(10);    
            PORTAbits.RA7 = 0;   
            __delay_ms(10);
            
            SSPBUF = FLAG_SPI;   //Pulsos de reloj para el esclavo y enviar datos

            while(!SSPSTATbits.BF){}    //Se recibe el dato
            PORTD = SSPBUF;             //Se muestra el dato en el PORTD
            PORTAbits.RA7 = 1;          
           
            __delay_ms(500);     // Enviamos y pedimos datos cada 1 segundo
                   
        }
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0b00000010;
    ANSELH = 0;
    
    TRISB = 0xFF;                // RB0 como entrada (configurada en decimal)
    
    TRISD = 0;
    PORTD = 0;
    
    TRISA = 0b00100011;
    PORTA = 0;

    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
   
    //Habilitar el oscilador
    ADCON1bits.ADFM  = 0;       // Justificado a la izquierda
    ADCON1bits.VCFG0 = 0;       // VDD tension de alimentacion
    ADCON1bits.VCFG1 = 0;       // VSS tierra

    ADCON0bits.ADCS  = 0b01;    // FOSC/8
    ADCON0bits.CHS   = 1;       // Puertos que se utilizaran
    __delay_us(50);
    ADCON0bits.ADON  = 1;       // Habilitar el modulo ADC
    __delay_us(50);

    PIR1bits.ADIF    = 0;       // Limpiar bandera de ADC
    PIE1bits.ADIE    = 1;       // Habilitar interrupcion
    INTCONbits.PEIE  = 1;        // Interrupciones perifericas
    INTCONbits.GIE   = 1;         // Interrupciones globales
    
    // Configuración de SPI
    // Configs de Maestro
    if(PORTAbits.RA0){
        TRISC = 0b00010000;         // -> SDI entrada, SCK y SD0 como salida
        PORTC = 0;
    
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0000;   // -> SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 1;        // -> Dato al final del pulso de reloj
        //SSPBUF = cont_master;       // Enviamos un dato inicial
    }
    // Configs del esclavo
    else{
        TRISC = 0b00011000; // -> SDI y SCK entradas, SD0 como salida
        PORTC = 0;
        
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0100;   // -> SPI Esclavo, SS hablitado
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj
        
        PIR1bits.SSPIF = 0;         // Limpiamos bandera de SPI
        PIE1bits.SSPIE = 1;         // Habilitamos int. de SPI
    
        //Interrupcion del puerto B
        OPTION_REGbits.nRBPU = 0;   // Habilitamos resistencias de pull-up del PORTB
    
        WPUBbits.WPUB0  = 1;         // Habilitamos resistencia de pull-up de RB0
        WPUBbits.WPUB1  = 1;         // Resistencia en el RB1

        //INTCONbits.GIE  = 1;         // Habilitamos interrupciones globales
        INTCONbits.RBIE = 1;        // Habilitamos interrupciones del PORTB
        IOCBbits.IOCB0  = 1;         // Habilitamos interrupción por cambio de estado para RB0
        IOCBbits.IOCB1  = 1; 
        INTCONbits.RBIF = 0;        // Limpiamos bandera de interrupción
    
        //Configuracion del PWM
        
        TRISCbits.TRISC1   = 1;         // RC1 como entrada

        PR2 = 62;                      // Configurar el periodo 
        CCP1CONbits.P1M    = 0;
        CCP1CONbits.CCP1M  = 0b1100;
    
        CCPR1L = 0x0f;                  // ciclo de trabajo
        CCP1CONbits.DC1B   = 0;
        PIR1bits.TMR2IF    = 0;         // apagar la bandera de interrupcion
        T2CONbits.T2CKPS   = 0b11;      // prescaler de 1:16
        T2CONbits.TMR2ON   = 1;         // prescaler de 1:16

        while(PIR1bits.TMR2IF == 0);    // esperar un ciclo del TMR2
        PIR1bits.TMR2IF    = 0;

        TRISCbits.TRISC2   = 0;     // Salida del PWM
        TRISCbits.TRISC1   = 0;
    }
}