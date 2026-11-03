
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32 UART Morse Code Player
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUF_SIZE 128
#define LINE_BUF_SIZE 32
#define MORSE_UNIT 200
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint8_t rx_byte;

volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

char line_buf[LINE_BUF_SIZE] = {0};
uint8_t line_index = 0;

/* 記住上一個播放的英文 */
char last_word[LINE_BUF_SIZE] = {0};

/* PC13 重播旗標 */
volatile uint8_t replay_flag = 0;
volatile uint32_t last_button_tick = 0;

/* A-Z 摩斯碼表 */
const char *morse_table[26] =
{
    ".-", "-...", "-.-.", "-..", ".", "..-.",
    "--.", "....", "..", ".---", "-.-", ".-..",
    "--", "-.", "---", ".--.", "--.-", ".-.",
    "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--.."
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
void UART_SendString(const char *str);
uint8_t UART_ReadByte(uint8_t *data);
void Morse_PlaySymbol(char symbol);
void Morse_PlayWord(const char *word);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* UART 傳送字串 */
void UART_SendString(const char *str)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)str,
        (uint16_t)strlen(str),
        HAL_MAX_DELAY
    );
}

/* Ring Buffer 讀取 */
uint8_t UART_ReadByte(uint8_t *data)
{
    if (rx_head == rx_tail)
    {
        return 0;
    }

    *data = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) % RX_BUF_SIZE;

    return 1;
}

/* UART2 接收中斷 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uint16_t next_head;

        next_head = (rx_head + 1U) % RX_BUF_SIZE;

        if (next_head != rx_tail)
        {
            rx_buf[rx_head] = rx_byte;
            rx_head = next_head;
        }

        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

/* 播放一個點或劃 */
void Morse_PlaySymbol(char symbol)
{
    if (symbol == '.')
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(MORSE_UNIT);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    }
    else if (symbol == '-')
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(3 * MORSE_UNIT);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    }
}

/* 播放完整英文 */
void Morse_PlayWord(const char *word)
{
    uint16_t i;
    uint16_t j;
    char c;
    const char *code;

    for (i = 0; word[i] != '\0'; i++)
    {
        c = word[i];

        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }

        if (c < 'A' || c > 'Z')
        {
            continue;
        }

        code = morse_table[c - 'A'];

        for (j = 0; code[j] != '\0'; j++)
        {
            Morse_PlaySymbol(code[j]);

            if (code[j + 1] != '\0')
            {
                HAL_Delay(MORSE_UNIT);
            }
        }

        if (word[i + 1] != '\0')
        {
            HAL_Delay(3 * MORSE_UNIT);
        }
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
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

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */

    /* 啟動 UART 中斷接收 */
    if (HAL_UART_Receive_IT(&huart2, &rx_byte, 1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        uint8_t data;

        /* 處理 UART 接收 */
        if (UART_ReadByte(&data))
        {
            if (data == '\r')
            {
                continue;
            }

            if (data == '\n')
            {
                line_buf[line_index] = '\0';

                if (line_index > 0)
                {
                    UART_SendString("RX: ");
                    UART_SendString(line_buf);
                    UART_SendString("\r\n");

                    /* 記住這次輸入的英文 */
                    strcpy(last_word, line_buf);

                    UART_SendString("PLAYING\r\n");

                    Morse_PlayWord(last_word);

                    UART_SendString("DONE\r\n");
                }

                line_index = 0;
                memset(line_buf, 0, sizeof(line_buf));
            }
            else
            {
                if (line_index < LINE_BUF_SIZE - 1)
                {
                    line_buf[line_index] = (char)data;
                    line_index++;
                }
            }
        }

        /* PC13 按鈕重播 */
        if (replay_flag)
        {
            replay_flag = 0;

            if (last_word[0] != '\0')
            {
                UART_SendString("PLAYING\r\n");

                Morse_PlayWord(last_word);

                UART_SendString("DONE\r\n");
            }
        }

    } /* 結束 while(1) */

    /* USER CODE END 3 */

} /* 結束 main() */

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(
        PWR_REGULATOR_VOLTAGE_SCALE1_BOOST
    );

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
        &RCC_ClkInitStruct,
        FLASH_LATENCY_4
    ) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
    /* USER CODE BEGIN USART2_Init 0 */
    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */
    /* USER CODE END USART2_Init 1 */

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit =
        UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(
        &huart2,
        UART_TXFIFO_THRESHOLD_1_8
    ) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(
        &huart2,
        UART_RXFIFO_THRESHOLD_1_8
    ) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN USART2_Init 2 */
    /* USER CODE END USART2_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA5 LED 預設熄滅 */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_5,
        GPIO_PIN_RESET
    );

    /* PC13 B1 按鈕 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PA5 LED */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 啟用 PC13 中斷 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* PC13 按鈕中斷 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        uint32_t now = HAL_GetTick();

        /* 200 ms 防彈跳 */
        if ((now - last_button_tick) >= 200U)
        {
            replay_flag = 1;
            last_button_tick = now;
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief Error Handler
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif
