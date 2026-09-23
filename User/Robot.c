#include <string.h>
#include "Robot.h"

typedef struct
{
	uint8_t anchors;
	uint8_t rear_short;
	uint8_t front_short;
	uint8_t support;
	uint32_t duration;
} RobotPhase;

/* 所有时间只是台架起点。伸长依赖折纸被动回弹；中段平移需两段行程兼容。
 * 释放相和运动相分离，避免锚点尚未卸载就拉伸。抓紧相保留旧锚点。 */
static const RobotPhase Robot_Phases[] =
{
	{7, 0, 0, 0, APP_ANCHOR_SETTLE_MS}, /* 先建立三锚点，禁止在此改变伸缩长度 */
	{6, 0, 0, 6, APP_RELEASE_MS},       /* 初始化：放后端，中/前承载 */
	{6, 1, 0, 6, APP_STROKE_MS},       /* 初始化：后段缩短，后端靠近中锚 */
	{7, 1, 0, 6, APP_ANCHOR_SETTLE_MS}, /* 初始化：重新锚定后端 */
	{3, 1, 0, 3, APP_RELEASE_MS},       /* 放前端，后/中承载 */
	{3, 1, 1, 3, APP_STROKE_MS},       /* 仅首周期整形：回收前端 */
	{3, 1, 0, 3, APP_STROKE_MS},       /* 前段伸长：前端前进 */
	{7, 1, 0, 3, APP_ANCHOR_SETTLE_MS}, /* 建立前端锚点 */
	{5, 1, 0, 5, APP_RELEASE_MS},       /* 放中锚，双端承载 */
	{5, 0, 1, 5, APP_STROKE_MS},       /* 前缩后伸：中锚前进 */
	{7, 0, 1, 5, APP_ANCHOR_SETTLE_MS}, /* 建立中锚 */
	{6, 0, 1, 6, APP_RELEASE_MS},       /* 放后端，中/前承载 */
	{6, 1, 1, 6, APP_STROKE_MS},       /* 后缩：拖动后端前进 */
	{7, 1, 1, 6, APP_ANCHOR_SETTLE_MS}, /* 建立后端；每周期结束双段缩短 */
};

static uint8_t Robot_IsActive(const Robot *robot)
{
	return robot->state == ROBOT_ARMED || robot->state == ROBOT_RUNNING ||
		robot->state == ROBOT_MANUAL || robot->state == ROBOT_GRIPPING;
}

static uint8_t Robot_MirrorAnchors(uint8_t anchors)
{
	return (uint8_t)((anchors & 2U) | ((anchors & 1U) << 2) | ((anchors & 4U) >> 2));
}

static uint8_t Robot_DesiredAnchors(const Robot *robot)
{
	uint8_t anchors = Robot_Phases[robot->phase].anchors;
	return robot->direction == DIRECTION_REV ? Robot_MirrorAnchors(anchors) : anchors;
}

static void Robot_EnterPhase(Robot *robot, uint32_t now)
{
	const RobotPhase *phase = &Robot_Phases[robot->phase];
	uint16_t segments;
	uint8_t rear = phase->rear_short;
	uint8_t front = phase->front_short;
	robot->required_anchors = phase->support;
	if (robot->direction == DIRECTION_REV)
	{
		rear = phase->front_short;
		front = phase->rear_short;
		robot->required_anchors = Robot_MirrorAnchors(phase->support);
	}
	segments = Pneumatic_SegmentBits(0, rear ? 7 : 0) | Pneumatic_SegmentBits(1, front ? 7 : 0);
	/* 抓紧/释放相保持伸缩状态。后续周期跳过初始化与整形。 */
	if (robot->phase == 0 || robot->phase == 1 || robot->phase == 3 || robot->phase == 4 ||
		robot->phase == 7 || robot->phase == 8 || robot->phase == 10 || robot->phase == 11 || robot->phase == 13)
	{
		segments = robot->output & Pneumatic_AllSegments();
	}
	robot->output = segments | Pneumatic_AnchorBits(Robot_DesiredAnchors(robot), robot->direction);
	robot->phase_at = now;
	robot->stable_tracking = 0;
}

void Robot_Init(Robot *robot, uint32_t now)
{
	memset(robot, 0, sizeof(*robot));
	robot->last_tick = now;
	robot->link_at = now;
}

void Robot_Heartbeat(Robot *robot, uint32_t now)
{
	robot->link_at = now;
}

uint8_t Robot_Arm(Robot *robot, const RobotInputs *inputs, uint32_t now)
{
	if (!APP_HARDWARE_CONFIRMED || robot->state != ROBOT_IDLE ||
		!inputs->permit || !inputs->estop_ok || (APP_USE_FEEDBACK && !inputs->vacuum_ok))
	{
		return 0;
	}
	robot->state = ROBOT_ARMED;
	robot->link_at = now;
	robot->last_tick = now;
	return 1;
}

uint8_t Robot_Start(Robot *robot, RobotDirection direction, uint16_t cycles, uint32_t now)
{
	if ((robot->state != ROBOT_ARMED && !(robot->state == ROBOT_HOLD && (robot->output & 0x06BU) == 0x029U)) ||
		(uint32_t)(now - robot->link_at) >= APP_LINK_TIMEOUT_MS || cycles == 0 || cycles > APP_MAX_CYCLES ||
		(direction != DIRECTION_FWD && direction != DIRECTION_REV))
	{
		return 0;
	}
	robot->direction = direction;
	robot->cycles_done = 0;
	robot->cycles_target = cycles;
	robot->phase = 0;
	robot->run_at = now;
	robot->state = ROBOT_RUNNING;
	Robot_EnterPhase(robot, now);
	return 1;
}

void Robot_Stop(Robot *robot)
{
	if (Robot_IsActive(robot))
	{
		robot->state = ROBOT_HOLD;
	}
}

void Robot_Fail(Robot *robot, RobotFault fault)
{
	if (robot->state != ROBOT_FAULT)
	{
		robot->fault = fault;
		robot->state = ROBOT_FAULT;
	}
}

uint8_t Robot_Release(Robot *robot, const RobotInputs *inputs)
{
	if (inputs->permit || robot->state == ROBOT_RUNNING || robot->state == ROBOT_MANUAL || robot->state == ROBOT_GRIPPING)
	{
		return 0;
	}
	robot->output = 0;
	robot->state = ROBOT_IDLE;
	robot->fault = FAULT_NONE;
	return 1;
}

uint8_t Robot_Manual(Robot *robot, uint16_t mask, uint32_t now)
{
	if (robot->state != ROBOT_ARMED || !Pneumatic_IsValid(mask))
	{
		return 0;
	}
	robot->output = mask;
	robot->state = ROBOT_MANUAL;
	robot->phase_at = now;
	return 1;
}

uint8_t Robot_Grip(Robot *robot, uint8_t front, uint8_t mode, uint32_t now)
{
	uint16_t other_inner = front ? 0x002U : 0x040U;
	if (!APP_HARDWARE_CONFIRMED || (robot->state != ROBOT_ARMED && robot->state != ROBOT_HOLD) ||
		(uint32_t)(now - robot->link_at) >= APP_LINK_TIMEOUT_MS || front > 1 || mode > 2 || (robot->output & other_inner) != 0)
	{
		return 0;
	}
	robot->grip_front = front;
	robot->grip_mode = mode;
	robot->required_anchors = front ? 3 : 6;
	robot->output |= Pneumatic_AnchorBits(robot->required_anchors, DIRECTION_FWD);
	robot->state = ROBOT_GRIPPING;
	robot->phase = 0;
	robot->phase_at = now;
	robot->run_at = now;
	robot->stable_tracking = 0;
	return 1;
}

static void Robot_TickGrip(Robot *robot, const RobotInputs *inputs, uint32_t now)
{
	uint8_t desired = robot->required_anchors;
	uint32_t duration = robot->phase == 1 ? APP_RELEASE_MS : APP_ANCHOR_SETTLE_MS;
	uint16_t pair = robot->grip_front ? 0x060U : 0x003U;
	if (robot->phase == 2 && robot->grip_mode == 1)
	{
		desired |= robot->grip_front ? ANCHOR_FRONT : ANCHOR_REAR;
	}
	if (APP_USE_FEEDBACK && robot->phase != 0 &&
		(inputs->anchors & robot->required_anchors) != robot->required_anchors)
	{
		Robot_Fail(robot, FAULT_FEEDBACK);
		return;
	}
	if ((inputs->anchors & desired) == desired)
	{
		if (!robot->stable_tracking) { robot->stable_tracking = 1; robot->stable_at = now; }
	}
	else { robot->stable_tracking = 0; }
	if ((uint32_t)(now - robot->phase_at) < duration) { return; }
	if (APP_USE_FEEDBACK && (!robot->stable_tracking || (uint32_t)(now - robot->stable_at) < APP_FEEDBACK_STABLE_MS))
	{
		if ((uint32_t)(now - robot->phase_at) >= APP_FEEDBACK_TIMEOUT_MS) { Robot_Fail(robot, FAULT_FEEDBACK); }
		return;
	}
	if (robot->phase == 0) { robot->output &= ~pair; robot->phase = 1; }
	else if (robot->phase == 1)
	{
		if (robot->grip_mode == 1) { robot->output |= robot->grip_front ? 0x020U : 0x001U; }
		else if (robot->grip_mode == 2) { robot->output |= robot->grip_front ? 0x040U : 0x002U; }
		robot->phase = 2;
	}
	else { robot->state = ROBOT_HOLD; }
	robot->phase_at = now;
	robot->stable_tracking = 0;
}

void Robot_Tick(Robot *robot, const RobotInputs *inputs, uint32_t now)
{
	uint32_t gap = now - robot->last_tick;
	uint32_t elapsed;
	uint8_t desired;
	robot->last_tick = now;
	if (!Robot_IsActive(robot))
	{
		return;
	}
	if (!inputs->estop_ok) { Robot_Fail(robot, FAULT_ESTOP); return; }
	if (!inputs->permit) { Robot_Fail(robot, FAULT_PERMIT); return; }
	if (gap > APP_MAX_TICK_GAP_MS) { Robot_Fail(robot, FAULT_SCHEDULER); return; }
	if ((uint32_t)(now - robot->link_at) >= APP_LINK_TIMEOUT_MS) { Robot_Fail(robot, FAULT_LINK); return; }
	if (APP_USE_FEEDBACK && !inputs->vacuum_ok) { Robot_Fail(robot, FAULT_VACUUM); return; }
	if (robot->state == ROBOT_GRIPPING)
	{
		Robot_TickGrip(robot, inputs, now);
		return;
	}
	if (robot->state == ROBOT_MANUAL)
	{
		if ((uint32_t)(now - robot->phase_at) >= APP_MANUAL_TIMEOUT_MS)
		{
			robot->output = 0;
			robot->state = ROBOT_ARMED;
		}
		return;
	}
	if (robot->state != ROBOT_RUNNING) { return; }
	if ((uint32_t)(now - robot->run_at) >= APP_MAX_RUN_MS) { Robot_Fail(robot, FAULT_RUNTIME); return; }
	if (APP_USE_FEEDBACK && (inputs->anchors & robot->required_anchors) != robot->required_anchors)
	{
		Robot_Fail(robot, FAULT_FEEDBACK);
		return;
	}
	desired = Robot_DesiredAnchors(robot);
	if ((inputs->anchors & desired) == desired)
	{
		if (!robot->stable_tracking) { robot->stable_tracking = 1; robot->stable_at = now; }
	}
	else { robot->stable_tracking = 0; }
	elapsed = now - robot->phase_at;
	if (elapsed < Robot_Phases[robot->phase].duration) { return; }
	if (APP_USE_FEEDBACK && (!robot->stable_tracking || (uint32_t)(now - robot->stable_at) < APP_FEEDBACK_STABLE_MS))
	{
		if (elapsed >= APP_FEEDBACK_TIMEOUT_MS) { Robot_Fail(robot, FAULT_FEEDBACK); }
		return;
	}
	if (robot->phase == 13)
	{
		robot->cycles_done++;
		if (robot->cycles_done >= robot->cycles_target) { robot->state = ROBOT_HOLD; return; }
		robot->phase = 4;
	}
	else if (robot->phase == 4 && robot->cycles_done != 0) { robot->phase = 6; }
	else { robot->phase++; }
	Robot_EnterPhase(robot, now);
}
