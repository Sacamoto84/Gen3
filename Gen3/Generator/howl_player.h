#ifndef HOWL_PLAYER_H_
#define HOWL_PLAYER_H_

#include "howl_types.h"
#include "fatfs.h"

#define HOWL_PLAYER_BUFFER_FRAMES 16

class HowlPlayer {
public:
	bool     is_loaded;
	bool     is_playing;
	bool     is_loop;
	float    speed_scale;       // 0.5x .. 2.0x
	char     current_file[64];
	uint32_t total_frames;
	uint32_t current_frame;

	void      Init();
	bool      LoadFile(const char* filepath);
	void      CloseFile();
	void      Play();
	void      Pause();
	void      Stop();
	HowlPulse Step();

	uint32_t  GetCurrentSec() const;
	uint32_t  GetTotalSec() const;

private:
	FIL       file;
	HowlPulse buffer[HOWL_PLAYER_BUFFER_FRAMES];
	uint32_t  buf_start_frame;
	uint8_t   buf_valid_count;
	float     subframe_accum;

	bool      RefillBuffer(uint32_t target_frame);
};

#endif /* HOWL_PLAYER_H_ */
