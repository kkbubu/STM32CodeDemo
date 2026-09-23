#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* 未确认气路、抓手承载、驱动极性与防坠措施前，保持 0。 */
#ifndef APP_HARDWARE_CONFIRMED
#define APP_HARDWARE_CONFIRMED 0
#endif
#ifndef APP_VALVE_COUNT
#define APP_VALVE_COUNT 7
#endif
#if APP_VALVE_COUNT != 7 && APP_VALVE_COUNT != 11
#error APP_VALVE_COUNT_must_be_7_or_11
#endif
/* 默认高有效。低有效模块必须改为 0 并加外部上拉。 */
#ifndef APP_MOS_ACTIVE_HIGH
#define APP_MOS_ACTIVE_HIGH 1
#endif
/* 1：PA0/1/2 接后/中/前锚定反馈，PA3 接负压就绪，均低有效。
 * 0：无传感器的定时试验模式，不代表已确认真实抓牢。 */
#ifndef APP_USE_FEEDBACK
#define APP_USE_FEEDBACK 0
#endif
#define APP_ANCHOR_SETTLE_MS 1200UL
#define APP_RELEASE_MS 500UL
#define APP_STROKE_MS 2000UL
#define APP_FEEDBACK_TIMEOUT_MS 5000UL
#define APP_FEEDBACK_STABLE_MS 100UL
#define APP_LINK_TIMEOUT_MS 5000UL
#define APP_MAX_RUN_MS 120000UL
#define APP_MAX_TICK_GAP_MS 100UL
#define APP_MANUAL_TIMEOUT_MS 2000UL
#define APP_MAX_CYCLES 1000U

/* 用户确认：每端一路向外锚定、一路向内抓取，互斥。
 * 无论前进后退，爬行均使用向外锚定回路。 */
#define APP_REAR_GRIP_FWD 0x001U
#define APP_REAR_GRIP_REV 0x001U
#define APP_FRONT_GRIP_FWD 0x020U
#define APP_FRONT_GRIP_REV 0x020U

#endif
