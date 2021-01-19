#include "sound_system.hpp"

#include <functional>
#include <iostream>

namespace
{
	const int COMMAND_QUEUE_SIZE = 16;
	const int SAMPLES_COUNT = 512;
	const int DEFAULT_SAMPLE_RATE = 48000;
	const int CD_SAMPLE_RATE = 44100;
	const float PI = acos(-1);

	typedef std::chrono::duration<long, std::ratio<1, DEFAULT_SAMPLE_RATE>> default_sample_duration;
	typedef std::chrono::duration<long, std::ratio<1, CD_SAMPLE_RATE>> cd_sample_duration;

	float clip(float sample, float max)
	{
		if (sample >= 0.0)
		{
			return fmin(sample, max);
		}
		else
		{
			return fmax(sample, -max);
		}
	}

	float scale(const float octave, const note note)
	{
		float note_at_octave = octave * 12 + (int)note;
		return 8 * pow(1.059463f, note_at_octave);
	}

	float envelope(const float time_micros, voice& voice)
	{
		float amplitude = 0;
		float release_amplitude = 0;
		float time = time_micros / 1000000.0f;

		if (voice.time_started > voice.time_ended)
		{
			float lifetime = time - voice.time_started;
			if (lifetime <= voice.attack_time)
			{
				amplitude = (lifetime - voice.attack_time) * voice.start_amplitude;
			}
			if (lifetime > voice.attack_time && lifetime <= (voice.attack_time + voice.decay_time))
			{
				amplitude = ((lifetime - voice.attack_time) / voice.decay_time) * (voice.sustain_amplitude - voice.start_amplitude) + voice.start_amplitude;
			}
			if (lifetime > (voice.attack_time + voice.decay_time))
			{
				amplitude = voice.sustain_amplitude;
			}
		}
		else
		{
			float lifetime = voice.time_ended - voice.time_started;
			if (lifetime <= voice.attack_time)
			{
				release_amplitude = (lifetime - voice.attack_time) * voice.start_amplitude;
			}
			if (lifetime > voice.attack_time && lifetime <= (voice.attack_time + voice.decay_time))
			{
				release_amplitude = ((lifetime - voice.attack_time) / voice.decay_time) * (voice.sustain_amplitude - voice.start_amplitude) + voice.start_amplitude;
			}
			if (lifetime > (voice.attack_time + voice.decay_time))
			{
				release_amplitude = voice.sustain_amplitude;
			}

			amplitude = ((time - voice.time_ended) / voice.release_time) * (0 - release_amplitude) + release_amplitude;
		}

		if (amplitude <= 0.01f)
		{
			amplitude = 0;
		}
		else if (isnan(amplitude) || isinf(amplitude))
		{
			amplitude = 0;
		}

		return amplitude;
	}

	float osc(const float time, voice& voice, int oscillator_id, float custom = 50.0)
	{
		// time is in microseconds, we need it in seconds
		auto seconds = time / 1000000.0f;
		auto& oscillator = voice.oscillators[oscillator_id];
		auto octave = voice.octave + oscillator.octave;
		auto hertz = scale(octave, voice.current_note);
		auto vel = hertz * 2.0f * PI;
		auto lfoVel = oscillator.lfo_frequency * 2.0f * PI;
		float frequency = vel * seconds + oscillator.lfo_amplitude * hertz * (sin(lfoVel * seconds));

		switch (oscillator.type)
		{
		case oscillator_type::sine: // Sine wave bewteen -1 and +1
			return sin(frequency);

		case oscillator_type::square: // Square wave between -1 and +1
			return sin(frequency) > 0 ? 1.0 : -1.0;

		case oscillator_type::triangle: // Triangle wave between -1 and +1
			return asin(sin(frequency)) * (2.0 / PI);

		case oscillator_type::saw: // Saw wave (analogue / warm / slow)
		{
			float output = 0.0f;
			for (float n = 1.0; n < custom; n++)
				output += (sin(n * frequency)) / n;
			return output * (2.0 / PI);
		}

		case oscillator_type::noise:
			return 2.0 * ((float)rand() / (float)RAND_MAX) - 1.0;

		case oscillator_type::silence:
		default:
			return 0.0f;
		}
	}
}

sound_system::sound_system(std::string device_name, buffer_return_mode return_mode)
	: voices{},
	  command_queue(COMMAND_QUEUE_SIZE), buffer_command_return(COMMAND_QUEUE_SIZE), worker_thread(), 
	  device_spec{}, device(device_name), error(nullptr),
	  return_mode(return_mode), device_id(0), initialized(false)
{
	auto audio_device_count = SDL_GetNumAudioDevices(0);
	if (audio_device_count > 0)
	{
		if (device == "")
		{
			device = SDL_GetAudioDeviceName(0, 0);
		}

		SDL_AudioSpec want{ DEFAULT_SAMPLE_RATE, AUDIO_F32, 1, 0, SAMPLES_COUNT, 0, 0, sound_system::sdl_audio_handler, this };
			
		device_id = SDL_OpenAudioDevice(device.c_str(), 0, &want, &device_spec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
		if (device_id == 0)
		{
			error = SDL_GetError();
		}
		else
		{
			for (int i = 0; i < system_voices_count; i++)
			{
				voices[i].volume = 0;
				voices[i].current_note = note::none;
			}
			sample_buffer_size = (static_cast<size_t>(SDL_AUDIO_BITSIZE(device_spec.format) / 8)) * device_spec.samples;

			memset(voices, 0, sizeof(voices));

			initialized = true;
		}
	}
	else
	{
		error = "Couldn't find any non-capture audio devices";
	}
}

sound_system::~sound_system()
{
	if (worker_thread.get() != nullptr && worker_thread->joinable())
	{
		command_queue.push(command{ command_type::exit });
		worker_thread->join();
	}

	if (device_id > 0)
	{
		SDL_CloseAudioDevice(device_id);
	}
}

void sound_system::start_worker_thread()
{
	worker_thread.reset(new std::thread(std::bind(&sound_system::worker_thread_entry, this)));
}

void sound_system::process_command(command command)
{
	command_queue.push(command);
}

bool sound_system::reclaim_buffer(command& command)
{
	if (can_reclaim_buffer())
	{
		auto count = buffer_command_return.pop(&command, 1);
		return count == 1;
	}

	return false;
}

void sound_system::handle_command_buffer(size_t command_byte_count, uint8_t* command_bytes)
{
	voices_mutex.lock();

	auto now = std::chrono::high_resolution_clock::now();
	auto time_micros = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count());
	float time = time_micros / 1000000.0f;
	for (int i = 0; i < command_byte_count; i++)
	{
		auto& voice = voices[get_voice(command_bytes[i])];
		uint8_t byte;
		uint8_t id;
		switch (get_command(command_bytes[i]))
		{
		case command_value::set_voice_volume:
			voice.volume = byte_to_float_fraction(command_bytes[++i]);
			voice.current_note = note::none;
			break;

		case command_value::set_voice_envelope:
			voice.attack_time = byte_to_float_time(command_bytes[++i]);
			voice.decay_time = byte_to_float_time(command_bytes[++i]);
			voice.sustain_amplitude = byte_to_float_fraction(command_bytes[++i]);
			voice.release_time = byte_to_float_time(command_bytes[++i]);
			voice.start_amplitude = byte_to_float_fraction(command_bytes[++i]);
			voice.max_life = byte_to_float_fraction(command_bytes[++i]);
			std::cout << voice.to_string();
			break;

		case command_value::set_voice_oscillator:
			byte = command_bytes[++i];
			id = get_oscillator_id(byte);
			voice.oscillators[id].type = get_oscillator_type(byte);
			voice.oscillators[id].octave = get_biased_octave(command_bytes[++i]);
			voice.oscillators[id].amplitude = byte_to_float_fraction(command_bytes[++i]);
			byte = command_bytes[++i];
			voice.oscillators[id].lfo_amplitude = get_lfo_amplitude(byte);
			voice.oscillators[id].lfo_frequency = get_lfo_frequency(byte);
			break;

		case command_value::play_note:
			byte = command_bytes[++i];
			voice.current_note = get_note(byte);
			voice.octave = get_shifted_octave(byte, 4);
			voice.time_started = time;
			voice.time_started = time + voice.max_life;
			break;

		case command_value::hold_note:
			byte = command_bytes[++i];
			voice.current_note = get_note(byte);
			voice.octave = get_shifted_octave(byte, 4);
			voice.time_started = time;
			voice.time_ended = 0;
			break;

		case command_value::release_note:
			voice.time_ended = time;
			break;

		default:
			break;
		}
	}

	voices_mutex.unlock();
}

void sound_system::worker_thread_entry()
{
	SDL_PauseAudioDevice(device_id, 0);
	start_time = std::chrono::high_resolution_clock::now();
	last_frame_time = start_time.min();
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(3));

		while (command_queue.read_available())
		{
			// Handle the commands
			command popped_commands[COMMAND_QUEUE_SIZE]{};
			auto count = command_queue.pop(popped_commands, COMMAND_QUEUE_SIZE);
			for (int i = 0; i < count; i++)
			{
				switch (popped_commands[i].type)
				{
				case command_type::exit:
					return;

				case command_type::buffer:
					// Handle the command buffer
					handle_command_buffer(popped_commands[i].command_count, popped_commands[i].command_buffer);

					if (return_mode == buffer_return_mode::return_when_done)
					{
						// When finished with a buffer, put it in the return queue, if requested
						buffer_command_return.push(popped_commands[i]);
					}
					break;
				}
			}
		}
	}
}

void sound_system::sdl_audio_handler(void* userdata, uint8_t* stream, int len)
{
	auto self = reinterpret_cast<sound_system*>(userdata);
	auto data_size_in_bytes = static_cast<size_t>(SDL_AUDIO_BITSIZE(self->device_spec.format) / 8);
	auto samples = len / data_size_in_bytes;

	auto frame_time = std::chrono::high_resolution_clock::now();
	if (self->last_frame_time > self->start_time)
	{
		frame_time = self->last_frame_time;
	}

	self->voices_mutex.lock();

	float* sample_buffer = reinterpret_cast<float*>(stream);
	for (int i = 0; i < (len / sizeof(float)); i++)
	{
		sample_buffer[i] = 0;
		auto sample_time_point = i * default_sample_duration(1);
		auto current_time_point = frame_time + sample_time_point;
		auto time_micros = (float)(std::chrono::duration_cast<std::chrono::microseconds>(current_time_point - self->start_time).count());
		float mixed_sound = 0;
		for (int voice = 0; voice < system_voices_count; voice++)
		{
			auto& current_voice = self->voices[voice];
			if (current_voice.volume <= 0.01f || current_voice.current_note == note::none)
			{
				continue;
			}

			float amplitude = envelope(time_micros, current_voice);
			if (amplitude <= 0.01f)
			{
				current_voice.current_note = note::none;
				continue;
			}

			float sound = 0;
			for (int oscillator_index = 0; oscillator_index < oscillators_per_voice; oscillator_index++)
			{
				sound += current_voice.oscillators[oscillator_index].amplitude * osc(time_micros, current_voice, oscillator_index);
			}
			mixed_sound += sound * amplitude * current_voice.volume;
		}
		sample_buffer[i] = clip(mixed_sound * 0.2f, 1.0f);
	}

	self->voices_mutex.unlock();

	auto next_frame_time = frame_time + default_sample_duration(samples);
	self->last_frame_time = std::chrono::time_point_cast<std::chrono::high_resolution_clock::duration>(next_frame_time);
}
