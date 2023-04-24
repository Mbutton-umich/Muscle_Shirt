/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"				//For printf() debugging
#include "oled_screen.h"		//Obviously for the oled WE DID NOT WRITE THIS
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

//For the moving average filter
#define WINDOW_SIZE			8						//Moving average filter window size

//FOR THE ADC
#define EMG_COUNT			5						//Number of EMGs
#define STRAIN_COUNT		2						//Number of strain sensors
#define SENSOR_COUNT 		EMG_COUNT+STRAIN_COUNT 	//EMG + Strain
#define ADC_BUFFER_LENGTH	WINDOW_SIZE+3 			// 8 Window time Slots the MAX, SUM, and AVG
#define ADC_DATA_LENGTH		WINDOW_SIZE				//Width of the moving average filter that stores the data in ADC_Val
#define MAX_INDEX			WINDOW_SIZE+2			//The location the max in the tail end of the of ADC_Val
#define SUM_INDEX			WINDOW_SIZE+1			//The location the running sum in the tail end of the of ADC_Val
#define AVG_INDEX			WINDOW_SIZE				//The location the moving avg in the tail end of the of ADC_Val
#define ADC_TIMEOUT			1000 					//Millisecond timeout value for an ADC conversion

//FOR THE LEDS
#define BITS_PER			24						//3 colors of 8 bits each
#define RES_FRAME			250 					// 800kHz > T= 1.25 us, TODO: datasheet lists both  300 and also 250 us for frame reset length meaning 200 slots of PWM
#define MAX_LED 			50+10 					//(Set this to ten over because the final LED glitches for some reason)
#define BRIGHTNESS 			0.25					//Percentage of full brightness 25% gets up just to .200 A
#define BAR_SIZE 			10 						//Number of LEDs that form a bar graph at a muscle site

//FOR THE ACTIVITY LOG
#define DEPTH				32 					//Number of samples that show binary representation of activity changes
#define ACTIVITY_THRESHOLD	0.62					//Percentage of max EMG value (TODO: POWER of 2 will be faster)that is considered "Active" for the sake of recording activity
													//TODO: EXTREEMLY FINICKY: Reduced to 0.62 because a push-up is just not that hard for our purposes
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */
//FOR THE MOVING AVERAGE FILTER
uint8_t window_timeslot = 0;									//Current position within the moving average filter
uint8_t channel = 0;											//Current EMG in terms of MUX address

//FOR THE ADC

volatile uint32_t 	ADC_Val[EMG_COUNT][ADC_BUFFER_LENGTH];		//Stores raw ADC data for every channel, the length is 8 for the moving average

//FOR THE LEDS
uint8_t bar_scalers[EMG_COUNT] = {1,1,1,1,1};					//Balances out the LED ouputs so they all look good on demo day (digital gain adjustments)
uint8_t LEDs[MAX_LED][3]; 										//LED Data that is changed reflecting true colors
uint8_t COLOR_HEATMAP[11][3]= {	{0,0,0}, 						//Heat map from green to yellow to dark red to indicate pump keep B off to reduce power
								{0, 255, 0},
								{51, 204, 0},
								{77, 178, 0},
								{102, 153, 0},
								{127,127,0},
								{153, 102, 0},
								{178, 77, 0},
								{204, 51, 0},
								{255, 0, 0},
								{255, 255, 255}}; 				//Setup as RGB
volatile uint8_t   PUMP[EMG_COUNT]; 							//Defines the current pump level 1-9, zero 0s nothing, 10 is all white
uint16_t PWM[(BITS_PER*MAX_LED)+(RES_FRAME)]; 					//PWM data that gets updated by the send function and sent via DMA there are 24 bits per individual LED plus 2 tail end bits for resetting between transmissions
volatile int DMA_Done = 0; 										//Flag to indicate a DMA transfer is ongoing

//FOR BUTTON
uint8_t clicked = 1;											//Single state keeper to prevent a second button event from firing while dealing with one

//FOR ACTIVITY MONITOR

uint8_t new_activity[EMG_COUNT]; 			//The activity fingerprint for one cycle through all the sensors, equivalently one timeslot
uint8_t activity_log[EMG_COUNT][DEPTH]; 	//Array that updates columns whenever the pattern of muscle activation changes, depth is the column count, essentially the memory for changes
uint8_t activity_index = 0;					//Location of the last updated activity column [0, DEPTH]
uint8_t push_ups = 0;						//The number of push-ups
uint32_t muscle_sums[EMG_COUNT];			//The activation counts for each muscle site
uint32_t rep_activations = 128;				//This is the number of activation peaks that translate to a single rep

//FOR FLEX SENSOR
uint16_t strain_start = 3480;
uint16_t strain_base = 3480;				//This is what the strain starts out as about 85% of the 5V max
float strain_ratio = 1;						//This minute chain in the shirt tension affects how quickly the muscles turn red by using this ratio


//For Screen
char itoa_buffer[3];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//FOR THE ADC
//Used to read EMG data coming off the mux
void ADC_Select_EMG_MUX(void){
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_9;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK){
	  Error_Handler();
	}

}
//Adjust to another ADC channel to read strain gauges
void ADC_Select_Strain_1(void){
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_10;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK){
		Error_Handler();
	}
}

//FOR THE LEDS
//Sets a specific LED to a specific color scaled by brightness
void set_led(uint8_t LEDnum, uint8_t RGB[3]){
	//Note the ordering of the matrix is GRB, not RGB, this is deliberate for the LED strips protocol
	LEDs[LEDnum][0] = RGB[1]*BRIGHTNESS;
	LEDs[LEDnum][1] = RGB[0]*BRIGHTNESS;
	LEDs[LEDnum][2] = RGB[2]*BRIGHTNESS;
}
//Input some level from the ADC and then you get a resulting led bar graph
void set_bar_level(uint8_t channel, float level) {
	//First scale down the raw ADC EMG value to be Some number capped at 10
	//TODO: Initialize all the ADC_Val[channel][MAX_INDEX] values to 1 will prevent a divide by 0 crash, for now I just add a one but initialization will avoid stupid addition EVERY TIME
	uint16_t scaled = ((level*bar_scalers[channel])/(((ADC_Val[channel][MAX_INDEX]))))*10;

	for(int i = BAR_SIZE*channel; i < BAR_SIZE*(channel+1); ++i){
		if((i%10) < (10-scaled)) {
			set_led(i, COLOR_HEATMAP[0]);
		}else {
			set_led(i, COLOR_HEATMAP[PUMP[channel]]);
		}
	}
}

//Sends an update to DMA PWM matrix controlling LEDs
void send_DMA(void){
	uint32_t buffer_index=0;
	uint32_t GRB;
	//Loop though every LED and Update the values per bit so PWM gets set to corrrdct value
	for (int i= 0; i<MAX_LED; i++) {
		GRB = ((LEDs[i][0]<<16) | (LEDs[i][1]<<8) | (LEDs[i][2]));
		for (int i=23; i>=0; --i){
			//Recall 80M Hz clock / 100 period counter = 800k Hz desired data rate
			if (GRB&(1<<i)){
				PWM[buffer_index++] = 70;  // 7/10 of 100 TODO: These were set off of 70 and 30 to reduce flickering
			} else {
				PWM[buffer_index++] = 30;  // 3/10  of 100
			}
		}
	}
	//There is some reset time required between sends supposed to be  either 250 us or 300 datasheet is unclear
	//Also seems to work without any...
	for (int i=0; i<250; i++) {
		PWM[buffer_index++] = 0;
	}
	//Start DMA and wait till its done setting flags accordingly
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)PWM, buffer_index);
	while (!DMA_Done){};
	DMA_Done = 0;
}
//Callback to make sure DMA transfers do not step on each other sending LED data pulses
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	DMA_Done=1;

}

//Updates the EMG value for a given channel and given timeslot
void update_EMG_value(uint32_t channel, uint32_t window_timeslot){
	//Spin up the ADC channel for the EMGs
	ADC_Select_EMG_MUX();
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, (uint32_t)ADC_TIMEOUT);
	//Save the old value in this timeslot
	uint32_t prior = ADC_Val[channel][window_timeslot];
	//Replace the old value with the new
	ADC_Val[channel][window_timeslot] = HAL_ADC_GetValue(&hadc1);
	//Stop the ADC
	HAL_ADC_Stop(&hadc1);
	//Recompute the running sum adding the new and subtracting the leaving old value as it passes out of the window
	ADC_Val[channel][SUM_INDEX]  = ADC_Val[channel][SUM_INDEX]  + ADC_Val[channel][window_timeslot] - prior;
	//Recompute the average from the updated sum
	ADC_Val[channel][AVG_INDEX] = ADC_Val[channel][SUM_INDEX]  /ADC_DATA_LENGTH;
	if(ADC_Val[channel][MAX_INDEX]  < ADC_Val[channel][window_timeslot]){
		ADC_Val[channel][MAX_INDEX]  = ADC_Val[channel][window_timeslot];
	}
	//Compare whatever value we just recorded to some percentage of the max. If we are over that threshold mark a 1 in the activity vector

	new_activity[channel] = (ADC_Val[channel][window_timeslot] > (ACTIVITY_THRESHOLD*ADC_Val[channel][MAX_INDEX]));
}

//Print the output of the moving average filter to console, easy to see via Arduino serial plotter
void DEBUG_print_EMG(uint8_t channel){
	//Debug print to output only
	printf("\n%u", ADC_Val[channel][AVG_INDEX] );
}

void update_strain(){
	ADC_Select_Strain_1();
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, (uint32_t)ADC_TIMEOUT);
	uint16_t strain_new = HAL_ADC_GetValue(&hadc1);
	printf("\n%u", strain_new);
	HAL_ADC_Stop(&hadc1);
	if(strain_new > strain_base){
		strain_base = strain_new;
	}

}


//Neatly prints the activity matrix for debugging via Arduino serial output
void DEBUG_print_AM(void){
	//Debug print to output only
	printf("\n");
	for(uint8_t i =0; i<EMG_COUNT; ++i){
		printf("%u\t", activity_log[i][activity_index]);
	}
}

/*After cycling through all the sensors we will have some binary vector showing what muscles were on during a timeslot
 * this vector named: "new_activity" is then compared against the most recent entry in the
 */

void set_pump(float burn, uint8_t channel){
	float reps = (burn+1)/(rep_activations*strain_ratio);
	PUMP[channel] = (uint8_t)(reps) + 1;
	if(PUMP[channel] >9){
		PUMP[channel] = 9;
	}
	switch (channel) {
		case 0:
			itoa(PUMP[channel], itoa_buffer,10);
			oled_writeLat(itoa_buffer);
			break;
		case 1:
			itoa(PUMP[channel], itoa_buffer,10);
			oled_writeChest(itoa_buffer);
			break;
		case 2:
			//Shoulder doesnt fit alledgely
			break;
		case 3:
			itoa(PUMP[channel], itoa_buffer,10);
			oled_writeTricep(itoa_buffer);
			break;
		case 4:
			itoa(PUMP[channel], itoa_buffer,10);
			oled_writeBicep(itoa_buffer);
			break;
	}
}
void update_activity(void){
	//Cycle through every member of the array
	uint8_t different = 0;
	uint8_t inactive = 0;
	for(uint8_t i = 0; i<EMG_COUNT; ++i){
		//If the activity in the new column is different from the latest entry then add it
		inactive += new_activity[i];
		if(new_activity[i] != activity_log[i][activity_index]){
			//set the flag
			different = 1;
			//Break out of the loop for even a single difference, break early for optimal speed
			break;
		}
	}
	if(different) {
		//Write to some slot on the log by incrementing with wraparound
		activity_index = (activity_index+1)%DEPTH;
		//Copy this changed column into the log
		uint32_t total_pump = 0;
		for(uint8_t j = 0; j < EMG_COUNT; ++j){
			total_pump += PUMP[j];
			activity_log[j][activity_index] = new_activity[j];
			muscle_sums[j]+= new_activity[j];
		}
		itoa((total_pump/EMG_COUNT), itoa_buffer,10);
		oled_writePumpLevel(itoa_buffer);
		//DEBUG_print_AM();
		//printf("\n%u", muscle_sums[4]);

	}
	//If no muscle is currently active
	if(!inactive){
		//Determine the strain stuff
		update_strain();
		//Ratio represents how much the shirt is stretched, tighter shirt means pump is higher, go red easier
		strain_ratio = (float)(strain_base) / (float)(strain_start);
	}
	//If we made it to the end of the log, count the number of pushups so far
	if(activity_index == DEPTH){
		activity_index =0;
		for(uint8_t k = 1; k < DEPTH; ++k){
			//If chest and tri both activate then both go low
			if(activity_log[1][k-1] && activity_log[3][k-1] && !(activity_log[1][k]) && !(activity_log[3][k])){
				++push_ups;
				//Don't want to reuse the data from the top half so increment k another time
				++k;
				itoa(push_ups, itoa_buffer,10);
				oled_writePushUp(itoa_buffer);
			}
		}
	}

}




//SHIT FOR THE MUX
//Changes the mux to some other channel to read a different EMG site
void rotate_channel(uint8_t channel) {
	//TODO: Just do a lookup table for all the 8 or so sensor value and avoid the bit shifting and anding for speed
	((channel & 0b1) == 0b1)		?	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET): HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
	(((channel>>1) & 0b1) == 0b1)	? 	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET): HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	(((channel>>2) & 0b1) == 0b1)	? 	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET): HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

void restart(void){
	strain_start = 3480;
	strain_base = 3480;
	strain_ratio = 1;
	for(unsigned int i = 0; i < EMG_COUNT; ++i){
		muscle_sums[i]=0;
		PUMP[i]=1;
		for(unsigned int j = 0; j < WINDOW_SIZE+2; ++j){
			ADC_Val[i][j] = 0;
		}
	}
}

void init_maxes(void){
	for(unsigned int i = 0 ; i < EMG_COUNT; ++i){
		//Maxes avoid divide by zero error
		ADC_Val[i][MAX_INDEX] = 4096;
		//Pump must start at 1 because 0 is off
		PUMP[i] = 1;
	}
}

//ISR fires when button is pressed and starts a timer that counts up to 50 ms for debouncing TODO: Crucial that both the button interrupts have lower (meaning higher) priorty compared to the DMA
//The DMA cannot be blocked mid stream or gets caught in a deadlock
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if((GPIO_Pin == GPIO_PIN_12) && (clicked > 0)){
		HAL_TIM_Base_Start_IT(&htim2);
		clicked = 0;
		DMA_Done = 1;
	}
}

//Timer for de-bouncing the restart button
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET){
	  //Send all white
	  restart();
	  for(unsigned int i = 0; i < 4; ++i) {
		  if(i%2 == 0) {
			  for(unsigned int j = 0; j < MAX_LED; ++j){
				  set_led(j, COLOR_HEATMAP[10]);
			  }
		  } else {
			  for(unsigned int k = 0; k < MAX_LED; ++k){
				  set_led(k, COLOR_HEATMAP[0]);
			  }
		  }
		  send_DMA();
		  HAL_Delay(250);
	  }
  }
  DMA_Done = 1;
  clicked = 1;
  HAL_TIM_Base_Stop_IT(&htim2);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  // OLED test


  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_LPUART1_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  //Set the EN to on for the MUX
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  oled_init();
  init_maxes();
  window_timeslot = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  for(unsigned int i =  0; i < 5 ; ++i) {
		  rotate_channel(i);
		  update_EMG_value(i, window_timeslot);
		  set_bar_level(i, ADC_Val[i][AVG_INDEX]);
	  }
	  update_activity();
	  //DEBUG_print_EMG(0);

	  window_timeslot = (window_timeslot + 1) % WINDOW_SIZE;
	  if(window_timeslot == 0){
		  send_DMA();
	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }


}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 100-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4000000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MUX_A1_A5__GPIO_Port, MUX_A1_A5__Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, MUX_EN_D3__Pin|LD3_Pin|MUX_A2_D11__Pin|MUX_A0_D4__Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : MUX_A1_A5__Pin */
  GPIO_InitStruct.Pin = MUX_A1_A5__Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MUX_A1_A5__GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : MUX_EN_D3__Pin LD3_Pin MUX_A2_D11__Pin MUX_A0_D4__Pin */
  GPIO_InitStruct.Pin = MUX_EN_D3__Pin|LD3_Pin|MUX_A2_D11__Pin|MUX_A0_D4__Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Button_D2__Pin */
  GPIO_InitStruct.Pin = Button_D2__Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button_D2__GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : VCP_RX_Pin */
  GPIO_InitStruct.Pin = VCP_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF3_USART2;
  HAL_GPIO_Init(VCP_RX_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
