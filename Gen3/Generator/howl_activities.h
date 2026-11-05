#ifndef HOWL_ACTIVITIES_H_
#define HOWL_ACTIVITIES_H_

#include "howl_types.h"

class HowlActivities {
public:
	ActivityId active_id;
	float      speed_scale;      // 0.25x .. 4.0x (по умолчанию 1.0)
	float      intensity_scale;  // 0.1x .. 2.0x (по умолчанию 1.0)
	uint32_t   million_seed;     // 0 .. 999999 для Million Seeds

	void       Init();
	void       Reset();
	void       SelectActivity(ActivityId id);
	HowlPulse  Step();

	static const char* GetActivityName(ActivityId id);

private:
	uint32_t tick_count;
	uint32_t sub_state;
	float    phase;
	uint32_t rng_state;

	HowlPulse StepLick();
	HowlPulse StepPenetration();
	HowlPulse StepVibro();
	HowlPulse StepMilker();
	HowlPulse StepChaos();
	HowlPulse StepLuxuryHJ();
	HowlPulse StepOpposites();
	HowlPulse StepRelentless();
	HowlPulse StepSineTime();
	HowlPulse StepMillionSeeds();

	float RandFloat();
};

#endif /* HOWL_ACTIVITIES_H_ */
