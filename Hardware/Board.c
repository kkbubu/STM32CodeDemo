#include "Board.h"
static const uint16_t Board_ValvePins[11] =
{
	GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_8,
	GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13
};
static uint16_t Board_OutputPins;

void Board_Init(void)
{
	GPIO_InitTypeDef gpio = {0};
	uint8_t channel;
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	for (channel = 0; channel < APP_VALVE_COUNT; channel++) { Board_OutputPins |= Board_ValvePins[channel]; }
	HAL_GPIO_WritePin(GPIOB, Board_OutputPins, APP_MOS_ACTIVE_HIGH ? GPIO_PIN_RESET : GPIO_PIN_SET);
	gpio.Pin = Board_OutputPins;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);
	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &gpio);
	gpio.Pin = GPIO_PIN_14;
	HAL_GPIO_Init(GPIOB, &gpio);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	gpio.Pin = GPIO_PIN_13;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &gpio);
}

void Board_ReadInputs(RobotInputs *inputs)
{
	inputs->permit = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET;
	inputs->estop_ok = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET;
	inputs->anchors = (uint8_t)((~GPIOA->IDR) & 7U);
	inputs->vacuum_ok = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET;
}

uint8_t Board_WriteValves(uint16_t mask)
{
	uint16_t high = 0;
	uint8_t channel;
	if (!Pneumatic_IsValid(mask)) { return 0; }
	for (channel = 0; channel < APP_VALVE_COUNT; channel++)
	{
		if ((mask & (1U << channel)) != 0) { high |= Board_ValvePins[channel]; }
	}
	if (!APP_MOS_ACTIVE_HIGH) { high = Board_OutputPins & ~high; }
	/* HAL 提供 GPIO 初始化；这里用单次 BSRR 写同时置位/复位，避免分次写的中间态。 */
	GPIOB->BSRR = (uint32_t)high | ((uint32_t)(Board_OutputPins & ~high) << 16);
	return 1;
}

void Board_SetLed(uint8_t on)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
