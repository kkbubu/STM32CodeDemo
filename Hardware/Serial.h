#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
/* USART1 PA9/PA10，115200 8N1，3.3V TTL；初始化失败返回 0。 */
uint8_t Serial_Init(void);
/* 每次最多处理 32 字节；完整行返回 1。溢出、非 ASCII、超长行锁存错误。 */
uint8_t Serial_ReadLine(char *line);
uint8_t Serial_TakeError(void);
/* 最多阻塞 30 ms；失败返回 0。 */
uint8_t Serial_Send(const char *text);
void Serial_Irq(void);
#endif
