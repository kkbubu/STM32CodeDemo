#ifndef COMMAND_H
#define COMMAND_H
#include <stddef.h>
#include "Robot.h"
/* 严格解析一整行 ASCII 命令；回复缓冲区至少 192 字节，返回 1 表示命令接受。
 * 仅 ARM / PING 刷新心跳；查询和非法命令不能延长运行许可。 */
uint8_t Command_Execute(Robot *robot, const RobotInputs *inputs, uint32_t now,
	const char *line, char *reply, size_t capacity);
#endif
