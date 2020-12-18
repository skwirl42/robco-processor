#pragma once

#include <stdint.h>
#include <memory>
#include <thread>
#include <optional>
#include <chrono>
#include <mutex>

#if defined(APPLE)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <boost/lockfree/spsc_queue.hpp>

#include "sound_commands.hpp"

const int system_voices_count = 4;

enum class command_type
{
	buffer,
	exit,
};

struct command
{
	command_type type;
	size_t command_count;
	uint8_t* command_buffer;
};

enum class buffer_return_mode
{
	never_return,
	return_when_done,
};

class sound_system
{
public:
	// desired_device, if provided, should be a string retrieved from SDL_GetAudioDeviceName
	sound_system(std::string device_name = "", buffer_return_mode return_mode = buffer_return_mode::never_return);
	~sound_system();

	// begins the worker thread, which processes commands and creates sound samples
	void start_worker_thread();

	// If command is a buffer command, the buffer can be reclaimed through can_reclaim/reclaim
	void process_command(command command);
	const bool can_reclaim_buffer() const { return buffer_command_return.read_available(); }
	bool reclaim_buffer(command& command);

	const std::string& get_device_name() const { return device; }

	const bool is_initialized() const { return initialized; }
	const char* get_error() const { return error; }

private:
	void handle_command_buffer(size_t command_byte_count, uint8_t* command_bytes);
	void worker_thread_entry();
	static void sdl_audio_handler(void* userdata, uint8_t * stream, int len);

private:
	voice voices[system_voices_count];
	std::mutex voices_mutex;
	boost::lockfree::spsc_queue<command> command_queue;
	boost::lockfree::spsc_queue<command> buffer_command_return;
	std::unique_ptr<std::thread> worker_thread;
	SDL_AudioSpec device_spec;
	std::chrono::high_resolution_clock::time_point start_time;
	std::chrono::high_resolution_clock::time_point last_frame_time;
	std::string device;
	float* sample_buffer;
	size_t sample_buffer_size;
	const char* error;
	buffer_return_mode return_mode;
	SDL_AudioDeviceID device_id;
	bool initialized;
};