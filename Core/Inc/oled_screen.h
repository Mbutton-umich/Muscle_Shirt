/*
 * oled_screen.h
 *
 *  Created on: Apr 4, 2023
 *      Author: nazariik
 */

#ifndef INC_OLED_SCREEN_H_
#define INC_OLED_SCREEN_H_

/* CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include "main.h"
/* CODE END Includes */

/* CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void oled_init(void);
void oled_writeChest(char* str);
void oled_writeLat(char* str);
void oled_writeBicep(char* str);
void oled_writeTricep(char* str);
void oled_writePushUp(char* str);
void oled_writePumpLevel(char* str);
/* CODE END PFP */

#endif /* INC_OLED_SCREEN_H_ */
