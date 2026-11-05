#ifndef HOWL_WAVE_GEN_H_
#define HOWL_WAVE_GEN_H_

#include "howl_types.h"

struct WavePoint {
	float time;
	float position;
	float slope;
};

struct WaveShapeDef {
	const char*       name;
	uint8_t           num_points;
	bool              is_hermite;
	const WavePoint*  points;
};

class WaveManager {
public:
	WaveShapeId shape_id;
	float       speed_hz;
	float       min_val;
	float       max_val;
	float       phase;
	bool        auto_change_en;

	void  Init(WaveShapeId shape, float speed, float min_v, float max_v);
	void  Reset();
	float Step();
	void  Randomize(bool is_freq);

private:
	void  AutoChangeStep(bool is_freq);
	float EvaluateShape(float u);
};

class HowlWaveGenerator {
public:
	WaveManager amp_wave;
	WaveManager freq_wave;

	void        Init();
	void        Reset();
	HowlPulse   Step();
	void        Randomize();

	static const char* GetShapeName(WaveShapeId id);
	static const WaveShapeDef& GetShapeDef(WaveShapeId id);
};

#endif /* HOWL_WAVE_GEN_H_ */
