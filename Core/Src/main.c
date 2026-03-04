/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>  // ��Ӵ���
#include <inttypes.h>  // 添加这个头文件
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_PIN GPIO_PIN_3
#define KEY_GPIO_PORT GPIOA
#define KEY_PRESSED 0  // 根据实际硬件修改，下拉输入时按键按下为1，上拉输入时为0
#define STOP_DELAY 1000  // 停止前的延迟时间(ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t measure_state = 0;  // 0: 未测量 1: 正在测量
uint8_t key_prev_state = 0;  // 按键上一状态
uint32_t debounce_delay = 250;  // 按键消抖延时(ms)
uint32_t last_key_time = 0;   // 上次按键时间
uint8_t stop_flag = 0;      // 0: 正常运行 1: 等待停止（取消测量后进入该状态）
uint32_t stop_start_time = 0;  // 记录开始倒计时的时间（用于计算1秒延迟）
uint32_t start_count = 0;    // 新增：记录启动读数的次数
uint32_t measure_start_time = 0;  // 新增：记录本次测量的启动时间（系统绝对时间）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// ��ӣ��ض���printf������
int fputc(int ch, FILE *f) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}


// 按键检测函数
void Key_Detect(void) {
  uint8_t current_state = HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_PIN);
  uint32_t current_time = HAL_GetTick();
  
  // 按键消抖：状态稳定debounce_delay时间
  if(current_state != key_prev_state && 
     (current_time - last_key_time) > debounce_delay) {
    key_prev_state = current_state;
    
//    // 检测到按键按下
//    if(current_state == KEY_PRESSED) {
//      measure_state = !measure_state;  // 切换状态
//      if(measure_state) {
////        printf("startstartstartstartstartstart\r\n");
//		printf("[%lu ms] startstartstartstartstartstart\r\n", current_time);
//      } else {
////        printf("cancellcancellcancellcancellcancel\r\n");
//		  printf("[%lu ms] cancellcancellcancellcancellcancel\r\n", current_time);
//      }
//    }
		if(current_state == KEY_PRESSED) {
  if(measure_state) {
    // 从"测量中"切换到"取消测量"：不直接停止，而是标记需要延迟停止
    measure_state = 0;       // 清除测量状态
    stop_flag = 1;           // 标记进入"等待停止"状态
    stop_start_time = current_time;  // 记录开始倒计时的时间
	uint32_t measure_duration = current_time - measure_start_time-105;  // 本次测量的相对时间（ms）
//    printf("[%"PRIu32" ms] cancellcancellcancellcancellcancel\r\n", current_time);
//	printf("[%"PRIu32" ms] cancellcancellcancellcancellcancel (times of %"PRIu32"  duration of %"PRIu32" ms)\r\n",
//           current_time, start_count, measure_duration);
  } else {
    // 从"停止"切换到"测量中"：直接开始，清除停止标记
    measure_state = 1;       // 开启测量状态
    stop_flag = 0;           // 清除停止标记
	start_count++;  // 每次启动时计数加1
	measure_start_time = current_time;  // 记录本次测量的启动时间（系统绝对时间）
//  printf("[%"PRIu32" ms] startstartstartstartstartstart\r\n", current_time);
//	printf("[%"PRIu32" ms] startstartstartstartstartstart ( times of %"PRIu32" )\r\n", current_time, start_count);
  
  }
} 
		 
		 
		 
  } else if(current_state != key_prev_state) {
    last_key_time = current_time;
  }
}

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
	uint32_t adc_value;
    float voltage;
	uint32_t current_time;  // 用于记录当前时间戳（毫秒）
//	uint32_t tick_count = 0;  // ���ڼ�ʱ�ļ�����
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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
 HAL_ADC_Start(&hadc1);  // ����ADC
 
 
 // 初始化按键状态
  key_prev_state = HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_PIN);
  last_key_time = HAL_GetTick();
   printf("symtem on...\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  Key_Detect();
	  
	  
	  
//	  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
//      adc_value = HAL_ADC_GetValue(&hadc1);
//      voltage = adc_value * 3.3f / 4095.0f;
//		current_time = HAL_GetTick();  // 获取当前时间戳
////		printf("%.2fV\r\n",voltage);
////      printf("ADC Value: %d, Voltage: %.2fV\r\n", adc_value, voltage);
//		printf("[%lu ms] %.2fV\r\n", current_time, voltage);  // 带时间戳输出
//    }
	  
	  if ((measure_state || (stop_flag && (HAL_GetTick() - stop_start_time) < STOP_DELAY)) && 
  HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
  adc_value = HAL_ADC_GetValue(&hadc1);
	voltage = adc_value * 3.3f / 4095.0f;
	current_time = HAL_GetTick();
	uint32_t measure_duration = current_time - measure_start_time-1; 
  printf("[%"PRIu32" ms] %.2f\r\n", current_time, voltage);
//	printf("[%"PRIu32" ms] %"PRIu32"\r\n", current_time, adc_value);
//	printf("[%"PRIu32" ms] %"PRIu32"\r\n", measure_duration, adc_value);
}
	  
	  
	HAL_Delay(48);
	  
//	 /* 2. ÿ��1�뷢��һ��״̬��Ϣ�������߼��� */
//    tick_count++;  // ÿ��ѭ������+1
//    // HAL_Delay(500)Ϊ500ms�����2��ѭ����1��
//    if (tick_count >= 2) {  
//      printf("System running normally...\r\n");  // ��ʱ״̬��Ϣ
//      tick_count = 0;  // ���ü�����
//    }
	
	
   
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
	  __disable_irq();
  uint32_t timeout = 0;
  while (1)
  {
    timeout++;
    if (timeout > 2000000) // Լ1�볬ʱ��λ���������Ƶ������
    {
      NVIC_SystemReset(); // �����λ
    }
  }
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
