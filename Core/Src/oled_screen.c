/*
 * oled_screen.c
 *
 *  Created on: Apr 4, 2023
 *      Author: nazariik
 */

/* CODE BEGIN Includes */
#include "oled_screen.h"
#include "muscle_logo.h"
/* CODE END Includes */

/* CODE BEGIN Public functions */
void oled_init() {
	  ssd1306_Init();
	  ssd1306_Clear();
	  ssd1306_SetColor(White);

	  // muscle logo
	  ssd1306_DrawBitmap2(0, 0, 128, 64, muscle_logo);

	  // text for push-ups
	  ssd1306_SetCursor(75,10);
	  ssd1306_WriteString("Push", Font_7x10);
	  ssd1306_SetCursor(75,20);
	  ssd1306_WriteString("Ups", Font_7x10);
	  ssd1306_SetCursor(107,15);
	  ssd1306_WriteString(": ", Font_7x10);

	  // text for pump level
	  ssd1306_SetCursor(75,40);
	  ssd1306_WriteString("Pump", Font_7x10);
	  ssd1306_SetCursor(75,50);
	  ssd1306_WriteString("Level", Font_7x10);
	  ssd1306_SetCursor(107,45);
	  ssd1306_WriteString(": ", Font_7x10);

	  ssd1306_UpdateScreen();
}

void oled_writeChest(char* str) {
	ssd1306_SetCursor(50,30);
	ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}

void oled_writeLat(char* str) {
	ssd1306_SetCursor(55,53);
	ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}

void oled_writeBicep(char* str) {
	ssd1306_SetCursor(20,15);
	ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}

void oled_writeTricep(char* str) {
	ssd1306_SetCursor(20,50);
	ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}

void oled_writePushUp(char* str) {
	  ssd1306_SetCursor(113,15);
	  ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}

void oled_writePumpLevel(char* str) {
	  ssd1306_SetCursor(113,45);
	  ssd1306_WriteString(str, Font_7x10);
	ssd1306_UpdateScreen();
}
/* CODE END Public functions */



