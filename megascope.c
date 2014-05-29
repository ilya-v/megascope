#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//#define N_UART_TICKS            103     //9600
//#define N_UART_TICKS          51      //18200
#define N_UART_TICKS          25      //38400
#define N_STOP_BITS             2


//#define SET_TX_PIN( )    ( PORTD |= ( 1 << PD1 ) )
//#define CLEAR_TX_PIN( )  ( PORTD &= ~( 1 << PD1 ) )
//#define GET_RX_PIN( )    ( PIND & ( 1 << PD2 ) )
#define SET_TX_PIN( )    ( PORTD |= ( 1 << PD1 ) )
#define CLEAR_TX_PIN( )  ( PORTD &= ~( 1 << PD1 ) )

typedef enum
{
    START_BIT, 
    TRANSMIT,
    STOP_BITS
} uart_state_t;

        char    volatile    uart_string[256]    = "";
const   char    volatile  * p_next_uart_char    = uart_string;
        uint8_t volatile    is_uart_idle        = 1;
        


ISR(TIMER0_COMPA_vect)
{
    static  uint8_t         char_to_send        = 0;
    static  uint8_t         bits_to_send        = 0;
    static  uint8_t         stop_bits_to_send   = 0;
    static  uart_state_t    uart_state          = START_BIT;
        
    switch (uart_state) {
    case START_BIT:
        char_to_send = (unsigned char)*p_next_uart_char;
        if (!char_to_send)
        {
            is_uart_idle = 1;            
            return;
        }
        is_uart_idle = 0;
        p_next_uart_char ++;
        bits_to_send = 8;
        uart_state  = TRANSMIT;
        CLEAR_TX_PIN();
    break;
    
    case TRANSMIT:
        if( char_to_send & 0x01 )
            SET_TX_PIN();
        else
            CLEAR_TX_PIN();
        char_to_send = char_to_send >> 1;
        bits_to_send --;

        if (!bits_to_send)
        {
            stop_bits_to_send = N_STOP_BITS;
            uart_state = STOP_BITS; 
        } 
    break;

    case STOP_BITS:
        SET_TX_PIN();        
        stop_bits_to_send --;
        if (!stop_bits_to_send)
            uart_state = START_BIT;        
    break; 
    } 
}

volatile char * print_byte(char volatile *s, uint8_t n)
{
    s[3] = '\t';
    s[2] = '0' + n % 10;
    n/=10;
    s[1] = '0' + n % 10;
    n/=10;
    s[0] = '0' + n;
    return s + 4;
}

volatile char * print_word(char volatile *s, uint16_t n)
{
    s[5] = '\t';
    s[4] = '0' + n % 10;
    n/=10;
    s[3] = '0' + n % 10;
    n/=10;
    s[2] = '0' + n % 10;
    n/=10;
    s[1] = '0' + n % 10;
    n/=10;
    s[0] = '0' + n;
    return s + 6;
}

unsigned char process_adc(uint8_t *adc_data)
{
    static uint8_t adc_index = 0;

    const uint8_t 
        are_data_ready  = ADCSRA & _BV(ADIF),
        is_adc_busy     = ADCSRA & _BV(ADSC);

    if (!are_data_ready)
    {
        if (!is_adc_busy)
        {
            ADMUX = (ADMUX & 0xF0) | adc_index;
            ADCSRA |= _BV(ADSC);
        }
        return  0;
    }
    
    ADCSRA |= _BV(ADIF);
    adc_data[adc_index] = ADCH;
    adc_index ++;
    if (adc_index >= 5)
        adc_index = 0;
    return adc_index == 0;  
}

int main( void )
{
    DDRD    = 0b11000010;
    PORTD   = 0;
    SET_TX_PIN();                    // Set the TX line to idle state.
    TCCR0A  = _BV(WGM01);
    TCCR0B  = _BV(CS01);           // Divide by 8 prescaler
    OCR0A   = N_UART_TICKS;
    TIMSK0 = _BV(OCIE0A);

    TCCR1A = 0;
    TCCR1B = _BV(CS12) | _BV(CS10);
    TCNT1 = 0;


    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    ADMUX = _BV(REFS0) | _BV(ADLAR);

    sei();

    uint8_t adc_data[5];
    for (;;)
    {  
        const uint8_t is_adc_ready = process_adc(adc_data);
        if ( is_uart_idle && is_adc_ready)
        {
            PIND = (unsigned char)0b10000000;
            char volatile *s = uart_string;
            s = print_word(s, (const volatile uint16_t)TCNT1);
            for (unsigned char i = 0; i < sizeof(adc_data); ++i)
                s = print_byte(s, adc_data[i]);            
            s[0] = '\r';
            s[1] = '\n';
            s[2] = 0;
            p_next_uart_char = uart_string;     
        }    
    }
}
