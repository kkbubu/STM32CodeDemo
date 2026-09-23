#include <stdio.h>
#include <string.h>
#include "Command.h"

static uint8_t Command_Number(const char *text, uint16_t *value)
{
	uint32_t result = 0;
	if (*text == '\0') { return 0; }
	while (*text != '\0')
	{
		if (*text < '0' || *text > '9') { return 0; }
		result = result * 10U + (uint32_t)(*text++ - '0');
		if (result > 65535U) { return 0; }
	}
	*value = (uint16_t)result;
	return 1;
}

uint8_t Command_Execute(Robot *robot, const RobotInputs *inputs, uint32_t now,
	const char *line, char *reply, size_t capacity)
{
	uint8_t accepted = 0;
	uint16_t number;
	if (capacity < 192U) { return 0; }
	if ((strncmp(line, "RUN ", 4) == 0 || strncmp(line, "GRIP ", 5) == 0 || strncmp(line, "JOG ", 4) == 0) &&
		(!inputs->permit || !inputs->estop_ok || (APP_USE_FEEDBACK && !inputs->vacuum_ok)))
	{
		snprintf(reply, capacity, "ERR PHYSICAL_INTERLOCK\r\n");
		return 0;
	}
	if (strcmp(line, "STATUS") == 0)
	{
		snprintf(reply, capacity, "STATE=%u FAULT=%u PHASE=%u MASK=%u CYCLES=%u/%u LOCK=%u CH=%u FB=%u IN=%u,%u,%u,%u\r\n",
			(unsigned)robot->state, (unsigned)robot->fault, (unsigned)robot->phase, (unsigned)robot->output,
			(unsigned)robot->cycles_done, (unsigned)robot->cycles_target, !APP_HARDWARE_CONFIRMED,
			APP_VALVE_COUNT, APP_USE_FEEDBACK, inputs->permit, inputs->estop_ok, inputs->anchors, inputs->vacuum_ok);
		return 1;
	}
	if (strcmp(line, "PING") == 0) { Robot_Heartbeat(robot, now); accepted = 1; }
	else if (strcmp(line, "ARM") == 0) { accepted = Robot_Arm(robot, inputs, now); }
	else if (strcmp(line, "STOP") == 0) { Robot_Stop(robot); accepted = 1; }
	else if (strcmp(line, "RELEASE") == 0) { accepted = Robot_Release(robot, inputs); }
	else if (strcmp(line, "GRIP FRONT IN") == 0) { accepted = Robot_Grip(robot, 1, 2, now); }
	else if (strcmp(line, "GRIP FRONT OUT") == 0) { accepted = Robot_Grip(robot, 1, 1, now); }
	else if (strcmp(line, "GRIP FRONT OFF") == 0) { accepted = Robot_Grip(robot, 1, 0, now); }
	else if (strcmp(line, "GRIP REAR IN") == 0) { accepted = Robot_Grip(robot, 0, 2, now); }
	else if (strcmp(line, "GRIP REAR OUT") == 0) { accepted = Robot_Grip(robot, 0, 1, now); }
	else if (strcmp(line, "GRIP REAR OFF") == 0) { accepted = Robot_Grip(robot, 0, 0, now); }
	else if (strncmp(line, "RUN FWD ", 8) == 0 && Command_Number(line + 8, &number))
	{
		accepted = Robot_Start(robot, DIRECTION_FWD, number, now);
	}
	else if (strncmp(line, "RUN REV ", 8) == 0 && Command_Number(line + 8, &number))
	{
		accepted = Robot_Start(robot, DIRECTION_REV, number, now);
	}
	else if (strncmp(line, "JOG ", 4) == 0 && Command_Number(line + 4, &number))
	{
		accepted = Robot_Manual(robot, number, now);
	}
	snprintf(reply, capacity, accepted ? "OK\r\n" : "ERR COMMAND_OR_INTERLOCK\r\n");
	return accepted;
}
