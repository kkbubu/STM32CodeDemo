#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdint.h>
uint8_t System_InitClock(void);
uint8_t System_InitWatchdog(void);
void System_FeedWatchdog(void);
/* 致命错误保持寄存器输出，停止喂狗；复位后所有输出关闭，须机械防坠。 */
void System_Fatal(void);
#endif
