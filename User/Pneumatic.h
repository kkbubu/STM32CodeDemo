#ifndef PNEUMATIC_H
#define PNEUMATIC_H
#include <stdint.h>
#include "AppConfig.h"

typedef enum { ANCHOR_REAR = 1, ANCHOR_MIDDLE = 2, ANCHOR_FRONT = 4 } AnchorMask;
/* 后端→后伸缩→中锚→前伸缩→前端；FWD 朝前端，REV 朝后端。 */
typedef enum { DIRECTION_FWD = 0, DIRECTION_REV = 1 } RobotDirection;

/* 返回吸负压通道位图；bit0 为 CH1。所有函数非阻塞，无硬件副作用。 */
uint16_t Pneumatic_AnchorBits(uint8_t anchors, RobotDirection direction);
uint16_t Pneumatic_SegmentBits(uint8_t front, uint8_t chambers);
uint8_t Pneumatic_IsValid(uint16_t mask);
uint16_t Pneumatic_AllSegments(void);
#endif
