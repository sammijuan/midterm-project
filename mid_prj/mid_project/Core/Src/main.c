
#include "main.h"
#include <stdint.h>
#include <string.h>

/* ==============================
   基本設定
   ============================== */

#define RX_BUF_SIZE    128
#define LINE_BUF_SIZE  32
#define MORSE_UNIT     200

/* UART2 Handle */
UART_HandleTypeDef huart2;

/* UART Interrupt 每次接收 1 byte */
uint8_t rx_byte;

/* Ring Buffer */
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

/* 英文字串緩衝區 */
char line_buf[LINE_BUF_SIZE];
uint8_t line_index = 0;

/* Morse Code A ~ Z */
const char *morse_table[26] =
{
    ".-",     /* A */
    "-...",   /* B */
    "-.-.",   /* C */
    "-..",    /* D */
    ".",      /* E */
    "..-.",   /* F */
    "--.",    /* G */
    "....",   /* H */
    "..",     /* I */
    ".---",   /* J */
    "-.-",    /* K */
    ".-..",   /* L */
    "--",     /* M */
    "-.",     /* N */
    "---",    /* O */
    ".--.",   /* P */
    "--.-",   /* Q */
    ".-.",    /* R */
    "...",    /* S */
    "-",      /* T */
    "..-",    /* U */
    "...-",   /* V */
    ".--",    /* W */
    "-..-",   /* X */
    "-.--",   /* Y */
    "--.."    /* Z */
};

/* 函式宣告 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

uint8_t UART_ReadByte(uint8_t *data);
void Morse_PlaySymbol(char symbol);
void Morse_PlayWord(const char *word);
void Error_Handler(void);

/* ==============================
   UART 傳送字串
   ============================== */

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)str,
        (uint16_t)strlen(str),
        HAL_MAX_DELAY
    );
}

/* ==============================
   從 Ring Buffer 讀取 1 byte
   ============================== */

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

/* ==============================
   UART 接收完成 Callback
   只負責將 byte 放入 Ring Buffer
   ============================== */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uint16_t next_head;

        next_head = (rx_head + 1U) % RX_BUF_SIZE;

        /* Buffer 未滿才放入資料 */
        if (next_head != rx_tail)
        {
            rx_buf[rx_head] = rx_byte;
            rx_head = next_head;
        }

        /* 再次啟動 UART Interrupt */
        HAL_UART_Receive_IT(
            &huart2,
            &rx_byte,
            1
        );
    }
}

/* ==============================
   播放一個摩斯碼符號
   ============================== */

void Morse_PlaySymbol(char symbol)
{
    if (symbol == '.')
    {
        /* 點：亮 200 ms */
        HAL_GPIO_WritePin(
            GPIOA,
            GPIO_PIN_5,
            GPIO_PIN_SET
        );

        HAL_Delay(MORSE_UNIT);

        HAL_GPIO_WritePin(
            GPIOA,
            GPIO_PIN_5,
            GPIO_PIN_RESET
        );
    }
    else if (symbol == '-')
    {
        /* 劃：亮 600 ms */
        HAL_GPIO_WritePin(
            GPIOA,
            GPIO_PIN_5,
            GPIO_PIN_SET
        );

        HAL_Delay(3 * MORSE_UNIT);

        HAL_GPIO_WritePin(
            GPIOA,
            GPIO_PIN_5,
            GPIO_PIN_RESET
        );
    }
}

/* ==============================
   播放完整英文單字
   ============================== */

void Morse_PlayWord(const char *word)
{
    uint16_t i;
    uint16_t j;
    char c;
    const char *code;

    for (i = 0; word[i] != '\0'; i++)
    {
        c = word[i];

        /* 小寫轉大寫 */
        if (c >= 'a' && c <= 'z')
        {
            c = c - 'a' + 'A';
        }

        /* 只處理 A ~ Z */
        if (c < 'A' || c > 'Z')
        {
            continue;
        }

        code = morse_table[c - 'A'];

        /* 播放該字母的點與劃 */
        for (j = 0; code[j] != '\0'; j++)
        {
            Morse_PlaySymbol(code[j]);

            /* 同一字母內，符號間隔 200 ms */
            if (code[j + 1] != '\0')
            {
                HAL_Delay(MORSE_UNIT);
            }
        }

        /* 不同字母間隔 600 ms */
        if (word[i + 1] != '\0')
        {
            HAL_Delay(3 * MORSE_UNIT);
        }
    }

    /* 確保 LED 最後熄滅 */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_5,
        GPIO_PIN_RESET
    );
}

/* ==============================
   main()
   ============================== */

int main(void)
{
    uint8_t data;

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    /* 初始化 Ring Buffer */
    rx_head = 0;
    rx_tail = 0;
    line_index = 0;

    memset(line_buf, 0, sizeof(line_buf));

    /* 開始 UART Interrupt 接收 */
    if (HAL_UART_Receive_IT(
            &huart2,
            &rx_byte,
            1
        ) != HAL_OK)
    {
        Error_Handler();
    }

    /* 主迴圈 */
    while (1)
    {
        /* 從 Ring Buffer 逐 byte 讀取 */
        if (UART_ReadByte(&data))
        {
            /* 忽略 CR */
            if (data == '\r')
            {
                continue;
            }

            /* 收到換行，代表完整文字 */
            if (data == '\n')
            {
                line_buf[line_index] = '\0';

                if (line_index > 0)
                {
                    /* 回傳收到的文字 */
                    UART_SendString("RX: ");
                    UART_SendString(line_buf);
                    UART_SendString("\r\n");

                    /* 開始播放 */
                    UART_SendString("PLAYING\r\n");

                    Morse_PlayWord(line_buf);

                    /* 播放完成 */
                    UART_SendString("DONE\r\n");
                }

                /* 清空文字緩衝區 */
                line_index = 0;
                memset(line_buf, 0, sizeof(line_buf));
            }
            else
            {
                /* 尚未收到換行，繼續存字元 */
                if (line_index < LINE_BUF_SIZE - 1)
                {
                    line_buf[line_index] = (char)data;
                    line_index++;
                }
            }
        }
    }
}

/* ==============================
   System Clock
   使用 HSI 16 MHz
   ============================== */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState = RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;

    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct
        ) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0
        ) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==============================
   USART2 初始化
   PA2 TX / PA3 RX
   115200 8N1
   ============================== */

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;

    huart2.Init.BaudRate = 115200;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;

    huart2.Init.OneBitSampling =
        UART_ONE_BIT_SAMPLE_DISABLE;

    huart2.Init.ClockPrescaler =
        UART_PRESCALER_DIV1;

    huart2.AdvancedInit.AdvFeatureInit =
        UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==============================
   GPIO 初始化
   PA5 = LD2
   ============================== */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 開啟 GPIOA Clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED 初始熄滅 */
    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_5,
        GPIO_PIN_RESET
    );

    /* PA5 設定為 Output */
    GPIO_InitStruct.Pin = GPIO_PIN_5;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );
}

/* ==============================
   Error Handler
   ============================== */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
