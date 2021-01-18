#include <iostream>
#include <filesystem>
#include <tuple>
#include <boost/program_options.hpp>

#if defined(APPLE)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "sound_system.hpp"
#include "exceptions.hpp"

namespace po = boost::program_options;

namespace
{
	// How many bytes will be read at a time from the sound file and passed into the sound system
	const int DEFAULT_BYTES_PER_SOUND_COMMAND_SET = 16;

	// 60000 ms per minute, divided by BPM, divided by eighth notes (assuming 4/4 time)
	const float DEFAULT_MILLISECONDS_PER_COMMAND_SET = 60000.0f / 90.0f / 2.0f;

	uint8_t harmonica_initialize_commands[] =
	{
		make_command_byte(command_value::set_voice_envelope, 0),
		0, // attack
		255, // decay
		200, // sustain volume
		25, // release time
		220, // start volume,
		255, // max life

		make_command_byte(command_value::set_voice_oscillator, 0),
		0 + (uint8_t)oscillator_type::square, // oscillator 0, square wave
		128, // octave relative to main note
		255, // oscillator amplitude
		(5 << 4) + 15, // 5Hz, 0.001 amplitude

		make_command_byte(command_value::set_voice_oscillator, 0),
		(1 << 4) + (uint8_t)oscillator_type::saw, // oscillator 1, saw wave
		129, // bias is -128, so this is +1 (relative octive)
		128, // oscillator amplitude
		(5 << 4) + 15, // 5Hz, 0.001 amplitude

		make_command_byte(command_value::set_voice_oscillator, 0),
		(2 << 4) + (uint8_t)oscillator_type::noise, // oscillator 2, noise
		130, // bias is -128, so this is +2 (relative octive)
		13, // oscillator amplitude
		0, // no LFO

		make_command_byte(command_value::set_voice_volume, 0),
		255, // full volume
	};

	uint8_t bell_initialize_commands[] =
	{
		make_command_byte(command_value::set_voice_envelope, 0),
		3, // attack
		128, // decay
		204, // sustain volume
		255, // release time
		255, // start volume,
		255, // max life

		make_command_byte(command_value::set_voice_oscillator, 0),
		0 + (uint8_t)oscillator_type::square, // oscillator 0
		128, // octave relative to main note
		255, // oscillator amplitude
		(5 << 4) + 15, // 5Hz, 0.001 amplitude

		make_command_byte(command_value::set_voice_oscillator, 0),
		(1 << 4) + (uint8_t)oscillator_type::sine, // oscillator 1
		129,
		128, // oscillator amplitude
		0,

		make_command_byte(command_value::set_voice_oscillator, 0),
		(2 << 4) + (uint8_t)oscillator_type::sine, // oscillator 2
		130,
		64, // oscillator amplitude
		0, // no LFO

		make_command_byte(command_value::set_voice_volume, 0),
		255, // full volume
	};

	std::pair<SDL_KeyCode, note> keyboard_keys[] =
	{
		{ SDLK_z, note::A },
		{ SDLK_s, note::AS },
		{ SDLK_x, note::B },
		{ SDLK_c, note::C },
		{ SDLK_f, note::CS },
		{ SDLK_v, note::D },
		{ SDLK_g, note::DS },
		{ SDLK_b, note::E },
		{ SDLK_n, note::F },
		{ SDLK_j, note::FS },
		{ SDLK_m, note::G },
		{ (SDL_KeyCode)0, note::A },
	};
}

void usage(char** argv, po::options_description* options)
{
	std::filesystem::path command_path{ argv[0] };
	std::cout << "Usage: " << command_path.filename().string() << " [options] <sound command file>" << std::endl;
	std::cout << *options << std::endl;
}

int main(int argc, char** argv)
{
	po::options_description cli_options("Allowed options");
	cli_options.add_options()
		("help,?", "output the help message")
		("list,L", "list audio devices")
		("device,D", po::value<std::string>(), "the audio device to use")
		;

	po::variables_map variables;
	po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);
	po::notify(variables);

	if (variables.count("help") > 0)
	{
		usage(argv, &cli_options);
		return 1;
	}

	auto command_buffer_size = 16;
	std::unique_ptr<uint8_t[]> command_buffer{new uint8_t[command_buffer_size]};
	char error_string[256];

	bool initialized_sdl = false;
	SDL_Window* window = nullptr;
	sound_system* sound_system_ptr = nullptr;

	auto teardown = [&]()
	{
		if (sound_system_ptr != nullptr)
		{
			delete sound_system_ptr;
		}

		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		if (initialized_sdl)
		{
			SDL_Quit();
		}
	};

	try
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0)
		{
			snprintf(error_string, 256, "SDL error: %s", SDL_GetError());
			throw basic_error() << error_message(error_string);
		}

		if (variables.count("list") > 0)
		{
			auto device_count = SDL_GetNumAudioDevices(0);
			std::cout << "Listing " << device_count << " audio devices:" << std::endl;
			const char* device_name;
			for (int i = 0; i < device_count; i++)
			{
				device_name = SDL_GetAudioDeviceName(i, 0);
				std::cout << "Device " << i << ": \"" << device_name << "\"" << std::endl;
			}

			throw basic_error() << error_message("Printed device list");
		}

		window = SDL_CreateWindow("keys", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if (window == nullptr)
		{
			snprintf(error_string, 256, "SDL error: %s", SDL_GetError());
			throw basic_error() << error_message(error_string);
		}

		initialized_sdl = true;

		if (variables.count("device") == 1)
		{
			sound_system_ptr = new sound_system(variables["device"].as<std::string>());
		}
		else
		{
			sound_system_ptr = new sound_system();
		}

		std::unique_ptr<sound_system> a_sound_system(sound_system_ptr);

		if (!a_sound_system->is_initialized())
		{
			snprintf(error_string, 256, "Sound system error: %s", a_sound_system->get_error());
			throw basic_error() << error_message(error_string);
		}

		std::cout << "Opened device " << a_sound_system->get_device_name() << std::endl;

		a_sound_system->start_worker_thread();

		command current_command{};
		current_command.command_buffer = command_buffer.get();
		current_command.type = command_type::buffer;

		current_command.command_buffer = bell_initialize_commands;
		current_command.command_count = sizeof(bell_initialize_commands);
		a_sound_system->process_command(current_command);

		current_command.command_buffer = command_buffer.get();

		bool running = true;
		SDL_Event event{};
		while (running)
		{
			if (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_QUIT:
					running = false;
					break;

				case SDL_AUDIODEVICEADDED:
					break;

				case SDL_KEYDOWN:
					for (int i = 0; keyboard_keys[i].first != 0; i++)
					{
						if (keyboard_keys[i].first == event.key.keysym.sym)
						{
							command_buffer[0] = make_command_byte(command_value::hold_note, 0);
							command_buffer[1] = (5 << 4) + ((uint8_t)keyboard_keys[i].second);
							current_command.command_count = 2;
							a_sound_system->process_command(current_command);
							break;
						}
					}
					break;

				case SDL_KEYUP:
					command_buffer[0] = make_command_byte(command_value::release_note, 0);
					current_command.command_count = 1;
					a_sound_system->process_command(current_command);
					break;
				}
			}
		}
	}
	catch (const basic_error& exception)
	{
		teardown();
		std::string const* error_text = boost::get_error_info<error_message>(exception);
		if (error_text == nullptr)
		{
			std::cerr << "Failed with unknown exception" << std::endl;
		}
		else
		{
			std::cout << "Failed with exception \"" << *error_text << "\"" << std::endl;
		}
		return -1;
	}
	catch (const std::exception& exception)
	{
		// Not a bad exception, probably just an early exit
		teardown();

		return 0;
	}
	catch (...)
	{
		teardown();

		std::cout << "Failed on an exception" << std::endl;
		return -1;
	}

	teardown();

	return 0;
}