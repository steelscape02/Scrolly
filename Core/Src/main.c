/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "drv_eeprom24xx08.h"
#include "drv_i2clcd.h"
#include "app_comm.h"

#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_ROWS 2 // Number of rows on the LCD
#define LCD_COLS 16 // Number of columns on the LCD

// EEPROM definitions - for V1, this will only have block 0 for the message
#define MESSAGE_BLOCK 0 // Block 0 of the EEPROM
#define MESSAGE_START_ADDRESS 0 // Start address of the message in EEPROM
#define MESSAGE_MAX_LENGTH 256 // Size of the one allocated block in EEPROM for messages

#define RING_BUFFER_SIZE 1024
#define MAX_LEN 256
#define MAX_MESSAGE_SIZE 100 // 100 characters maximum message size

#define NO_COMMAND "NO_CMD"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_rx;

/* USER CODE BEGIN PV */
static bool display = false;
static bool scroll = false;
static bool new_message_ready = false;
static bool motion_detected;
static bool backlight_state = false;
uint8_t I2C_ADDR =0x27; // I2C address of the PCF8574

char message[MAX_MESSAGE_SIZE] = {0}; // char array to store message received
uint8_t uart2_byte; // byte received from UART2
uint8_t buffer_position = 0; // how many bytes received so far in message

char command[30] = "";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
void Handle_UART(UART_HandleTypeDef *huart);
char* FindCommand(char *buffer, char *str);
bool StartsWith(const char *str, const char *prefix);
void SplitAndRemove_String(char *str,char *sub);

void LCD_Backlight_On(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);
void LCD_Backlight_Off(I2C_HandleTypeDef *hi2c1, uint8_t I2C_ADDR);



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  CharLCD_Init(&hi2c1, I2C_ADDR);
  HAL_StatusTypeDef eeprom_status;
  HAL_UART_RegisterCallback(&huart2, HAL_UART_RX_COMPLETE_CB_ID, Handle_UART);

  HAL_UART_Receive_IT(&huart2, &uart2_byte, 1); // put byte from UART2 in "uart2_byte"

  PWR_PVDTypeDef sConfigPVD = {0};
  __HAL_RCC_PWR_CLK_ENABLE();

  sConfigPVD.PVDLevel = PWR_PVDLEVEL_3;
  sConfigPVD.Mode = PWR_MODE_IT_RISING_FALLING;
  HAL_PWR_PVDConfig(&sConfigPVD);

  HAL_PWR_EnablePVD();

  eeprom_status = readFromEEPROM(&hi2c1);
  HAL_Delay(5);

  if (eeprom_status == HAL_OK) {
    message[0] = '\0';
    new_message_ready = true;
  } else if (eeprom_status != HAL_OK) {
    char *msg = "EEPROM read failed\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (motion_detected != backlight_state) {
		  if (motion_detected) {
			  LCD_Backlight_On(&hi2c1, I2C_ADDR);
        display = true;
			  backlight_state = motion_detected;
		  } else {
			  LCD_Backlight_Off(&hi2c1, I2C_ADDR);
        CharLCD_Clear(&hi2c1, I2C_ADDR); // Clear the display
        backlight_state = motion_detected;
        continue;
		  }
	  }else if(!motion_detected){
     continue;
    }

	  if (new_message_ready) {
		  new_message_ready = false;
      if (strncmp(message, "add", 3) == 0) {
        SplitAndRemove_String(message, "add");
        add(message, sizeof(message));
      } else if (strncmp(message, "rem", 3) == 0) {
        rem();
      } else if (strncmp(message, "clr", 3) == 0) {
        clr(&hi2c1, I2C_ADDR);
      } else {
        char* msg = "Unrecognized command\n\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      }

      // TODO: Add message length check
      // Something like: app_comm NeedsScroll with bool retval
      buildMessage();
      display = true;
      if(needsScroll(LCD_COLS)){
        scroll = true;
      } else {
        scroll = false;
      }
		  memset(message, 0, sizeof(message));
		  buffer_position = 0;
	  }
	  if(display){
      CharLCD_Clear(&hi2c1, I2C_ADDR);
		  if(scroll){ // Check scroll value
        writeScrolling(&hi2c1, I2C_ADDR, LCD_COLS);
        HAL_Delay(500);
		  }else{
        writeNoScroll(&hi2c1, I2C_ADDR);
        display = false;
		  }
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
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
  hi2c1.Init.Timing = 0x10D19CE4;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MOTION_SENS_Pin */
  GPIO_InitStruct.Pin = MOTION_SENS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MOTION_SENS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief Handles UART reception and processes incoming bytes.
 * @param huart Pointer to the UART handle struct.
 */
void Handle_UART(UART_HandleTypeDef *huart){
	if (huart->Instance != USART2) return;

	if ((uart2_byte != '\r') && (uart2_byte != '\n') && (uart2_byte != '\0')){
		if (buffer_position < MAX_MESSAGE_SIZE - 1){
			message[buffer_position++] = uart2_byte;
		}
	} else {
		message[buffer_position] = ' ';
		message[buffer_position + 1] = ' ';
		message[buffer_position + 2] = '\0';
		new_message_ready = true;   // just flag it
	}
	HAL_UART_Receive_IT(&huart2, &uart2_byte, 1);
}

/**
 * @brief Splits a string and removes a specified substring from it.
 * @param str The original string to modify.
 * @param sub The substring to remove from the original string.
 */
void SplitAndRemove_String(char *str, char *sub) {
    char new_sub[32];
    snprintf(new_sub, sizeof(new_sub), "%s ", sub);

    char *pos = strstr(str, new_sub);
    if (pos != NULL) {
        size_t sub_len = strlen(new_sub);
        memmove(pos, pos + sub_len, strlen(pos + sub_len) + 1);
    }
}

/**
 * @brief Callback function for GPIO external interrupts.
 * @param GPIO_Pin The pin that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == MOTION_SENS_Pin){
		if(HAL_GPIO_ReadPin(MOTION_SENS_GPIO_Port, MOTION_SENS_Pin) == GPIO_PIN_SET){
			motion_detected = true;
		}else{
			motion_detected = false;
		}
	}
}

/**
 * @brief Callack function for Power Voltage Detection (PVD) interrupts. Triggers when voltage drops below 2.9V
 */
void HAL_PWR_PVDCallback(void){
  //writeToEEPROM();
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
	  //TODO: Add error handler that writes to LCD with error
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
