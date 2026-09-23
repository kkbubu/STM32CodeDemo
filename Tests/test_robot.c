#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Command.h"

static unsigned checks;
#define CHECK(expression) do { checks++; if (!(expression)) { printf("FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (0)
static RobotInputs inputs = {1, 1, 7, 1};

static void advance(Robot *robot, uint32_t *now, uint32_t duration, uint8_t heartbeat)
{
	uint32_t offset;
	for (offset = 0; offset < duration; offset += 10)
	{
		*now += 10;
		if (heartbeat) { Robot_Heartbeat(robot, *now); }
		Robot_Tick(robot, &inputs, *now);
	}
}

static void prepare(Robot *robot, uint32_t now)
{
	inputs.permit = 1; inputs.estop_ok = 1; inputs.anchors = 7; inputs.vacuum_ok = 1;
	Robot_Init(robot, now);
	(void)Robot_Arm(robot, &inputs, now);
}

int main(void)
{
	Robot robot;
	uint32_t now = 0;
	uint16_t saved;
	uint8_t direction, phase_seen[14], previous_phase;
	unsigned mask;
	int position[3], candidate[3], rear_length, front_length, fixed, anchor_index;
	char reply[192];
	const char *invalid[] = {"RUN FWD 0", "RUN FWD -1", "RUN FWD +1", "RUN FWD 65536", "RUN FWD 99999999999999",
		"RUN FWD 1x", "RUN FWD 1 ", "RUN UP 1", "JOG 3", "JOG 96", "JOG 65535", "RUN REV", "ARM extra", "run fwd 1", ""};
	prepare(&robot, 0);
	CHECK(robot.output == 0);
	CHECK(Pneumatic_AnchorBits(7, DIRECTION_FWD) == 0x029U);
	CHECK(Pneumatic_AnchorBits(7, DIRECTION_REV) == 0x029U);
	CHECK(Pneumatic_AllSegments() == (APP_VALVE_COUNT == 7 ? 0x014U : 0x794U));
	for (mask = 0; mask <= 0xFFFFU; mask++)
	{
		uint8_t valid = mask < (1U << APP_VALVE_COUNT) && (mask & 3U) != 3U && (mask & 96U) != 96U;
		CHECK(Pneumatic_IsValid((uint16_t)mask) == valid);
	}
#if !APP_HARDWARE_CONFIRMED
	CHECK(robot.state == ROBOT_IDLE);
	CHECK(!Robot_Start(&robot, DIRECTION_FWD, 1, 0));
	CHECK(!Robot_Manual(&robot, 1, 0));
	CHECK(!Command_Execute(&robot, &inputs, 0, "ARM", reply, sizeof(reply)));
	CHECK(robot.output == 0);
#else
	CHECK(robot.state == ROBOT_ARMED);
	for (mask = 0; mask < sizeof(invalid) / sizeof(invalid[0]); mask++)
	{
		CHECK(!Command_Execute(&robot, &inputs, 0, invalid[mask], reply, sizeof(reply)));
		CHECK(robot.state == ROBOT_ARMED && robot.output == 0);
	}
	CHECK(!Robot_Start(&robot, (RobotDirection)2, 1, 0));
	CHECK(!Robot_Start(&robot, DIRECTION_FWD, 1001, 0));
	/* 双方向、三周期：连续运行、跨 uint32_t 回绕和双向抓手互斥。 */
	for (direction = 0; direction < 2; direction++)
	{
		now = 0xFFFFF000UL;
		prepare(&robot, now);
		memset(phase_seen, 0, sizeof(phase_seen));
		position[0] = 0; position[1] = 2; position[2] = 4;
		CHECK(Robot_Start(&robot, (RobotDirection)direction, 3, now));
		while (robot.state == ROBOT_RUNNING)
		{
			phase_seen[robot.phase] = 1;
			CHECK(Pneumatic_IsValid(robot.output));
			CHECK((robot.output & 0x042U) == 0); /* 内抓回路不参与爬行。 */
			CHECK((robot.output & 0x029U) != 0);
			previous_phase = robot.phase;
			saved = robot.output;
			advance(&robot, &now, 10, 1);
			if (robot.phase != previous_phase)
			{
				/* 阀切换前后至少保留一个相同的锚定回路。 */
				CHECK((saved & robot.output & 0x029U) != 0);
				/* 独立等行程理想模型：锚定点不移动，两段长度由 1/2 表示缩/伸。
				 * 如在两端都固定时单独改变它们之间的长度，此约束必失败。 */
				rear_length = (robot.output & 4U) ? 1 : 2;
				front_length = (robot.output & 16U) ? 1 : 2;
				fixed = (robot.output & 1U) ? 0 : ((robot.output & 8U) ? 1 : 2);
				candidate[0] = position[fixed] - (fixed >= 1 ? rear_length : 0) - (fixed == 2 ? front_length : 0);
				candidate[1] = candidate[0] + rear_length;
				candidate[2] = candidate[1] + front_length;
				for (anchor_index = 0; anchor_index < 3; anchor_index++)
				{
					unsigned bit = anchor_index == 0 ? 1U : (anchor_index == 1 ? 8U : 32U);
					if (robot.output & bit) { CHECK(candidate[anchor_index] == position[anchor_index]); }
					position[anchor_index] = candidate[anchor_index];
				}
			}
		}
		CHECK(robot.state == ROBOT_HOLD && robot.cycles_done == 3);
		CHECK(robot.output == (0x029U | Pneumatic_AllSegments()));
		CHECK(position[0] == (direction == 0 ? 4 : -2));
		CHECK(position[1] == (direction == 0 ? 5 : -1));
		CHECK(position[2] == (direction == 0 ? 6 : 0));
		for (mask = 0; mask < 14; mask++) { CHECK(phase_seen[mask]); }
		CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
		Robot_Stop(&robot);
		CHECK(!Robot_Release(&robot, &inputs));
		inputs.permit = 0;
		CHECK(Robot_Release(&robot, &inputs) && robot.output == 0);
	}
	/* 每一相位 STOP 和故障均保持输出，不能悄悄释放支撑。 */
	for (mask = 0; mask < 14; mask++)
	{
		now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
		while (robot.phase != mask && robot.state == ROBOT_RUNNING) { advance(&robot, &now, 10, 1); }
		CHECK(robot.phase == mask);
		saved = robot.output; Robot_Stop(&robot); advance(&robot, &now, 10000, 0);
		CHECK(robot.state == ROBOT_HOLD && robot.output == saved);
	}
	now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
	saved = robot.output; advance(&robot, &now, APP_LINK_TIMEOUT_MS, 0);
	CHECK(robot.state == ROBOT_FAULT && robot.fault == FAULT_LINK);
	/* 最后一相位输出不一定等于开头；故障后必须保持。 */
	saved = robot.output; Robot_Heartbeat(&robot, now); advance(&robot, &now, 500, 1);
	CHECK(robot.output == saved && robot.state == ROBOT_FAULT);
	now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
	inputs.estop_ok = 0; saved = robot.output; advance(&robot, &now, 10, 1);
	CHECK(robot.fault == FAULT_ESTOP && robot.output == saved);
	now = 0; prepare(&robot, now); inputs.permit = 0; advance(&robot, &now, 10, 1);
	CHECK(robot.fault == FAULT_PERMIT);
	now = 0; prepare(&robot, now); Robot_Tick(&robot, &inputs, APP_MAX_TICK_GAP_MS + 1);
	CHECK(robot.fault == FAULT_SCHEDULER);
	now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1000, now));
	advance(&robot, &now, APP_MAX_RUN_MS, 1);
	CHECK(robot.fault == FAULT_RUNTIME);
	now = 0; prepare(&robot, now); CHECK(Robot_Manual(&robot, 2, now));
	CHECK(!Robot_Start(&robot, DIRECTION_FWD, 1, now));
	advance(&robot, &now, APP_MANUAL_TIMEOUT_MS - 10, 1); CHECK(robot.output == 2);
	advance(&robot, &now, 10, 1); CHECK(robot.output == 0 && robot.state == ROBOT_ARMED);
	CHECK(Command_Execute(&robot, &inputs, now, "STATUS", reply, sizeof(reply)));
	CHECK(strstr(reply, "STATE=1") != 0);
	CHECK(!Command_Execute(&robot, &inputs, now, "PING", reply, 5));
	/* 抓取时先交接支撑，互斥切换，中间保留明确泄压间隔。 */
	for (direction = 0; direction < 2; direction++)
	{
		now = 0; prepare(&robot, now);
		CHECK(Robot_Grip(&robot, direction, 2, now));
		CHECK(robot.state == ROBOT_GRIPPING && (robot.output & 0x042U) == 0);
		advance(&robot, &now, APP_ANCHOR_SETTLE_MS, 1);
		CHECK(robot.phase == 1 && (robot.output & (direction ? 0x060U : 0x003U)) == 0);
		advance(&robot, &now, APP_RELEASE_MS - 10, 1);
		CHECK((robot.output & 0x042U) == 0);
		advance(&robot, &now, 10, 1);
		CHECK((robot.output & (direction ? 0x040U : 0x002U)) != 0);
		advance(&robot, &now, APP_ANCHOR_SETTLE_MS, 1);
		CHECK(robot.state == ROBOT_HOLD && Pneumatic_IsValid(robot.output));
		CHECK(!Robot_Grip(&robot, !direction, 2, now));
		CHECK(!Robot_Start(&robot, DIRECTION_FWD, 1, now));
		CHECK(Robot_Grip(&robot, direction, 1, now));
		advance(&robot, &now, 2 * APP_ANCHOR_SETTLE_MS + APP_RELEASE_MS, 1);
		CHECK(robot.state == ROBOT_HOLD && robot.output == 0x029U);
		inputs.permit = 0;
		CHECK(!Command_Execute(&robot, &inputs, now, "GRIP FRONT IN", reply, sizeof(reply)));
		CHECK(!Command_Execute(&robot, &inputs, now, "RUN FWD 1", reply, sizeof(reply)));
		CHECK(robot.output == 0x029U);
	}
#if APP_USE_FEEDBACK
	now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
	inputs.anchors = 0; advance(&robot, &now, APP_FEEDBACK_TIMEOUT_MS, 1);
	CHECK(robot.fault == FAULT_FEEDBACK && robot.phase == 0);
	now = 0; prepare(&robot, now); CHECK(Robot_Start(&robot, DIRECTION_FWD, 1, now));
	advance(&robot, &now, APP_ANCHOR_SETTLE_MS, 1); CHECK(robot.phase == 1);
	inputs.anchors = 1; advance(&robot, &now, 10, 1); CHECK(robot.fault == FAULT_FEEDBACK);
	now = 0; prepare(&robot, now); inputs.vacuum_ok = 0; advance(&robot, &now, 10, 1);
	CHECK(robot.fault == FAULT_VACUUM);
#endif
#endif
	printf("PASS channels=%d confirmed=%d feedback=%d checks=%u\n", APP_VALVE_COUNT, APP_HARDWARE_CONFIRMED, APP_USE_FEEDBACK, checks);
	return 0;
}
