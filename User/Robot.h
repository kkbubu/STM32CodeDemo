#ifndef ROBOT_H
#define ROBOT_H
#include "Pneumatic.h"

typedef enum { ROBOT_IDLE, ROBOT_ARMED, ROBOT_RUNNING, ROBOT_HOLD, ROBOT_FAULT, ROBOT_MANUAL, ROBOT_GRIPPING } RobotState;
typedef enum { FAULT_NONE, FAULT_ESTOP, FAULT_PERMIT, FAULT_LINK, FAULT_FEEDBACK,
	FAULT_VACUUM, FAULT_RUNTIME, FAULT_SCHEDULER, FAULT_IO } RobotFault;
typedef struct
{
	uint8_t permit;        /* PA8 接地：允许运行；断线禁止。 */
	uint8_t estop_ok;      /* PB14 常闭回路接地：正常；断线/按下故障。 */
	uint8_t anchors;       /* 后/中/前到位：bit0/1/2。 */
	uint8_t vacuum_ok;
} RobotInputs;
typedef struct
{
	RobotState state;
	RobotFault fault;
	RobotDirection direction;
	uint16_t output;
	uint16_t cycles_done;
	uint16_t cycles_target;
	uint8_t phase;
	uint8_t required_anchors;
	uint8_t stable_tracking;
	uint8_t grip_front;
	uint8_t grip_mode;
	uint32_t phase_at;
	uint32_t run_at;
	uint32_t link_at;
	uint32_t last_tick;
	uint32_t stable_at;
} Robot;

/* 初始化仅改变上下文，输出为零；时间单位均为 ms，支持 uint32_t 回绕。 */
void Robot_Init(Robot *robot, uint32_t now);
void Robot_Tick(Robot *robot, const RobotInputs *inputs, uint32_t now);
void Robot_Heartbeat(Robot *robot, uint32_t now);
uint8_t Robot_Arm(Robot *robot, const RobotInputs *inputs, uint32_t now);
uint8_t Robot_Start(Robot *robot, RobotDirection direction, uint16_t cycles, uint32_t now);
/* 停止冻结当前输出，不自动泄压，也不保证物理保压；重新运行必须先托住并 RELEASE。 */
void Robot_Stop(Robot *robot);
void Robot_Fail(Robot *robot, RobotFault fault);
/* RELEASE 仅在物理许可断开时允许；必须先人工托住机器人。 */
uint8_t Robot_Release(Robot *robot, const RobotInputs *inputs);
/* 点动仅在 ARMED，最长 2 秒后恢复零；用于台架，不用于悬空机器人。 */
uint8_t Robot_Manual(Robot *robot, uint16_t mask, uint32_t now);
/* 抓手操作：front=0 后端/1 前端；mode=0 松开/1 外撑/2 内抓。
 * 先补足另一端和中锚，等待，再对目标端先泄压后切换；只允许一端内抓。
 * 从 ARMED 或 HOLD 启动，完成后 HOLD 持续保持，不自动转为爬行。 */
uint8_t Robot_Grip(Robot *robot, uint8_t front, uint8_t mode, uint32_t now);
#endif
