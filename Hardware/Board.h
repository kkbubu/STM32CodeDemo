#ifndef BOARD_H
#define BOARD_H
#include "Robot.h"
#include "stm32f1xx_hal.h"
/* GPIO 输出先预装关闭电平，再切换推挽；所有阀使用 GPIOB 单端口。 */
void Board_Init(void);
void Board_ReadInputs(RobotInputs *inputs);
/* 校验通道范围和抓手互斥，非法输入保持原输出并返回 0。 */
uint8_t Board_WriteValves(uint16_t mask);
void Board_SetLed(uint8_t on);
#endif
