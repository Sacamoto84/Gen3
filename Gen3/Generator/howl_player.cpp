#include "howl_player.h"
#include <string.h>

void HowlPlayer::Init() {
	is_loaded = false;
	is_playing = false;
	is_loop = false;
	speed_scale = 1.0f;
	current_file[0] = '\0';
	total_frames = 0;
	current_frame = 0;
	buf_start_frame = 0;
	buf_valid_count = 0;
	subframe_accum = 0.0f;
}

void HowlPlayer::CloseFile() {
	if (is_loaded) {
		f_close(&file);
		is_loaded = false;
	}
	is_playing = false;
	total_frames = 0;
	current_frame = 0;
	buf_valid_count = 0;
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
	RefillBuffer(0);
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
	if (is_loaded) RefillBuffer(0);
}

bool HowlPlayer::RefillBuffer(uint32_t target_frame) {
	if (!is_loaded || target_frame >= total_frames) return false;

	uint32_t offset = 8 + target_frame * 16;
	if (f_lseek(&file, offset) != FR_OK) return false;

	uint32_t frames_to_read = total_frames - target_frame;
	if (frames_to_read > HOWL_PLAYER_BUFFER_FRAMES) frames_to_read = HOWL_PLAYER_BUFFER_FRAMES;

	UINT bytes_to_read = frames_to_read * 16;
	UINT br = 0;
	if (f_read(&file, buffer, bytes_to_read, &br) != FR_OK) return false;

	buf_start_frame = target_frame;
	buf_valid_count = (uint8_t)(br / 16);
	return buf_valid_count > 0;
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

	if (current_frame < buf_start_frame || current_frame >= (buf_start_frame + buf_valid_count)) {
		if (!RefillBuffer(current_frame)) {
			return { 0.0f, 0.0f, 0.0f, 0.0f };
		}
	}

	uint32_t local_idx = current_frame - buf_start_frame;
	if (local_idx < buf_valid_count) {
		return buffer[local_idx];
	}

	return { 0.0f, 0.0f, 0.0f, 0.0f };
}

uint32_t HowlPlayer::GetCurrentSec() const {
	return current_frame / 40;
}

uint32_t HowlPlayer::GetTotalSec() const {
	return total_frames / 40;
}
