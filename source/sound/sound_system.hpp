#pragma once

#include <stdint.h>
#include <memory>
#include <thread>
#include <optional>
#include <chrono>

#include <SDL.h>

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
	sound_system(buffer_return_mode return_mode = buffer_return_mode::never_return, std::optional<std::string> desired_device = nullptr);
	~sound_system();

	// begins the worker thread, which processes commands and creates sound samples
	void start_worker_thread();

	// If command is a buffer command, the buffer can be reclaimed through can_reclaim/reclaim
	void process_command(command command);
	const bool can_reclaim_buffer() const { return buffer_command_return.read_available(); }
	bool reclaim_buffer(command& command);

	const bool is_initialized() const { return initialized; }
	const char* get_error() const { return error; }

private:
	void handle_command_buffer(size_t command_byte_count, uint8_t* command_bytes);
	void worker_thread_entry();

private:
	voice voices[system_voices_count];
	boost::lockfree::spsc_queue<command> command_queue;
	boost::lockfree::spsc_queue<command> buffer_command_return;
	std::unique_ptr<std::thread> worker_thread;
	SDL_AudioSpec device_spec;
	std::chrono::high_resolution_clock::time_point start_time;
	float* sample_buffer;
	size_t sample_buffer_size;
	const char* error;
	buffer_return_mode return_mode;
	SDL_AudioDeviceID device_id;
	int current_voice;
	bool initialized;
};