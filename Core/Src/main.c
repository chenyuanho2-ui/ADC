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
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BASE_SAMPLE_RATE 1000  // 基准采样率 1kHz (1ms)
#define RX_BUFFER_SIZE 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 控制状态变量
uint8_t is_running = 1;        // 默认开机直接运行
uint32_t target_print_freq = 4; // 默认打印频率 1kHz (即不进行平均)
uint32_t samples_per_print = 249;    // 计算出的累加次数：1000 / target_print_freq

// ADC 数据处理变量
uint32_t adc_accumulator = 0;  // 累加器
uint32_t sample_counter = 0;   // 当前已采集样本数

// 计时变量
uint32_t last_sample_tick = 0;
uint32_t last_blink_tick = 0;
uint32_t last_key_tick = 0;

// 串口接收变量
uint8_t rx_byte;               // 接收单字节缓冲
char rx_buffer[RX_BUFFER_SIZE]; // 命令字符串缓冲
uint8_t rx_index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 重定向 printf 到串口
int fputc(int ch, FILE *f) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10);
  return ch;
}

// 处理接收到的命令字符串
void Process_Command(char *cmd) {
    if (cmd[0] == 'c') {
        // 切换启停
        is_running = !is_running;
        printf("System %s\r\n", is_running ? "Resumed" : "Stopped");
    } 
    else if (cmd[0] == 't') {
        // 解析频率 t1000 或 t1
        int freq = atoi(&cmd[1]);
        if (freq > 0 && freq <= 1000) {
            target_print_freq = freq;
            samples_per_print = BASE_SAMPLE_RATE / target_print_freq;
            if (samples_per_print == 0) samples_per_print = 1;
            
            // 重置计数器防止逻辑错误
            sample_counter = 0;
            adc_accumulator = 0;
            printf("Print Freq set to: %d Hz (Avg %lu samples)\r\n", freq, samples_per_print);
        } else {
            printf("Error: Freq must be 1-1000\r\n");
        }
    }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 串口接收中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 如果收到换行符或缓冲区满，处理命令
        if (rx_byte == '\n' || rx_byte == '\r' || rx_index >= RX_BUFFER_SIZE - 1) {
            rx_buffer[rx_index] = '\0'; // 字符串结束符
            if (rx_index > 0) {
                Process_Command(rx_buffer);
            }
            rx_index = 0; // 重置缓冲区
        } else {
            rx_buffer[rx_index++] = rx_byte; // 存入缓冲
        }
        // 重新开启接收
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1); // ADC 校准，提高精度
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1); // 开启串口接收中断
  
  printf("System Ready. Commands: 'c' to toggle, 't1000' to set freq.\r\n");
  last_sample_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  uint32_t current_tick = HAL_GetTick();

    // --- 1. 按键检测 (PA3 上拉，按下为低电平) ---
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
        if (current_tick - last_key_tick > 200) { // 200ms 消抖
            is_running = !is_running;
            printf("Key Pressed: System %s\r\n", is_running ? "Resumed" : "Stopped");
            last_key_tick = current_tick;
        }
    } else {
        // 如果按键抬起，稍微更新时间，避免长按松开后的误触（可选）
    }

    // --- 2. 采样任务 (1kHz = 1ms) ---
    if (is_running) {
        // 检查是否经过了 1ms
        if (current_tick - last_sample_tick >= 1) {
            last_sample_tick = current_tick;

            // 启动 ADC 转换
            HAL_ADC_Start(&hadc1);
            if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
                uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
                adc_accumulator += adc_val;
                sample_counter++;
            }

            // 检查是否达到打印/平均条件
            if (sample_counter >= samples_per_print) {
                // 计算平均值 (防止浮点运算过于频繁，先用整型，打印时转浮点)
                float voltage = (float)(adc_accumulator / sample_counter) * 3.3f / 4095.0f;
                
                // 打印数据
                // 注意：如果频率是 1000Hz，波特率 115200 可能跟不上打印速度，建议只在低频时打印详细信息
                if (target_print_freq > 500) {
                     // 高频模式简化输出，避免串口阻塞
                     printf("%.2f\n", voltage); 
                } else {
                     printf("[%lu ms] Volt: %.3f V\r\n", current_tick, voltage);
                }

                // 清空计数器
                adc_accumulator = 0;
                sample_counter = 0;
            }
        }

        // --- 3. LED 闪烁 (PC13) ---
        // 运行状态下，每 200ms 翻转一次 (2.5Hz 闪烁)
        if (current_tick - last_blink_tick > 200) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            last_blink_tick = current_tick;
        }
    } else {
        // 停止状态下，可以强制关闭 LED 或常亮，这里选择关闭 (PC13 通常低电平亮，高电平灭)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
        // 保持采样时间同步，避免恢复时瞬间执行多次
        last_sample_tick = current_tick;
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
