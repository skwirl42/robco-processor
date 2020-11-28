#include "sound_system.hpp"

#include <functional>

namespace
{
	const int COMMAND_QUEUE_SIZE = 16;
	const int SAMPLES_COUNT = 512;
	const int DEFAULT_SAMPLE_RATE = 48000;
	const int CD_SAMPLE_RATE = 44100;
	const float PI = std::acos(-1);

	typedef std::chrono::duration<long, std::ratio<1, DEFAULT_SAMPLE_RATE>> default_sample_duration;
	typedef std::chrono::duration<long, std::ratio<1, CD_SAMPLE_RATE>> cd_sample_duration;

	float scale(const float octave, const note note)
	{
		float note_at_octave = octave * 12 + (int)note;
		return 8 * pow(1.059463f, note_at_octave);
	}

	float envelope(const float time, voice& voice)
	{
		float amplitude = 0;
		float release_amplitude = 0;

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

		return amplitude;
	}

	float osc(const float time, voice& voice, int oscillator_id, float custom = 50.0)
	{
		auto& oscillator = voice.oscillators[oscillator_id];
		auto octave = voice.octave + oscillator.octave;
		auto hertz = scale(octave, voice.current_note);
		auto vel = hertz * 2.0f * PI;
		auto lfoVel = oscillator.lfo_frequency * 2.0f * PI;
		float frequency = vel * time + oscillator.lfo_amplitude * hertz * (sin(lfoVel * time));

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

sound_system::sound_system(buffer_return_mode return_mode, std::optional<std::string> desired_device)
	: voices{},
	  command_queue(COMMAND_QUEUE_SIZE), buffer_command_return(COMMAND_QUEUE_SIZE), worker_thread(), 
	  device_spec{}, sample_buffer(nullptr), error(nullptr),
	  return_mode(return_mode), device_id(0), current_voice(0), initialized(false)
{
	std::string device {};
	if (desired_device.has_value())
	{
		device = desired_device.value();
	}
	else
	{
		auto audio_device_count = SDL_GetNumAudioDevices(0);
		if (audio_device_count > 0)
		{
			// We'll be feeding data to the device using SDL_QueueAudio,
			// since we need to listen for system commands
			device = SDL_GetAudioDeviceName(0, 0);
			SDL_AudioSpec want{ DEFAULT_SAMPLE_RATE, AUDIO_F32, 1, 0, SAMPLES_COUNT, 0, 0, nullptr, nullptr };
			
			device_id = SDL_OpenAudioDevice(device.c_str(), 0, &want, &device_spec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
			if (device_id == 0)
			{
				error = SDL_GetError();
			}
			else
			{
				initialized = true;

				sample_buffer_size = (static_cast<size_t>(SDL_AUDIO_BITSIZE(device_spec.format) / 8)) * device_spec.samples;
				sample_buffer = new float[device_spec.samples];
			}
		}
		else
		{
			error = "Couldn't find any non-capture audio devices";
		}
	}
}

sound_system::~sound_system()
{
	if (worker_thread->joinable())
	{
		command_queue.push(command{ command_type::exit });
		worker_thread->join();
	}

	if (device_id > 0)
	{
		SDL_CloseAudioDevice(device_id);
	}

	delete[] sample_buffer;
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
	auto now = std::chrono::high_resolution_clock::now();
	auto time_micros = (float)((now - start_time).count()) / (float)std::micro::den;
	for (int i = 0; i < command_byte_count; i++)
	{
		auto& voice = voices[get_voice(command_bytes[i])];
		auto& oscillator = voice.oscillators[0];
		uint8_t byte;
		switch (get_command(command_bytes[i]))
		{
		case command_value::set_voice_volume:
			voice.volume = byte_to_float_fraction(command_bytes[++i]);
			break;

		case command_value::set_voice_envelope:
			voice.attack_time = byte_to_float_time_micros(command_bytes[++i]);
			voice.decay_time = byte_to_float_time_micros(command_bytes[++i]);
			voice.sustain_amplitude = byte_to_float_fraction(command_bytes[++i]);
			voice.release_time = byte_to_float_time_micros(command_bytes[++i]);
			voice.start_amplitude = byte_to_float_fraction(command_bytes[++i]);
			voice.max_life = byte_to_float_time_micros(command_bytes[++i]);
			break;

		case command_value::set_voice_oscillator:
			byte = command_bytes[++i];
			oscillator = voice.oscillators[get_oscillator(byte)];
			oscillator.type = get_oscillator_type(byte);
			oscillator.octave = get_biased_octave(command_bytes[++i]);
			oscillator.amplitude = byte_to_float_fraction(command_bytes[++i]);
			byte = command_bytes[++i];
			oscillator.lfo_amplitude = get_lfo_amplitude(byte);
			oscillator.lfo_frequency = get_lfo_frequency(byte);
			break;

		case command_value::play_note:
			byte = command_bytes[++i];
			voice.current_note = get_note(byte);
			voice.octave = get_shifted_octave(byte, 4);
			voice.time_started = time_micros;
			voice.time_started = time_micros + voice.max_life;
			break;

		case command_value::hold_note:
			byte = command_bytes[++i];
			voice.current_note = get_note(byte);
			voice.octave = get_shifted_octave(byte, 4);
			voice.time_started = time_micros;
			voice.time_ended = 0;
			break;

		case command_value::release_note:
			voice.time_ended = time_micros;
			break;
		}
	}
}

void sound_system::worker_thread_entry()
{
	while (1)
	{
		SDL_PauseAudioDevice(device_id, 0);
		start_time = std::chrono::high_resolution_clock::now();
		while (command_queue.read_available())
		{
			auto frame_time = std::chrono::high_resolution_clock::now();

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

			// Generate a sound buffer's worth of samples
			for (int i = 0; i < device_spec.samples; i++)
			{
				auto current_time_point = std::chrono::time_point_cast<std::chrono::microseconds>(frame_time + i * default_sample_duration(1));
				auto time_micros = (float)((current_time_point - start_time).count()) / (float)std::micro::den;
				float mixed_sound = 0;
				for (int voice = 0; voice < system_voices_count; voice++)
				{
					auto& current_voice = voices[voice];
					float amplitude = envelope(time_micros, current_voice);
					float sound = 0;
					for (int oscillator = 0; oscillator < oscillators_per_voice; oscillator++)
					{
						sound += current_voice.oscillators[oscillator].amplitude * osc(time_micros, current_voice, oscillator);
					}
					mixed_sound += sound * amplitude * current_voice.volume;
				}
				sample_buffer[i] = mixed_sound * 0.2f;
			}
			SDL_QueueAudio(device_id, sample_buffer, sample_buffer_size);

			auto final_time = frame_time + device_spec.samples * default_sample_duration(1);
			auto now = std::chrono::high_resolution_clock::now();
			if (final_time > now)
			{
				std::this_thread::sleep_for(final_time - now);
			}
		}
	}
}
