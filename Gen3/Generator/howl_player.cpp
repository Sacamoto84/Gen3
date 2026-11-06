#include "howl_player.h"
#include "main.h"
#include <string.h>

void HowlPlayer::Init() {
	is_loaded = false;
	is_playing = false;
	is_loop = false;
	speed_scale = 1.0f;
	current_file[0] = '\0';
	total_frames = 0;
	current_frame = 0;
	bank_start_frame[0] = 0;
	bank_start_frame[1] = 0;
	bank_valid_count[0] = 0;
	bank_valid_count[1] = 0;
	subframe_accum = 0.0f;
}

void HowlPlayer::CloseFile() {
	is_playing = false;
	bool was_loaded = is_loaded;
	is_loaded = false;
	__DMB();

	if (was_loaded) {
		f_close(&file);
	}
	total_frames = 0;
	current_frame = 0;
	bank_start_frame[0] = 0;
	bank_start_frame[1] = 0;
	bank_valid_count[0] = 0;
	bank_valid_count[1] = 0;
}

bool HowlPlayer::FillBank(uint8_t b, uint32_t target_frame) {
	if (!is_loaded || target_frame >= total_frames || b > 1) return false;

	// Сбрасываем валидность банка перед чтением, чтобы ISR не прочитал недописанные данные
	bank_valid_count[b] = 0;
	__DMB();

	uint32_t offset = 8 + target_frame * 16;
	if (f_lseek(&file, offset) != FR_OK) return false;

	uint32_t frames_to_read = total_frames - target_frame;
	if (frames_to_read > HOWL_PLAYER_BANK_FRAMES) frames_to_read = HOWL_PLAYER_BANK_FRAMES;

	UINT bytes_to_read = frames_to_read * 16;
	UINT br = 0;
	if (f_read(&file, (void*)buffer[b], bytes_to_read, &br) != FR_OK) return false;

	bank_start_frame[b] = target_frame;
	__DMB();
	bank_valid_count[b] = (uint8_t)(br / 16);
	return bank_valid_count[b] > 0;
}

bool HowlPlayer::LoadFile(const char* filepath) {
	CloseFile();

	if (f_open(&file, filepath, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
		return false;
	}

	char header[8];
	UINT br = 0;
	if (f_read(&file, header, 8, &br) != FR_OK || br != 8) {
		f_close(&file);
		return false;
	}

	if (memcmp(header, "YEAHBOI!", 8) != 0) {
		f_close(&file);
		return false;
	}

	FSIZE_t size = f_size(&file);
	if (size < 24) {
		f_close(&file);
		return false;
	}

	total_frames = (uint32_t)((size - 8) / 16);
	current_frame = 0;
	subframe_accum = 0.0f;
	strncpy(current_file, filepath, sizeof(current_file) - 1);
	current_file[sizeof(current_file) - 1] = '\0';

	is_loaded = true;
	FillBank(0, 0);
	if (bank_valid_count[0] < total_frames) {
		FillBank(1, bank_valid_count[0]);
	} else {
		bank_valid_count[1] = 0;
	}
	return true;
}

void HowlPlayer::Play() {
	if (is_loaded) is_playing = true;
}

void HowlPlayer::Pause() {
	is_playing = false;
}

void HowlPlayer::Stop() {
	is_playing = false;
	current_frame = 0;
	subframe_accum = 0.0f;
	if (is_loaded) {
		FillBank(0, 0);
		if (bank_valid_count[0] < total_frames) {
			FillBank(1, bank_valid_count[0]);
		} else {
			bank_valid_count[1] = 0;
		}
	}
}

void HowlPlayer::Update() {
	if (!is_loaded || !is_playing || total_frames == 0) return;

	// Определяем активный банк (где сейчас находится current_frame)
	uint8_t active_bank = 0xFF;
	if (current_frame >= bank_start_frame[0] && current_frame < (bank_start_frame[0] + bank_valid_count[0])) {
		active_bank = 0;
	} else if (current_frame >= bank_start_frame[1] && current_frame < (bank_start_frame[1] + bank_valid_count[1])) {
		active_bank = 1;
	}

	if (active_bank == 0xFF) {
		// Текущий кадр выпал из обоих банков (например, переход/Seek или старт)
		FillBank(0, current_frame);
		if (bank_valid_count[0] > 0) {
			uint32_t next = bank_start_frame[0] + bank_valid_count[0];
			if (next < total_frames) {
				FillBank(1, next);
			} else if (is_loop) {
				FillBank(1, 0);
			} else {
				bank_valid_count[1] = 0;
			}
		}
		return;
	}

	// Заполняем альтернативный (фоновый) банк на опережение
	uint8_t next_bank = active_bank ^ 1;
	uint32_t needed_next_frame = bank_start_frame[active_bank] + bank_valid_count[active_bank];
	if (needed_next_frame >= total_frames && is_loop) {
		needed_next_frame = 0;
	}

	if (needed_next_frame < total_frames) {
		if (bank_start_frame[next_bank] != needed_next_frame || bank_valid_count[next_bank] == 0) {
			FillBank(next_bank, needed_next_frame);
		}
	}
}

HowlPulse HowlPlayer::Step() {
	if (!is_loaded || !is_playing || total_frames == 0) {
		return { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	subframe_accum += speed_scale;
	while (subframe_accum >= 1.0f) {
		subframe_accum -= 1.0f;
		current_frame++;
		if (current_frame >= total_frames) {
			if (is_loop) {
				current_frame = 0;
			} else {
				current_frame = total_frames - 1;
				is_playing = false;
				break;
			}
		}
	}

	if (current_frame >= bank_start_frame[0] && current_frame < (bank_start_frame[0] + bank_valid_count[0])) {
		return buffer[0][current_frame - bank_start_frame[0]];
	}
	if (current_frame >= bank_start_frame[1] && current_frame < (bank_start_frame[1] + bank_valid_count[1])) {
		return buffer[1][current_frame - bank_start_frame[1]];
	}

	return { 0.0f, 0.0f, 0.0f, 0.0f };
}

uint32_t HowlPlayer::GetCurrentSec() const {
	return current_frame / 40;
}

uint32_t HowlPlayer::GetTotalSec() const {
	return total_frames / 40;
}
