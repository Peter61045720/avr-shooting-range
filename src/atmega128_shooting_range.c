#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

#include "avr/io.h"
#include "avr/interrupt.h"
#include "util/delay.h"

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
#define CG_RAM_ADDR     0x00000040
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

static void lcd_send_int(long number) {
	int digits[20];
	unsigned char temp = 0, number_length = 0;

	if (number < 0) {
		lcd_send_char('-');
		number = -number;
	}

	do {
		temp++;
		digits[temp] = number % 10;
		number = number / 10;
	} while (number);

	number_length = temp;

	for (temp = number_length; temp > 0; temp--) {
        lcd_send_char(digits[temp] + 48);
    }
}

static void lcd_send_text(char* text) {
    while (*text) {
        lcd_send_char(*text++);
    }
}

static void lcd_send_char_array(char* text, int n) {
    for (int i = 0; i < n; i++) {
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

// -------------------- Button handling --------------------
#define BUTTON_NONE     0
#define BUTTON_UP       1
#define BUTTON_LEFT     2
#define BUTTON_MIDDLE   3
#define BUTTON_RIGHT    4
#define BUTTON_DOWN     5
static int button_accept = 1;

static int button_pressed() {
    if (!(PINA & 0b00000001) && button_accept) {
        button_accept = 0;
        return BUTTON_UP;
    }

    if (!(PINA & 0b00000010) && button_accept) {
        button_accept = 0;
        return BUTTON_LEFT;
    }

    if (!(PINA & 0b00000100) && button_accept) {
        button_accept = 0;
        return BUTTON_MIDDLE;
    }

    if (!(PINA & 0b00001000) && button_accept) {
        button_accept = 0;
        return BUTTON_RIGHT;
    }

    if (!(PINA & 0b00010000) && button_accept) {
        button_accept = 0;
        return BUTTON_DOWN;
    }

    return BUTTON_NONE;
}

static void button_unlock() {
    if (
        (
            (PINA & 0b00000001) | 
            (PINA & 0b00000010) | 
            (PINA & 0b00000100) | 
            (PINA & 0b00001000) | 
            (PINA & 0b00010000)
        ) == 31
    ) {
        button_accept = 1;
    }
}

static void wait_until_button_pressed(int button_code) {
    while (button_pressed() != button_code) {
        button_unlock();
    }
}

// -------------------- Custom characters --------------------
#define CHAR_CROSSHAIR      0
#define CHAR_HIT_MARKER     1
#define CHAR_HEART          2
#define CHAR_TIMER          3
#define CHAR_HOURGLASS_UP   4
#define CHAR_HOURGLASS_DOWN 5
#define CHAR_GUN_PART_1     6
#define CHAR_GUN_PART_2     7

static unsigned char cg_ram[64] = {
    0b00000, 0b01010, 0b10001, 0b10101, 0b10001, 0b01010, 0b00000, 0b00000, // CROSSHAIR
    0b00000, 0b10001, 0b01010, 0b00000, 0b01010, 0b10001, 0b00000, 0b00000, // HIT MARKER
    0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000, // HEART
    0b01110, 0b00100, 0b01110, 0b10101, 0b10111, 0b10001, 0b01110, 0b00000, // TIMER
    0b11111, 0b11111, 0b01110, 0b00100, 0b01010, 0b10001, 0b11111, 0b00000, // HOURGLASS UP
    0b11111, 0b10001, 0b01010, 0b00100, 0b01110, 0b11111, 0b11111, 0b00000, // HOURGLASS DOWN
    0b00000, 0b00000, 0b00111, 0b11111, 0b11101, 0b10110, 0b00000, 0b00000, // GUN PART 1
    0b00000, 0b00001, 0b11111, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000, // GUN PART 2
};

static void cg_ram_init() {
    lcd_send_command(CG_RAM_ADDR);
    for (unsigned int i = 0; i < 64; ++i) {
        lcd_send_char(cg_ram[i]);
    }
}

// -------------------- Game functions & helpers --------------------

// ----- Game logic -----

// Movement directions
#define DIRECTION_UP    1
#define DIRECTION_LEFT  2
#define DIRECTION_RIGHT 3
#define DIRECTION_DOWN  4

// Movement types for targets
#define MOVE_NONE       0
#define MOVE_HORIZONTAL 1
#define MOVE_VERTICAL   2
#define MOVE_CIRCULAR   3

// Difficulties
#define EASY    0
#define MEDIUM  1
#define HARD    2

// Play area size
#define ROWS 2
#define COLS 10

// Play area boundaries
#define MIN_X 0
#define MIN_Y 0
#define MAX_X 9
#define MAX_Y 1

// Play area offset on the LCD
#define OFFSET 3

// ASCII codes
#define SPACE       32
#define NUMBER_0    48
#define NUMBER_1    49
#define NUMBER_2    50
#define NUMBER_3    51
#define NUMBER_4    52
#define NUMBER_5    53
#define NUMBER_6    54
#define NUMBER_7    55
#define NUMBER_8    56
#define NUMBER_9    57

typedef struct {
    int x, y;
    int prev_x, prev_y;
} crosshair_t;

typedef struct {
    int x, y;
    char value;
    int movement_type;
} target_t;

typedef struct {
    target_t targets[7];
    int target_count;
    char max_target_value;
} level_t;

static crosshair_t crosshair = { 5, 1, 5, 1 };
static level_t levels[9];
static int current_level = 0;
static int current_difficulty = EASY;
static char current_number = NUMBER_0;
static int health_points = 3;
static long score = 0;
static volatile int remaining_time = 99;
static volatile int tick = 0;

static char play_area[ROWS][COLS] = {
    { SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE },
    { SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE }
};

static void levels_init() {
    // Level 1 (easy,   4 targets, no target movement)
    levels[0].targets[0] = (target_t){ 2, 0, NUMBER_1, MOVE_NONE };
    levels[0].targets[1] = (target_t){ 3, 0, NUMBER_2, MOVE_NONE };
    levels[0].targets[2] = (target_t){ 6, 0, NUMBER_3, MOVE_NONE };
    levels[0].targets[3] = (target_t){ 7, 0, NUMBER_4, MOVE_NONE };
    levels[0].target_count = 4;
    levels[0].max_target_value = NUMBER_4;

    // Level 2 (easy,   4 targets, no target movement)
    levels[1].targets[0] = (target_t){ 0, 1, NUMBER_1, MOVE_NONE };
    levels[1].targets[1] = (target_t){ 1, 0, NUMBER_2, MOVE_NONE };
    levels[1].targets[2] = (target_t){ 8, 0, NUMBER_3, MOVE_NONE };
    levels[1].targets[3] = (target_t){ 9, 1, NUMBER_4, MOVE_NONE };
    levels[1].target_count = 4;
    levels[1].max_target_value = NUMBER_4;

    // Level 3 (easy,   4 targets, no target movement)
    levels[2].targets[0] = (target_t){ 0, 0, NUMBER_1, MOVE_NONE };
    levels[2].targets[1] = (target_t){ 9, 0, NUMBER_2, MOVE_NONE };
    levels[2].targets[2] = (target_t){ 5, 0, NUMBER_3, MOVE_NONE };
    levels[2].targets[3] = (target_t){ 2, 1, NUMBER_4, MOVE_NONE };
    levels[2].target_count = 4;
    levels[2].max_target_value = NUMBER_4;

    // TODO: implement rest of the levels
    // Level 4 (medium, 5 targets, horizontal target movement)

    // Level 5 (medium, 5 targets, vertical target movement)

    // Level 6 (medium, 5 targets, horizontal and vertical target movement)

    // Level 7 (hard,   7 targets, horizontal and circular target movement)

    // Level 8 (hard,   7 targets, vertical and circular target movement)

    // Level 9 (hard,   7 targets, horizontal, vertical and circular target movement)
}

static void place_targets() {
    for (int i = 0; i < levels[current_level].target_count; i++) {
        target_t* target = &levels[current_level].targets[i];
        play_area[target->y][target->x] = target->value;
    }
}

static void clear_play_area() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            play_area[i][j] = SPACE;
        }
    }
}

static int level_completed() {
    if (current_number == levels[current_level].max_target_value) {
        current_level++;
        clear_play_area();
        place_targets();
        crosshair.x = 5;
        crosshair.y = 1;
        current_number = NUMBER_0;
        return 1;
    }

    return 0;
}

static void move_crosshair(int direction) {
    crosshair.prev_x = crosshair.x;
    crosshair.prev_y = crosshair.y;

    if (direction == DIRECTION_UP) {
        crosshair.y = MIN_Y;
    }

    if (direction == DIRECTION_LEFT && crosshair.x > MIN_X) {
        crosshair.x--;
    }

    if (direction == DIRECTION_RIGHT && crosshair.x < MAX_X) {
        crosshair.x++;
    }

    if (direction == DIRECTION_DOWN) {
        crosshair.y = MAX_Y;
    }
}

static int crosshair_on_target() {
    return play_area[crosshair.y][crosshair.x] != SPACE;
}

static int crosshair_on_correct_target() {
    return play_area[crosshair.y][crosshair.x] == current_number + 1;
}

static void shoot() {
    if (crosshair_on_target() && crosshair_on_correct_target()) {
        play_area[crosshair.y][crosshair.x] = SPACE;
        current_number++;
        score += 50 * (current_difficulty * 2 + 1);
    } else {
        health_points--;
    }
}

static void reset_game_state() {
    current_level = 0;
    current_difficulty = EASY;
    current_number = NUMBER_0;
    crosshair.x = 5;
    crosshair.y = 1;
    crosshair.prev_x = 5;
    crosshair.prev_y = 1;
    health_points = 3;
    score = 0;
    remaining_time = 99;
    tick = 0;
    clear_play_area();
}

// ----- Display -----
static void display_title_screen() {
    lcd_send_command(CLR_DISP);
    lcd_send_line1(" Number Hunt");
    lcd_send_command(DD_RAM_ADDR + 13);
    lcd_send_char(CHAR_GUN_PART_1);
    lcd_send_char(CHAR_GUN_PART_2);
    lcd_send_line2("  Press START! ");
}

static void display_loading_screen() {
    lcd_send_command(CLR_DISP);
    lcd_send_command(DD_RAM_ADDR + 1);
    lcd_send_char(CHAR_HOURGLASS_UP);
    lcd_send_text(" Loading... ");
    lcd_send_char(CHAR_HOURGLASS_DOWN);
    _delay_ms(5000);
}

static void display_current_level() {
    lcd_send_command(CLR_DISP);
    lcd_send_line1("    Level ");
    lcd_send_int(current_level + 1);
    _delay_ms(4000);
}

static void display_game_over_screen() {
    lcd_send_command(CLR_DISP);
    lcd_send_line1("   Game over");
    lcd_send_line2("  Press START! ");
}

static void display_score_screen() {
    lcd_send_command(CLR_DISP);
    lcd_send_line1(" Score: ");
    lcd_send_int(score);
    lcd_send_line2("  Press START! ");
}

// HUD layer (HP, Time)
static void hud_init() {
    lcd_send_command(CLR_DISP);

    // Line 1
    lcd_send_command(DD_RAM_ADDR);
    lcd_send_text("HP|");

    lcd_send_command(DD_RAM_ADDR + 13);
    lcd_send_char('|');
    lcd_send_char(CHAR_TIMER);
    lcd_send_char('T');

    // Line 2
    lcd_send_command(DD_RAM_ADDR2);
    lcd_send_int(health_points);
    lcd_send_char(CHAR_HEART);
    lcd_send_char('|');

    lcd_send_command(DD_RAM_ADDR2 + 13);
    lcd_send_char('|');
    lcd_send_int(remaining_time);
}

static void update_hud() {
    lcd_send_command(DD_RAM_ADDR2);
    lcd_send_int(health_points);

    lcd_send_command(DD_RAM_ADDR2 + 14);

    if (remaining_time < 10) {
        lcd_send_int(0);
    }

    lcd_send_int(remaining_time);
}

// Play area layer (Targets)
static void play_area_init() {
    lcd_send_command(DD_RAM_ADDR + OFFSET);
    lcd_send_char_array(play_area[0], COLS);
    lcd_send_command(DD_RAM_ADDR2 + OFFSET);
    lcd_send_char_array(play_area[1], COLS);
}

// Overlay layer (Crosshair)
static int get_dd_ram_address(int x, int y) {
    int base = y == MIN_Y ? DD_RAM_ADDR : DD_RAM_ADDR2;
    return base + OFFSET + x;
}

static void overlay_init() {
    lcd_send_command(get_dd_ram_address(crosshair.x, crosshair.y));
    lcd_send_char(CHAR_CROSSHAIR);
}

static void update_overlay() {
    lcd_send_command(get_dd_ram_address(crosshair.prev_x, crosshair.prev_y));
    lcd_send_char(play_area[crosshair.prev_y][crosshair.prev_x]);

    lcd_send_command(get_dd_ram_address(crosshair.x, crosshair.y));
    lcd_send_char(crosshair_on_target() ? CHAR_HIT_MARKER : CHAR_CROSSHAIR);
}

// -------------------- Timer --------------------
static void timer_init() {
    // Timer1 (16 bit)
    TCNT1 = 0;
    TCCR1B |= (1 << CS12) | (1 << CS10);    // Timer1 clock rate: 16 000 000 Hz / 1 024 = 15 625 Hz
    TIMSK = (1 << TOIE1);                   // Overflow period: 65 536 / 15 625 Hz = 4,194304 sec
    sei();
}

static void reset_timer(int t) {
    TCNT1 = 0;
    tick = 0;
    remaining_time = t;
}

ISR(TIMER1_OVF_vect) {
    tick++;
    
    if (tick >= 4) {
        tick = 0;
        remaining_time--;
    }
}

// -------------------- Main function --------------------
int main() {
    port_init();
    lcd_init();
    cg_ram_init();
    timer_init();
    levels_init();

    // ----- Program loop -----
    while (1) {
        reset_game_state();
        display_title_screen();
        wait_until_button_pressed(BUTTON_MIDDLE);
        display_loading_screen();
        place_targets();
        display_current_level();
        reset_timer(99);
        hud_init();
        play_area_init();
        overlay_init();

        // ----- Game loop -----
        while (1) {
            if (health_points <= 0 || remaining_time <= 0) {
                break;
            }

            int button = button_pressed();

            if (button == BUTTON_UP) {
                move_crosshair(DIRECTION_UP);
                update_overlay();
            }

            if (button == BUTTON_LEFT) {
                move_crosshair(DIRECTION_LEFT);
                update_overlay();
            }

            if (button == BUTTON_RIGHT) {
                move_crosshair(DIRECTION_RIGHT);
                update_overlay();
            }

            if (button == BUTTON_DOWN) {
                move_crosshair(DIRECTION_DOWN);
                update_overlay();
            }

            if (button == BUTTON_MIDDLE) {
                shoot();
                update_overlay();

                if (level_completed()) {
                    display_current_level();
                    reset_timer(99);
                    hud_init();
                    play_area_init();
                    overlay_init();
                }
            }

            update_hud();
            button_unlock();
        }

        display_game_over_screen();
        wait_until_button_pressed(BUTTON_MIDDLE);
        display_score_screen();
        wait_until_button_pressed(BUTTON_MIDDLE);
    }

    return 0;
}