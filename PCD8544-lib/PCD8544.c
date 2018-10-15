#include "nokia5110.h"
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

// Fonts to use, uncomment selected ones
#define TinyFont_FLAG 1
#define SmallFont_FLAG 1
//#define AdaFont_FLAG 1
//#define SinclairFont_FLAG 1

// Screen buffer is only way to send data to LCD
unsigned char LCD_Screen_Buffer[504] = {0};

		//-----//
		/* SPI */
		//-----//
		
// Initializing SPI hardware
void initSPI(void) {
	LCD_SS_DDR |= (1 << LCD_SS);			// SS pin as output
	LCD_SS_PORT |= (1 << LCD_SS);			// Start up unselected (HIGH)
	
	LCD_RESET_DDR |= (1 << LCD_RESET);		// Reset pin as output
	LCD_DC_DDR |= (1 << LCD_DC);			// DC pin as output
	
	SPI_MOSI_DDR |= (1 << SPI_MOSI);		// output on MOSI
	SPI_MISO_PORT |= (1 << SPI_MISO);		// pullup on MISO
	SPI_SCK_DDR |= (1 << SPI_SCK);			// output on SCK

	//SPCR |= (1 << SPR0);// | (1 << SPR0);	// Clock scaling if needed 
	SPSR |= (1 << SPI2X);					// 2X SPI for 4MHz
	SPCR &= ~(1 << DORD);					// MSB first
	SPCR |= (1 << CPOL) | (1 << CPHA) | (1 << MSTR) | (1 << SPE);	/* SPI mode 3, master and enable SPI*/ 
}

// Talking through SPI shift register
__attribute__((always_inline)) void SPI_tradeByte(uint8_t byte) {
	SPDR = byte;						// SPI starts sending immediately
	loop_until_bit_is_set(SPSR, SPIF);	// wait until done (function from sfr_defs.h called from avr/io.h)
}

		//-------------------------------------//
		/* Functions that talk to LCD for real */
		//-------------------------------------//

// Sending commands and data
void LCD_sendByte(uint8_t byte) {
	LCD_SLAVE_SELECT;
	SPI_tradeByte(byte);
	LCD_SLAVE_DESELECT;
}

void LCD_sendCommand(uint8_t command) {
	LCD_SLAVE_SELECT;
	SPI_tradeByte(POWERUP_EIS);
	SPI_tradeByte(command);
	SPI_tradeByte(BIS_H);
	LCD_SLAVE_DESELECT;
}

void LCD_writeData(uint8_t data) {
	LCD_DC_HIGH;
	LCD_sendByte(data);
	LCD_DC_LOW;
}

// Setting up LCD and powering off
void LCD_powerOn(void) {	
	LCD_RESET_LOW; //Toggle reset
	LCD_RESET_HIGH;
	
	LCD_DC_LOW;
	LCD_sendByte(POWERUP_EIS);
	LCD_sendByte(VOP_SET);
	LCD_sendByte(TEMPERATURE_CONTROL_COEF_SET);
	LCD_sendByte(BIAS_SYSTEM_SET);
	LCD_sendByte(BIS_H);
	LCD_sendByte(DISPLAY_NORMAL);
	LCD_allScreen(0);
	LCD_updateScreen();
}

void LCD_powerDown(void) {
	LCD_SLAVE_SELECT;
	SPI_tradeByte(POWERDOWN_MODE);
	LCD_SLAVE_DESELECT;
}

// Updating screen by sending screen buffer data
void LCD_setXadd(uint8_t address) {
	if ((address >= 0) & (address <= 83)) {
		LCD_SLAVE_SELECT;
		SPI_tradeByte(address |= (1 << 7));
		LCD_SLAVE_DESELECT;
	} else {
		return;
	}
}

void LCD_setYadd(uint8_t address) {
	if ((address >= 0) & (address <= 5)) {
		LCD_SLAVE_SELECT;
		SPI_tradeByte(address |= (1 << 6));
		LCD_SLAVE_DESELECT;
	} else {
		return;
	}
}

void LCD_updateScreen(void) {
	LCD_setXadd(0);
	LCD_setYadd(0);
	LCD_DC_HIGH;
	LCD_SLAVE_SELECT;
	for (uint16_t i = 0; i < 504; i++) { // 16 bit unsigned int allows up to 8kB bitmaps
		SPI_tradeByte(LCD_Screen_Buffer[i]);
	}
	LCD_SLAVE_DESELECT;
	LCD_DC_LOW;
}

		//----------------------------//
		/* Drawing into screen buffer */
		//----------------------------//
		
// Handy function
void LCD_allScreen(uint8_t byte) {
	for (uint16_t i = 0; i < 504; i++) {
		LCD_Screen_Buffer[i] = byte;
	}
}
		
// Only for bitmaps in program memory
void LCD_drawFromPGM(unsigned char *image_pointer, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row){
	uint8_t bitmap_cols = x_b;
	uint8_t bitmap_rows = (y_b + 7)/8;
	
	for (uint8_t drawn_rows = 0; drawn_rows < bitmap_rows; drawn_rows++) {
		for (uint8_t drawn_cols = 0; drawn_cols < bitmap_cols; drawn_cols++) {
			LCD_Screen_Buffer[col + drawn_cols + (row + drawn_rows) * 84] = pgm_read_byte(image_pointer);
			image_pointer++;
		}
	}
}

void LCD_drawOverFromPGM(unsigned char *image_pointer, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row){
	uint8_t bitmap_cols = x_b;
	uint8_t bitmap_rows = (y_b + 7)/8;
	
	for (uint8_t drawn_rows = 0; drawn_rows < bitmap_rows; drawn_rows++) {
		for (uint8_t drawn_cols = 0; drawn_cols < bitmap_cols; drawn_cols++) {
			LCD_Screen_Buffer[col + drawn_cols + (row + drawn_rows) * 84] |= pgm_read_byte(image_pointer);
			image_pointer++;
		}
	}
}

// Standard drawing
void LCD_draw(unsigned char bitmap[], uint16_t bitmap_length, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row){
	uint8_t bitmap_cols = x_b;
	uint8_t bitmap_rows = (y_b + 7)/8;
	uint16_t i = 0;
	
	for (uint8_t drawn_rows = 0; drawn_rows < bitmap_rows; drawn_rows++) {
		for (uint8_t drawn_cols = 0; drawn_cols < bitmap_cols; drawn_cols++) {
			if (i < bitmap_length) {
				LCD_Screen_Buffer[(col + drawn_cols + (row + drawn_rows) * 84)%504] = bitmap[i];
				i++;
			}
		}
	}
}

void LCD_drawOver(unsigned char bitmap[], uint16_t bitmap_length, uint8_t x_b, uint8_t y_b, uint8_t col, uint8_t row){
	uint8_t bitmap_cols = x_b;
	uint8_t bitmap_rows = (y_b + 7)/8;
	uint16_t i = 0;
	
	for (uint8_t drawn_rows = 0; drawn_rows < bitmap_rows; drawn_rows++) {
		for (uint8_t drawn_cols = 0; drawn_cols < bitmap_cols; drawn_cols++) {
			if (i < bitmap_length) {
				LCD_Screen_Buffer[(col + drawn_cols + (row + drawn_rows) * 84)%504] |= bitmap[i];
				i++;
			}
		}
	}
}

// Writing
void LCD_printInt(uint16_t n, uint8_t col, uint8_t row, char justification) {
	char n_str[5];
	utoa(n, n_str, 10);
	
	//LCD_printSmallFont("     ", 6 * (strlen(n_str) + 1), strlen(n_str) + 1, col, row, justification); // Clear previous number
	LCD_printSmallFont(n_str, 6 * strlen(n_str), strlen(n_str), col, row, justification);
}

void LCD_printFP(int16_t n, uint16_t SF_exp, uint8_t d_places, uint8_t col, uint8_t row, char justification) {
	
	char n_str[7];	// -32768 to 32767
	char d_str[7];	// Up to 7 decimal places (2^-16 = 0.0000153)
	char str[11];
	
	// Whole part string
	itoa(n >> SF_exp, n_str, 10);
	strcpy(str,n_str);
	
	// Decimal point
	strcat(str,".");
	
	
	// Decimal part string
	uint16_t d = n & ((1 << SF_exp) - 1);
	// Zero padding	
	if (SF_exp >= 4 && d <= (1 << (SF_exp - 4))) {
		strcat(str,"0");
		
		if (SF_exp >= 7 && d <= (1 << (SF_exp - 6))) {
			strcat(str,"0");
			
			if (SF_exp >= 10 && d <= (1 << (SF_exp - 9))) {
				strcat(str,"0");
				
				if (SF_exp >= 14 && d <= (1 << (SF_exp - 13))) {
					strcat(str,"0");
				}
			}
		}
	}
	
	uint32_t pwrs10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000};
	// Convert decimals to correct string
	utoa(((uint32_t)d * pwrs10[d_places]) >> SF_exp, d_str, 10);
	
	strcat(str,d_str);
	
	// Print
	LCD_printSmallFont(str, 6 * strlen(str), strlen(str), col, row, justification);
}

void LCD_setFont(const char *pgm_start_location, uint8_t px_width, uint8_t px_height) {
	LCD_font = (struct Font){pgm_start_location, px_width, (px_height >> 3) + 1};
}

#ifdef TinyFont_FLAG
#include "TinyFont.h"
void LCD_printTinyFont(char string[], uint8_t width, uint8_t length, uint8_t col, uint8_t row) {
	unsigned char stringBitmap[length * 4];
	for (uint8_t i = 0; i < length; i++) {
		for (uint8_t j = 0; j < 4; j++) {
			stringBitmap[i * 4 + j] = pgm_read_byte(&TinyFont[4 * (string[i] - 32) + j]);
		}
	}
	LCD_draw(stringBitmap, arrayLength(stringBitmap), width, ((length * 4) / width) * 8 + 1, col, row);
}
#endif

#ifdef SmallFont_FLAG
#include "SmallFont.h"
void LCD_printSmallFont(char string[], uint8_t width, uint8_t length, uint8_t col, uint8_t row, char justification) {
	unsigned char stringBitmap[length * 6];
	uint8_t x;
	
	for (uint8_t i = 0; i < length; i++) {
		for (uint8_t j = 0; j < 6; j++) {
			stringBitmap[i * 6 + j] = pgm_read_byte(&SmallFont[6 * (string[i] - 32) + j]);
		}
	}
	
	switch(justification) {
		case 'c':
			x = col - (width >> 1);
			break;
			
		case 'r':
			x = col - width;
			break;
		
		default :
			x = col;
	};
	
	LCD_draw(stringBitmap, arrayLength(stringBitmap), width, ((length * 6) / width) * 8 + 1, x, row);
}
#endif

#ifdef AdaFont_FLAG
#include "AdaFont.h"
void LCD_printAdaFont(char string[], uint8_t width, uint8_t length, uint8_t col, uint8_t row) {
	unsigned char stringBitmap[length * 5];
	for (uint8_t i = 0; i < length; i++) {
		for (uint8_t j = 0; j < 5; j++) {
			stringBitmap[i * 5 + j] = pgm_read_byte(&AdaFont[5 * (string[i] - 32) + j]);
		}
	}
	LCD_draw(stringBitmap, arrayLength(stringBitmap), width, ((length * 5) / width) * 8 + 1, col, row);
}
#endif

#ifdef SinclairFont_FLAG
#include "SinclairFont.h"
void LCD_printSinclairFont(char string[], uint8_t width, uint8_t length, uint8_t col, uint8_t row) {
	unsigned char stringBitmap[length * 6];
	for (uint8_t i = 0; i < length; i++) {
		for (uint8_t j = 0; j < 6; j++) {
			stringBitmap[i * 6 + j] = pgm_read_byte(&SinclairFont[6 * (string[i]) + j]);
		}
	}
	LCD_draw(stringBitmap, arrayLength(stringBitmap), width, ((length * 6) / width) * 8 + 1, col, row);
}
#endif









/*
* Functions with structs as arguments
*/


void LCD_printSmallFont_s(struct Text *text) {
	uint8_t char_width = LCD_font.px_width;
	
	uint8_t length = (text->char_length) ? text->char_length : strlen(text->string);
	uint8_t px_width = (text->px_width) ? text->px_width : length * LCD_font.px_width;
	
	unsigned char stringBitmap[length * LCD_font.px_width];
	
	for (uint8_t i = 0; i < length; i++) {
		for (uint8_t j = 0; j < LCD_font.px_width; j++) {
			//stringBitmap[i * 6 + j] = pgm_read_byte(&SmallFont[6 * (*(text->string + i) - 32) + j]);
			stringBitmap[i * LCD_font.px_width + j] = 
						pgm_read_byte(LCD_font.pgm_start_location + (LCD_font.px_width * (*(text->string + i) - 32) + j));
		}
	}
	
	uint8_t col;
	switch(text->justification) {
		case 'c':
			col = *(text->pos) - (px_width >> 1);
			break;
			
		case 'r':
			col = *(text->pos) - px_width;
			break;
		
		default :
			col = *(text->pos);
	};
	
	LCD_draw(stringBitmap, arrayLength(stringBitmap), px_width, 
				((length * LCD_font.px_width) / px_width) * 8 + 1, col, *(text->pos + 1));
}









// Drawing
void LCD_drawPixel(uint8_t x, uint8_t y) {
	LCD_Screen_Buffer[x + (y / 8) * 84] |= (1 << (y & 0x07));
}

void LCD_drawLine (int8_t x0, int8_t y0, int8_t x1, int8_t y1) {
  int8_t dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
  int8_t dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int err = dx + dy, e2; // error value e_xy 
 
  for (;;){  // loop
    LCD_drawPixel(x0, y0);
	
    if (x0 == x1 && y0 == y1) break;
	
    e2 = 2 * err;
	
    if (e2 >= dy) { err += dy; x0 += sx; } // e_xy + e_x > 0
    if (e2 <= dx) { err += dx; y0 += sy; } // e_xy + e_y < 0
  }
}

void LCD_drawRectangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, int8_t r) {
	
	// Swap coordinates if necessary
	if (x0 > x1) {
		uint8_t x_temp = x1;
		x1 = x0;
		x0 = x_temp;
	}
	
	if (y0 > y1) {
		uint8_t y_temp = y1;
		y1 = y0;
		y0 = y_temp;
	}
	
	// Draw edges
	// x0 < x1 and y0 < y1
	LCD_drawLine(x0 + r, y0, x1 - r, y0); // y0 horizontal
	LCD_drawLine(x0 + r, y1, x1 - r, y1); // y1 horizontal
	LCD_drawLine(x0, y0 + r, x0, y1 - r); // x0 vertical
	LCD_drawLine(x1, y0 + r, x1, y1 - r); // x1 vertical
	
	// Draw radiuses
	int8_t x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
	int8_t radius = r;
	
	do {
      LCD_drawPixel(x1 - x - radius, y0 - y + radius); /*   I. Quadrant */
      LCD_drawPixel(x0 - y + radius, y1 - x - radius); /*  II. Quadrant */
      LCD_drawPixel(x0 + x + radius, y0 - y + radius); /* III. Quadrant */
      LCD_drawPixel(x1 + y - radius, y1 - x - radius); /*  IV. Quadrant */
	  
      r = err;
	  
      if (r >  x) err += ++x * 2 + 1; /* e_xy + e_x > 0 */
      if (r <= y) err += ++y * 2 + 1; /* e_xy + e_y < 0 */
   } while (x < 0);
}

void LCD_drawBanner(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, int8_t r) {
	
	// Swap coordinates if necessary
	if (x0 > x1) {
		uint8_t x_temp = x1;
		x1 = x0;
		x0 = x_temp;
	}
	
	if (y0 > y1) {
		uint8_t y_temp = y1;
		y1 = y0;
		y0 = y_temp;
	}
	
	// Draw edges
	// x0 < x1 and y0 < y1
	LCD_drawLine(x0 + r, y0, x1 - r, y0); // y0 horizontal
	LCD_drawLine(x0 + r, y1, x1 - r, y1); // y1 horizontal
	LCD_drawLine(x0, y0 + r, x0, y1 - r); // x0 vertical
	LCD_drawLine(x1, y0 + r, x1, y1 - r); // x1 vertical
	
	// Draw radiuses
	int8_t x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
	
	do {
      LCD_drawPixel(x0 - x, y0 + y); /*   I. Quadrant */
      LCD_drawPixel(x1 - y, y0 - x); /*  II. Quadrant */
      LCD_drawPixel(x1 + x, y1 - y); /* III. Quadrant */
      LCD_drawPixel(x0 + y, y1 + x); /*  IV. Quadrant */
	  
      r = err;
	  
      if (r >  x) err += ++x * 2 + 1; /* e_xy + e_x > 0 */
      if (r <= y) err += ++y * 2 + 1; /* e_xy + e_y < 0 */
   } while (x < 0);
}

void LCD_drawCircle(uint8_t xm, uint8_t ym, int8_t r) {
   int8_t x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */ 
   
   do {
      LCD_drawPixel(xm - x, ym + y); /*   I. Quadrant */
      LCD_drawPixel(xm - y, ym - x); /*  II. Quadrant */
      LCD_drawPixel(xm + x, ym - y); /* III. Quadrant */
      LCD_drawPixel(xm + y, ym + x); /*  IV. Quadrant */
	  
      r = err;
	  
      if (r >  x) err += ++x * 2 + 1; /* e_xy + e_x > 0 */
      if (r <= y) err += ++y * 2 + 1; /* e_xy + e_y < 0 */
   } while (x < 0);
}
