#include "Board.h"
#include "Serial.h"
#include "System.h"
#include "Command.h"
#include <string.h>

int main(void)
{
	Robot robot;
	RobotInputs inputs;
	uint32_t now;
	char line[64];
	char reply[192];
	uint8_t has_line;
	uint16_t applied_output = 0;
	/* 先将执行器置为关闭，随后准备时钟、通信与应用。 */
	if (HAL_Init() != HAL_OK) { System_Fatal(); }
	Board_Init();
	if (!System_InitClock() || !Serial_Init() || !System_InitWatchdog()) { System_Fatal(); }
	Robot_Init(&robot, HAL_GetTick());
	if (!Serial_Send("WORM HAL READY; STATUS FOR CONFIG; RELEASE VENTS\r\n")) { Robot_Fail(&robot, FAULT_IO); }
	while (1)
	{
		now = HAL_GetTick();
		Board_ReadInputs(&inputs);
		Robot_Tick(&robot, &inputs, now);
		has_line = Serial_ReadLine(line);
		if (Serial_TakeError()) { Robot_Fail(&robot, FAULT_IO); has_line = 0; }
		if (has_line)
		{
			(void)Command_Execute(&robot, &inputs, now, line, reply, sizeof(reply));
			if (strcmp(line, "STOP") == 0) { robot.output = applied_output; }
			/* 指令后、写输出前再次检查许可与停止回路。 */
			Board_ReadInputs(&inputs);
			Robot_Tick(&robot, &inputs, HAL_GetTick());
		}
		/* 写出前发现故障时，冻结上次实际输出，不能提交尚未执行的新动作。 */
		if (robot.state == ROBOT_FAULT) { robot.output = applied_output; }
		if (!Board_WriteValves(robot.output)) { Robot_Fail(&robot, FAULT_IO); }
		else { applied_output = robot.output; }
		if (has_line && !Serial_Send(reply)) { Robot_Fail(&robot, FAULT_IO); }
		Board_SetLed(robot.state == ROBOT_RUNNING ? 1 : ((HAL_GetTick() / 500U) & 1U));
		System_FeedWatchdog();
		__WFI();
	}
}
