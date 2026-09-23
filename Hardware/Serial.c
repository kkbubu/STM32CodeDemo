#include <string.h>
#include "stm32f1xx_hal.h"
#include "Serial.h"

static UART_HandleTypeDef Serial_Uart;
static uint8_t Serial_RxByte;
static volatile uint8_t Serial_Head, Serial_Tail, Serial_Error;
static uint8_t Serial_Buffer[128];
static char Serial_Line[64];
static uint8_t Serial_Length, Serial_Drop;

uint8_t Serial_Init(void)
{
	GPIO_InitTypeDef gpio = {0};
	__HAL_RCC_USART1_CLK_ENABLE();
	gpio.Pin = GPIO_PIN_9;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio);
	gpio.Pin = GPIO_PIN_10;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &gpio);
	Serial_Uart.Instance = USART1;
	Serial_Uart.Init.BaudRate = 115200;
	Serial_Uart.Init.WordLength = UART_WORDLENGTH_8B;
	Serial_Uart.Init.StopBits = UART_STOPBITS_1;
	Serial_Uart.Init.Parity = UART_PARITY_NONE;
	Serial_Uart.Init.Mode = UART_MODE_TX_RX;
	Serial_Uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	Serial_Uart.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&Serial_Uart) != HAL_OK) { return 0; }
	HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	return HAL_UART_Receive_IT(&Serial_Uart, &Serial_RxByte, 1) == HAL_OK;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
	uint8_t next = (Serial_Head + 1U) & 127U;
	if (uart != &Serial_Uart) { return; }
	if (next == Serial_Tail) { Serial_Error = 1; }
	else { Serial_Buffer[Serial_Head] = Serial_RxByte; Serial_Head = next; }
	if (HAL_UART_Receive_IT(uart, &Serial_RxByte, 1) != HAL_OK) { Serial_Error = 1; }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
	Serial_Error = 1;
	/* 错误后重置接收器；不自动恢复机器人步态。 */
	if (HAL_UART_AbortReceive(uart) != HAL_OK) { return; }
	if (HAL_UART_Receive_IT(uart, &Serial_RxByte, 1) != HAL_OK) { Serial_Error = 1; }
}

uint8_t Serial_ReadLine(char *line)
{
	uint8_t count, byte;
	for (count = 0; count < 32 && Serial_Tail != Serial_Head; count++)
	{
		byte = Serial_Buffer[Serial_Tail];
		Serial_Tail = (Serial_Tail + 1U) & 127U;
		if (byte == '\r' || byte == '\n')
		{
			if (Serial_Drop) { Serial_Drop = 0; Serial_Length = 0; continue; }
			if (Serial_Length == 0) { continue; }
			Serial_Line[Serial_Length] = '\0';
			memcpy(line, Serial_Line, Serial_Length + 1U);
			Serial_Length = 0;
			return 1;
		}
		if (byte < 32 || byte > 126 || Serial_Length >= sizeof(Serial_Line) - 1U)
		{
			Serial_Drop = 1;
			Serial_Error = 1;
		}
		if (!Serial_Drop) { Serial_Line[Serial_Length++] = (char)byte; }
	}
	return 0;
}

uint8_t Serial_TakeError(void)
{
	uint8_t error;
	uint32_t irq = __get_PRIMASK();
	__disable_irq();
	error = Serial_Error;
	Serial_Error = 0;
	if (error)
	{
		Serial_Tail = Serial_Head;
		Serial_Length = 0;
		Serial_Drop = 1; /* 丢弃直到下一行结束，禁止截断命令执行。 */
	}
	__set_PRIMASK(irq);
	return error;
}

uint8_t Serial_Send(const char *text)
{
	return HAL_UART_Transmit(&Serial_Uart, (uint8_t *)text, (uint16_t)strlen(text), 30) == HAL_OK;
}

void Serial_Irq(void) { HAL_UART_IRQHandler(&Serial_Uart); }
