#include <stdio.h>
#include "Board.h"
GPIO_TypeDef Test_A, Test_B, Test_C;
static unsigned checks, init_count, output_preloaded;
static uint32_t startup_bsrr;
#define CHECK(x) do { checks++; if (!(x)) { printf("FAIL board line %d: %s\n", __LINE__, #x); return 1; } } while (0)

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state)
{
	port->BSRR = state ? pins : (uint32_t)pins << 16;
	if (port == GPIOB) { startup_bsrr = port->BSRR; output_preloaded = 1; }
}
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio)
{
	if (port == GPIOB && gpio->Mode == GPIO_MODE_OUTPUT_PP && !output_preloaded) { output_preloaded = 99; }
	init_count++;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
	return (port->IDR & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

int main(void)
{
	unsigned mask, channel;
	uint32_t expected, all = APP_VALVE_COUNT == 7 ? 0x03E3U : 0x3FE3U;
	const uint8_t pin_numbers[] = {0, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13};
	RobotInputs inputs;
	Board_Init();
	CHECK(output_preloaded == 1 && init_count == 4);
	CHECK(startup_bsrr == (APP_MOS_ACTIVE_HIGH ? all << 16 : all));
	for (mask = 0; mask < (1U << APP_VALVE_COUNT); mask++)
	{
		Test_B.BSRR = 0xDEADBEEFUL;
		if (!Pneumatic_IsValid((uint16_t)mask))
		{
			CHECK(!Board_WriteValves((uint16_t)mask));
			CHECK(Test_B.BSRR == 0xDEADBEEFUL);
			continue;
		}
		expected = 0;
		for (channel = 0; channel < APP_VALVE_COUNT; channel++)
		{
			if (mask & (1U << channel)) { expected |= 1U << pin_numbers[channel]; }
		}
		if (!APP_MOS_ACTIVE_HIGH) { expected = all & ~expected; }
		expected |= (all & ~expected) << 16;
		CHECK(Board_WriteValves((uint16_t)mask));
		CHECK(Test_B.BSRR == expected);
	}
	Test_A.IDR = 0xFFFF; Test_B.IDR = 0xFFFF;
	Board_ReadInputs(&inputs);
	CHECK(!inputs.permit && !inputs.estop_ok && !inputs.anchors && !inputs.vacuum_ok);
	Test_A.IDR = 0; Test_B.IDR = 0;
	Board_ReadInputs(&inputs);
	CHECK(inputs.permit && inputs.estop_ok && inputs.anchors == 7 && inputs.vacuum_ok);
	printf("PASS GPIO channels=%d active_high=%d checks=%u\n", APP_VALVE_COUNT, APP_MOS_ACTIVE_HIGH, checks);
	return 0;
}
