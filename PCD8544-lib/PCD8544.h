							/* Libreria SPI para LCD Nokia 5110 */
#include <avr/io.h>

		/* Pin definitions */ //(toda esta parte deberia luego ponerse en un header de pindefines aparte)
// LCD slave select
#define LCD_SS		PB2
#define LCD_SS_PORT	PORTB
#define LCD_SS_PIN	PINB	// Not used
#define LCD_SS_DDR	DDRB

// LCD DC pin
#define LCD_DC		PB1
#define LCD_DC_PORT	PORTB
#define LCD_DC_PIN	PINB	// Not used
#define LCD_DC_DDR	DDRB

// LCD reset pin
#define LCD_RESET		PB0
#define LCD_RESET_PORT	PORTB
#define LCD_RESET_PIN	PINB	// Not used
#define LCD_RESET_DDR	DDRB

// SPI pins (Set for ATmega328)
#define SPI_MOSI                     PB3
#define SPI_MOSI_PORT                PORTB
#define SPI_MOSI_PIN                 PINB
#define SPI_MOSI_DDR                 DDRB

#define SPI_MISO                     PB4
#define SPI_MISO_PORT                PORTB
#define SPI_MISO_PIN                 PINB
#define SPI_MISO_DDR                 DDRB

#define SPI_SCK                     PB5
#define SPI_SCK_PORT                PORTB
#define SPI_SCK_PIN                 PINB
#define SPI_SCK_DDR                 DDRB

		/* Handy macros */
// LCD slave select and deselect
#define LCD_SLAVE_SELECT	LCD_SS_PORT &= ~(1 << LCD_SS) //Recordar que select slave es con LOW
#define LCD_SLAVE_DESELECT	LCD_SS_PORT |= (1 << LCD_SS) //Recordar que slave deselect es con HIGH

// LCD reset
#define LCD_RESET_LOW	LCD_RESET_PORT &= ~(1 << LCD_RESET)
#define LCD_RESET_HIGH	LCD_RESET_PORT |= (1 << LCD_RESET)

// LCD D/C toggle
#define LCD_DC_LOW	LCD_DC_PORT &= ~(1 << LCD_DC)
#define LCD_DC_HIGH	LCD_DC_PORT |= (1 << LCD_DC)

// Array length for passing bitmap length as argument
#define arrayLength(array) (sizeof((array))/sizeof((array)[0]))

		/* Instruction set -- from datasheet */
// Bytes to be sent through MOSI that represent LCD commands

// Power up and down
#define POWERDOWN_MODE		0b00100100	//For powering down, doesn´t matter but horizontal addressing and basic instruction set
#define POWERUP_EIS			0b00100001	//For powering up with extended instruction set (ESI), doesn´t matter but horizontal addressing mode
#define BIS_H				0b00100000	//Set for basic instruction set (BIS) and horizontal addressing
#define BIS_V				0b00100010	//Set for basic instruction set (BIS) and vertical addressing

// LCD settings
#define BIAS_SYSTEM_SET					0b00010011	//Set MUX 1:48 (Little effect?)
#define VOP_SET							0b10111111	//Set Vop to 5V (Little effect?)
#define TEMPERATURE_CONTROL_COEF_SET	0b00000100	//Set temperature coefficient (Little effect?)

// Display configuration
#define DISPLAY_BLANK	0b00001000	//Blank display
#define DISPLAY_NORMAL	0b00001100	//Normal mode
#define DISPLAY_FULL	0b00001001	//All segments on
#define DISPLAY_INVERSE	0b00001101	//Inverse video mode


// Struct
struct Text {
	char *string;
	uint8_t px_width;
	uint8_t char_length;
	uint8_t pos[2];
	char justification;
};

struct Font {
	char *pgm_start_location;
	uint8_t px_width;
	uint8_t px_height;
} LCD_font;

void LCD_setFont(const char *pgm_start_location, uint8_t px_width, uint8_t px_height);

// Functions

/* SPI */

		/* Init SPI to run on port and pin set above */
void initSPI(void);

		/* Generic, just loads up HW SPI register and waits */
void SPI_tradeByte(uint8_t byte);

/* Communication with LCD directly */

		/* Send command to LCD as defined above*/
void LCD_sendCommand(uint8_t command);

		/* Write data byte to DDRAM */
void LCD_writeData(uint8_t data);

		/* Activate LCD, set bias system, temperature coefficient, Vop and set to normal display mode */
void LCD_powerOn(void);

		/* Put LCD in power down mode */
void LCD_powerDown(void);

		/* Select X address start */
void LCD_setXadd(uint8_t address);

		/* Select Y address start */
void LCD_setYadd(uint8_t address);

/* Writing to LCD through screen buffer frame */

		/* Draw Screen Buffer on LCD */
void LCD_updateScreen(void);

		/* Print same byte on all screen bytes */
void LCD_allScreen(uint8_t byte);

		/* Print image from program memory onto whole screen, array has to 
		be organize in scanning left to right hand vertical orientation of bits */
void LCD_drawFromPGM(unsigned char *image_pointer, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row);

		/* Same as drawFromPGM but ORs into the screen buffer */
void LCD_drawOverFromPGM(unsigned char *image_pointer, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row);

		/* Same as other but bitmap isn't in program memory */
void LCD_draw(unsigned char bitmap[], uint16_t bitmap_length, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row);

		/* Same as other but bitmap isn't in program memory */
void LCD_drawOver(unsigned char bitmap[], uint16_t bitmap_length, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row);

/* Printing text boxes */

void LCD_printInt(uint16_t n, uint8_t col, uint8_t row, char justification);

void LCD_printFP(int16_t n, uint16_t SF_exp, uint8_t d_places, uint8_t col, uint8_t row, char justification);

		/* 6x8 font */
void LCD_printSmallFont(char string[], uint8_t width, uint8_t length, uint8_t col, uint8_t row, char justification);

/* Drawing functions */
void LCD_drawPixel(uint8_t x, uint8_t y);

void LCD_drawLine (int8_t x0, int8_t y0, int8_t x1, int8_t y1);

void LCD_drawRectangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, int8_t r);

void LCD_drawBanner(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, int8_t r);

void LCD_drawCircle(uint8_t xm, uint8_t ym, int8_t r);
