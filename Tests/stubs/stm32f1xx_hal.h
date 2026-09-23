#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
/* 仅用于宿主机验证 GPIO 映射；不链接到真实固件。 */
typedef struct { uint32_t IDR, BSRR; } GPIO_TypeDef;
extern GPIO_TypeDef Test_A, Test_B, Test_C;
#define GPIOA (&Test_A)
#define GPIOB (&Test_B)
#define GPIOC (&Test_C)
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
#define GPIO_PIN_0 1U
#define GPIO_PIN_1 2U
#define GPIO_PIN_2 4U
#define GPIO_PIN_3 8U
#define GPIO_PIN_5 32U
#define GPIO_PIN_6 64U
#define GPIO_PIN_7 128U
#define GPIO_PIN_8 256U
#define GPIO_PIN_9 512U
#define GPIO_PIN_10 1024U
#define GPIO_PIN_11 2048U
#define GPIO_PIN_12 4096U
#define GPIO_PIN_13 8192U
#define GPIO_PIN_14 16384U
#define GPIO_MODE_OUTPUT_PP 1U
#define GPIO_MODE_INPUT 2U
#define GPIO_SPEED_FREQ_LOW 0U
#define GPIO_PULLUP 1U
#define GPIO_NOPULL 0U
#define __HAL_RCC_AFIO_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pins, GPIO_PinState state);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
#endif
