#include "stm32f1xx_hal.h"
#include "Serial.h"
#include "System.h"
void SysTick_Handler(void) { HAL_IncTick(); }
void USART1_IRQHandler(void) { Serial_Irq(); }
void NMI_Handler(void) { System_Fatal(); }
void HardFault_Handler(void) { System_Fatal(); }
void MemManage_Handler(void) { System_Fatal(); }
void BusFault_Handler(void) { System_Fatal(); }
void UsageFault_Handler(void) { System_Fatal(); }
