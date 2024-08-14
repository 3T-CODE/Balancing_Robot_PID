/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <stdint.h>
#include "mpu6050.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
MPU6050_t MPU6050;
volatile int encoder1 = 0;
volatile int encoder2 = 0;
volatile float Ax = 0;
volatile float Ay = 0;
volatile float Az = 0;
volatile float Gx = 0;
volatile float Gy = 0;
volatile float Gz = 0;
volatile float Kalman_X = 0;
volatile float Kalman_Y = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Control Motor
void Motor_Control(int16_t M2_velocity , int16_t M1_velocity)
{
	//Left Motor
	if(M1_velocity < 0)
	{
		TIM3 -> CCR1 = 0;
		TIM3 -> CCR2 = - M1_velocity;
	}

	else if(M1_velocity > 0)
	{
		TIM3 -> CCR1 = M1_velocity;
		TIM3 -> CCR2 = 0;
	}
	//Right Motor
	if(M2_velocity < 0)
	{
		TIM3 -> CCR3 = 0;
		TIM3 -> CCR4 = - M2_velocity;
	}

	else if(M2_velocity > 0)
	{
		TIM3 -> CCR3 = M2_velocity;
		TIM3 -> CCR4 = 0;
	}

}


void PID_Control(float _Kp , float _Ki , float _Kd , float angle)
{
	float angleError ;
	float oldAngleError = 0;
	float I = 0;
	float P = 0, D = 0;
	float Total = 0;
	int16_t increased_speed = 0, decreased_speed = 0;

	//Calculate PID
	angleError = -2.51846361 - angle;
	P = _Kp * angleError ;
	D = _Kd * (angleError - oldAngleError);

	oldAngleError = angleError;

	if((- 0.2 < angleError) && ( angleError < 0.2))
	{
		I = I + (_Ki * angleError);
	}
	else
	{
		_Ki = 0;
	}

	Total = P + I + D;

	increased_speed =  + ((Total > 0) ? Total : -Total);
	decreased_speed =  - ((Total > 0) ? Total : -Total);

	if(increased_speed < -900)
		increased_speed = -900;
	if(increased_speed > 900)
		increased_speed = 900;
	if(decreased_speed < -900)
		decreased_speed = -900;
	if(decreased_speed > 900)
		decreased_speed = 900;

	//Control Motor
	if (angleError > 0)
	{
		Motor_Control(decreased_speed, decreased_speed);
	}
	else if (angleError < 0)
	{
		Motor_Control(increased_speed, increased_speed);
	}
	else
	{
		Motor_Control(0 , 0);
	}
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_1 | TIM_CHANNEL_2);
  HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_1 | TIM_CHANNEL_2);

  while (MPU6050_Init(&hi2c2) == 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  char [100] ;
	  MPU6050_Read_All(&hi2c2, &MPU6050);
	  Ax = MPU6050.Ax;
	  Ay = MPU6050.Ay;
	  Az = MPU6050.Az;
	  Gx = MPU6050.Gx;
	  Gy = MPU6050.Gy;
	  Gz = MPU6050.Gz;
	  Kalman_X = MPU6050.KalmanAngleX;
	  Kalman_Y = MPU6050.KalmanAngleY;

	  PID_Control(85, 0.02, 1.0, Kalman_Y);


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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL5;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
