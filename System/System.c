#include "stm32f1xx_hal.h"
#include "System.h"
#ifdef __CC_ARM
/* 链接期禁止半主机调用，确保固件不依赖连接中的调试器。 */
#pragma import(__use_no_semihosting)
#endif
static IWDG_HandleTypeDef System_Watchdog;

uint8_t System_InitClock(void)
{
	RCC_OscInitTypeDef oscillator = {0};
	RCC_ClkInitTypeDef clock = {0};
	/* HSI/2 × 16 = 64 MHz，不依赖开发板外部晶振。 */
	oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	oscillator.HSIState = RCC_HSI_ON;
	oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	oscillator.PLL.PLLState = RCC_PLL_ON;
	oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	oscillator.PLL.PLLMUL = RCC_PLL_MUL16;
	if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) { return 0; }
	clock.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clock.APB1CLKDivider = RCC_HCLK_DIV2;
	clock.APB2CLKDivider = RCC_HCLK_DIV1;
	return HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) == HAL_OK;
}

uint8_t System_InitWatchdog(void)
{
	System_Watchdog.Instance = IWDG;
	System_Watchdog.Init.Prescaler = IWDG_PRESCALER_64;
	System_Watchdog.Init.Reload = 1249; /* LSI 40 kHz 标称约 2 秒，实测受 LSI 公差影响。 */
	return HAL_IWDG_Init(&System_Watchdog) == HAL_OK;
}

void System_FeedWatchdog(void)
{
	if (HAL_IWDG_Refresh(&System_Watchdog) != HAL_OK) { System_Fatal(); }
}

void System_Fatal(void)
{
	__disable_irq();
	while (1) { /* 看门狗已启动时触发复位；未启动时等待人工复位。 */ }
}
