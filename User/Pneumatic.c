#include "Pneumatic.h"

uint16_t Pneumatic_AnchorBits(uint8_t anchors, RobotDirection direction)
{
	uint16_t mask = 0;
	if ((anchors & ANCHOR_REAR) != 0)
	{
		mask |= (direction == DIRECTION_FWD) ? APP_REAR_GRIP_FWD : APP_REAR_GRIP_REV;
	}
	if ((anchors & ANCHOR_MIDDLE) != 0)
	{
		mask |= 0x008U;
	}
	if ((anchors & ANCHOR_FRONT) != 0)
	{
		mask |= (direction == DIRECTION_FWD) ? APP_FRONT_GRIP_FWD : APP_FRONT_GRIP_REV;
	}
	return mask;
}

uint16_t Pneumatic_SegmentBits(uint8_t front, uint8_t chambers)
{
	uint16_t mask = 0;
	if ((chambers & 1U) != 0)
	{
		mask |= front ? 0x010U : 0x004U;
	}
#if APP_VALVE_COUNT == 11
	if ((chambers & 2U) != 0)
	{
		mask |= front ? 0x200U : 0x080U;
	}
	if ((chambers & 4U) != 0)
	{
		mask |= front ? 0x400U : 0x100U;
	}
#endif
	return mask;
}

uint16_t Pneumatic_AllSegments(void)
{
	return Pneumatic_SegmentBits(0, 7) | Pneumatic_SegmentBits(1, 7);
}

uint8_t Pneumatic_IsValid(uint16_t mask)
{
	if ((mask & ~((1U << APP_VALVE_COUNT) - 1U)) != 0)
	{
		return 0;
	}
	/* 未证明两路可同时动作前，双向抓手禁止同时吸气。 */
	return ((mask & 0x003U) != 0x003U) && ((mask & 0x060U) != 0x060U);
}
