#include <iostream>
#include <filesystem>
#include <tuple>
#include <boost/program_options.hpp>
#include <SDL.h>

#include "sound_system.hpp"

namespace po = boost::program_options;

namespace
{
	// How many bytes will be read at a time from the sound file and passed into the sound system
	const int DEFAULT_BYTES_PER_SOUND_COMMAND_SET = 16;

	// 60000 ms per minute, divided by BPM, divided by eighth notes (assuming 4/4 time)
	const float DEFAULT_MILLISECONDS_PER_COMMAND_SET = 60000.0f / 90.0f / 2.0f;

	std::pair<SDL_KeyCode, note> keyboard_keys[] =
	{
		{ SDLK_z, note::A },
		{ SDLK_s, note::AS },
		{ SDLK_x, note::B },
		{ SDLK_c, note::C },
		{ SDLK_f, note::CS },
		{ SDLK_v, note::D },
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
	uint8_t* command_buffer = new uint8_t[command_buffer_size]{};
	char error_string[256];

	bool initialized_sdl = false;
	SDL_Window* window = nullptr;
	const char* bad_exception_text = nullptr;
	try
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0)
		{
			snprintf(error_string, 256, "SDL error: %s", SDL_GetError());
			bad_exception_text = error_string;
			throw std::bad_exception();
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

			throw std::exception("Printed device list");
		}

		window = SDL_CreateWindow("keys", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if (window == nullptr)
		{
			snprintf(error_string, 256, "SDL error: %s", SDL_GetError());
			bad_exception_text = error_string;
			throw std::bad_exception();
		}

		initialized_sdl = true;

		sound_system* sound_system_ptr = nullptr;
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
			bad_exception_text = error_string;
			throw std::bad_exception();
		}

		a_sound_system->start_worker_thread();

		command current_command{};
		current_command.command_buffer = command_buffer;
		current_command.type = command_type::buffer;

		uint8_t initialize_commands[] =
		{
			make_command_byte(command_value::set_voice_envelope, 0),
			100, // attack
			10, // decay
			200, // sustain volume
			200, // release time
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

		current_command.command_buffer = initialize_commands;
		current_command.command_count = sizeof(initialize_commands);
		a_sound_system->process_command(current_command);

		current_command.command_buffer = command_buffer;

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

				case SDL_MOUSEBUTTONDOWN:
					std::cout << "mouse boop" << std::endl;
					break;
				}
			}
		}
	}
	catch (const std::bad_exception& exception)
	{
		delete[] command_buffer;

		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		if (initialized_sdl)
		{
			SDL_Quit();
		}

		std::cout << "Failed with exception \"" << exception.what() << "\"" << std::endl;
		return -1;
	}
	catch (const std::exception& exception)
	{
		delete[] command_buffer;

		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		if (initialized_sdl)
		{
			SDL_Quit();
		}

		// Not a bad exception
		return 0;
	}
	catch (...)
	{
		delete[] command_buffer;

		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		if (initialized_sdl)
		{
			SDL_Quit();
		}

		std::cout << "Failed on an exception" << std::endl;
		return -1;
	}

	delete[] command_buffer;

	if (window != nullptr)
	{
		SDL_DestroyWindow(window);
	}

	if (initialized_sdl)
	{
		SDL_Quit();
	}

	return 0;
}