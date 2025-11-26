#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

#include "avr/io.h"

#define	__AVR_ATMEGA128__	1

// -------------------- copied from: avr_lcd.h --------------------
#define CLR_DISP        0x00000001
#define DISP_ON         0x0000000C
#define DISP_OFF        0x00000008
#define CUR_HOME        0x00000002
#define CUR_OFF         0x0000000C
#define CUR_ON_UNDER    0x0000000E
#define CUR_ON_BLINK    0x0000000F
#define CUR_LEFT        0x00000010
#define CUR_RIGHT       0x00000014
#define CUR_UP          0x00000080
#define CUR_DOWN        0x000000C0
#define ENTER           0x000000C0
#define DD_RAM_ADDR     0x00000080
#define DD_RAM_ADDR2    0x000000C0

// -------------------- General Init --------------------
static void port_init() {
    PORTA = 0b00011111;     DDRA = 0b01000000;
    PORTB = 0b00000000;     DDRB = 0b00000000;
    PORTC = 0b00000000;     DDRC = 0b11110111;
    PORTD = 0b11000000;     DDRD = 0b00001000;
    PORTE = 0b00000000;     DDRE = 0b00110000;
    PORTF = 0b00000000;     DDRF = 0b00000000;
    PORTG = 0b00000000;     DDRG = 0b00000000;
}

// -------------------- LCD Init & Helpers --------------------
static void delay(unsigned int b) {
    volatile unsigned int a = b;  // NOTE: volatile added to prevent the compiler to optimization the loop away

    while (a) {
        a--;
    }
}

static void e_pulse() {
	PORTC = PORTC | 0b00000100; // set E to high
	delay(1400);                // delay ~110ms
	PORTC = PORTC & 0b11111011; // set E to low
}

static void lcd_send_command(unsigned char a) {
    unsigned char data = 0b00001111 | a;    // get high 4 bits
    PORTC = (PORTC | 0b11110000) & data;    // set D4-D7
    PORTC = PORTC & 0b11111110;             // set RS port to 0
    e_pulse();                               // pulse to set D4-D7 bits

    data = a<<4;                           // get low 4 bits
    PORTC = (PORTC & 0b00001111) | data;   // set D4-D7
    PORTC = PORTC & 0b11111110;            // set RS port to 0 -> display set to command mode
    e_pulse();                              // pulse to set d4-d7 bits
}

static void lcd_send_char(unsigned char a) {
	unsigned char data = 0b00001111 | a;   // get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;   // set D4-D7
	PORTC = PORTC | 0b00000001;            // set RS port to 1
	e_pulse();                              // pulse to set D4-D7 bits

	data = a<<4;                           // get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;   // clear D4-D7
	PORTC = PORTC | 0b00000001;            // set RS port to 1 -> display set to command mode
	e_pulse();                              // pulse to set d4-d7 bits
}

static void lcd_send_text(char* text) {
    while (*text) {
        lcd_send_char(*text++);
    }
}

static void lcd_send_line1(char* text) {
    lcd_send_command(DD_RAM_ADDR);
    lcd_send_text(text);
}

static void lcd_send_line2(char* text) {
    lcd_send_command(DD_RAM_ADDR2);
    lcd_send_text(text);
}

static void lcd_init() {
    // LCD initialization
    // step by step (from Gosho) - from DATASHEET

    PORTC = PORTC & 0b11111110;
    delay(10000);

    PORTC = 0b00110000;     // set D4, D5 port to 1
    e_pulse();               // high->low to E port (pulse)
    delay(1000);

    PORTC = 0b00110000;     // set D4, D5 port to 1
    e_pulse();               // high->low to E port (pulse)
    delay(1000);

    PORTC = 0b00110000;     // set D4, D5 port to 1
    e_pulse();               // high->low to E port (pulse)
    delay(1000);

    PORTC = 0b00100000;     // set D4 to 0, D5 port to 1
    e_pulse();               // high->low to E port (pulse)

    // NOTE: added missing initialization steps
    lcd_send_command(0x28);      // function set: 4 bits interface, 2 display lines, 5x8 font
    lcd_send_command(DISP_OFF);  // display off, cursor off, blinking off
    lcd_send_command(CLR_DISP);  // clear display
    lcd_send_command(0x06);      // entry mode set: cursor increments, display does not shift

    lcd_send_command(DISP_ON);
    lcd_send_command(CLR_DISP);
}

int main() {
    port_init();
    lcd_init();

    lcd_send_line1(" Shooting Range");
    lcd_send_line2("  Press  start");

    unsigned int b = 1; // button lock (0: pressed, 1: released)

    while (1) {
        // Middle Button (Button 3)
        if (!(PINA & 0b00000100) && b) {
            lcd_send_command(CLR_DISP);
            lcd_send_line1("Loading...");
            b = 0;  // button is pressed
        }

        //check state of all buttons
        if (
            (
                  (PINA & 0b00000001)
                | (PINA & 0b00000010)
                | (PINA & 0b00000100)
                | (PINA & 0b00001000)
                | (PINA & 0b00010000)
            ) == 31
        ) {
            b = 1;  // if all buttons are released b gets value 1
        }
    }

    return 0;
}