#include <xc.h>
#define _XTAL_FREQ 4000000     // 4 MHz clock frequency

// Configuration bits
#pragma config FOSC = XT        // External crystal or simulator clock
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

void UART_Init(void)
{
    /*
     * Baud rate: 9600 @ 4 MHz, high-speed mode
     * SPBRG = (Fosc / (16 × Baud)) ? 1 = 25
     */
    SPBRG = 0x19;   // 9600 bps exact in high-speed mode

    /*
     * TXSTA ? Transmit Status and Control (0x98)
     * 7 CSRC = 0  (don?t care, async)
     * 6 TX9  = 0  (8-bit)
     * 5 TXEN = 1  (enable TX)
     * 4 SYNC = 0  (async)
     * 3 SENDB= 0
     * 2 BRGH = 1  (high-speed)
     * 1 TRMT = 0  (read-only)
     * 0 TX9D = 0
     * 0b00100100 = 0x24
     */
    TXSTA = 0x24;   // enable TX, async, high-speed

    /*
     * RCSTA ? Receive Status and Control (0x18)
     * 7 SPEN = 1  (enable serial port)
     * 6 RX9  = 0  (8-bit)
     * 5 SREN = 0
     * 4 CREN = 1  (continuous receive)
     * 3 ADDEN= 0
     * 2 FERR = 0
     * 1 OERR = 0
     * 0 RX9D = 0
     * 0b10010000 = 0x90
     */
    RCSTA = 0x90;   // enable port and receiver

    TRISC6 = 0;     // RC6 = TX ? output
    TRISC7 = 1;     // RC7 = RX ? input
}

// UART transmit one character
void UART_TxChar(char data)
{
    while(!TXIF);       // Wait until transmit buffer is empty
    TXREG = data;       // Send data
}

// UART transmit string
void UART_TxString(const char *str)
{
    while(*str)
    {
        UART_TxChar(*str++);
    }
}

// -------- ADC Initialization --------
void ADC_Init(void)
{
    /*
     * ADCON1 (0x9F)
     * 7 ADFM = 1 ? Right justified result
     * 6 ADCS2 = 0 ? Conversion clock (Fosc/32 total with ADCS1:ADCS0)
     * 5?4 PCFG3:PCFG2 = 0 0
     * 3?0 PCFG1:PCFG0 = 0 0 ? All analog channels enabled, Vref = Vdd/Vss
     * Binary: 0b10000000 ? 0x80
     */
    ADCON1 = 0x80;   // Right justified, Vref=Vdd, 8 analog inputs

    /*
     * ADCON0 (0x1F)
     * Bit 7?6 ADCS1:ADCS0 = 10 ? Fosc/32 (recommended for 4 MHz)
     * Bit 5?3 CHS2:CHS0 = 000 ? Select channel AN0
     * Bit 2 GO/DONE = 0 ? Idle
     * Bit 1 ADON = 1 ? ADC enabled
     * Bit 0 ? don?t care
     * Binary: 0b10000001 ? 0x81
     */
    ADCON0 = 0x81;   // Select AN0, ADC ON, Fosc/32
}

// -------- ADC Read Function --------
unsigned int ADC_Read(unsigned char channel)
{
    // Select ADC channel (0?7)
    ADCON0 &= 0xC5;               // Clear existing channel bits
    ADCON0 |= (channel << 3);     // Set new channel
    __delay_ms(2);                // Acquisition time

    GO_nDONE = 1;                 // Start conversion
    while(GO_nDONE);              // Wait until done

    return ((ADRESH << 8) + ADRESL); // Return 10-bit result
}

// ---------- Integer to String ----------
void intToString(unsigned int num, char *str)
{
    char temp[6];
    int i = 0, j = 0;

    if(num == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while(num > 0)
    {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }

    // Reverse
    for(j = 0; j < i; j++)
        str[j] = temp[i - j - 1];

    str[i] = '\0';
}

// ---------- Float to String (1 decimal place) ----------
void floatToString(float num, char *str)
{
    unsigned int int_part, frac_part;
    char temp[6];
    int i = 0, j;

    int_part = (unsigned int)num;
    frac_part = (unsigned int)((num - int_part) * 10); // 1 decimal

    intToString(int_part, temp);  // integer part
    while(temp[i] != '\0') {
        str[i] = temp[i];
        i++;
    }

    str[i++] = '.';
    str[i++] = frac_part + '0';
    str[i] = '\0';
}


void main(void)
{
    UART_Init();                 // Initialize UART
    __delay_ms(100);

    UART_TxString("UART Test Start\r\n");
    
    unsigned int adc_value;
    float temperature;
     char adc_str[6];
    char temp_str[10];

    ADC_Init();   // Initialize ADC
    TRISA = 0xFF; // RA0 (AN0) input
    
    while(1)
    {
        adc_value = ADC_Read(0);  // Read LM35 sensor on AN0

        // Convert ADC to temperature (°C)
        // Step size = 4.88mV, LM35 = 10mV/°C
        temperature = (adc_value * 4.88) / 10.0;

        intToString(adc_value, adc_str);
        floatToString(temperature, temp_str);

        UART_TxString("ADC = ");
        UART_TxString(adc_str);
        UART_TxString("\r\nTemp = ");
        UART_TxString(temp_str);
        UART_TxString(" C\r\n\r\n");

        __delay_ms(1000);
        //UART_TxString("Hello World!\r\n");
       // __delay_ms(1000);        // 1 second delay
    }
}
