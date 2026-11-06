#ifndef HOWL_PLAYER_H_
#define HOWL_PLAYER_H_

#include "howl_types.h"
#include "fatfs.h"

#define HOWL_PLAYER_BANK_FRAMES 32

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

	// Вызывается в thread mode (UI loop) для неблокирующей подкачки с SD
	void      Update();

	// Вызывается из DMA ISR (только быстрое чтение из памяти)
	HowlPulse Step();

	uint32_t  GetCurrentSec() const;
	uint32_t  GetTotalSec() const;

private:
	FIL       file;
	HowlPulse buffer[2][HOWL_PLAYER_BANK_FRAMES];
	uint32_t  bank_start_frame[2];
	uint8_t   bank_valid_count[2];
	float     subframe_accum;

	bool      FillBank(uint8_t b, uint32_t target_frame);
};

#endif /* HOWL_PLAYER_H_ */
