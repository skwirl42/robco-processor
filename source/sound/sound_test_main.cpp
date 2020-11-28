#include <iostream>
#include <filesystem>
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
}

void usage(char** argv, po::options_description* options)
{
	std::filesystem::path command_path{ argv[0] };
	std::cout << "Usage: " << command_path.filename().string() << " [options] <sound command file>" << std::endl;
	std::cout << *options << std::endl;
}

int main(int argc, char** argv)
{
	size_t bytes_per_command = 0;
	size_t preamble_bytes = 0;
	float millis_per_command = 0;
	po::options_description cli_options("Allowed options");
	cli_options.add_options()
		("help,?", "output the help message")
		("preamble,P", po::value<size_t>(&preamble_bytes), "the number of command bytes to process before beginning to time command sets")
		("bytes,B", po::value<size_t>(&bytes_per_command)->default_value(DEFAULT_BYTES_PER_SOUND_COMMAND_SET), "the number of bytes to read per sound command set to the sound system")
		("time,T", po::value<float>(&millis_per_command)->default_value(DEFAULT_MILLISECONDS_PER_COMMAND_SET), "the number of milliseconds to wait per sound command set")
		;

	po::options_description options;
	options.add_options()
		("sound-command-file", po::value<std::string>()->required(), "a binary file containing sound commands")
		;
	options.add(cli_options);

	po::positional_options_description positionals;
	positionals.add("sound-command-file", 1);

	po::variables_map variables;
	po::store(po::command_line_parser(argc, argv).options(options).positional(positionals).run(), variables);

	try
	{
		po::notify(variables);
	}
	catch (po::required_option& required_option)
	{
		std::cout << "Command file not specified" << std::endl << std::endl;
		usage(argv, &cli_options);
		return -1;
	}

	if (variables.count("help") > 0)
	{
		usage(argv, &cli_options);
		return 1;
	}

	auto &command_file_name = variables["sound-command-file"].as<std::string>();
	std::filesystem::path command_file_path(command_file_name);

	if (!std::filesystem::exists(command_file_path))
	{
		std::cout << "Please specify a valid command file" << std::endl << std::endl;
		usage(argv, &cli_options);
		return -1;
	}

	auto command_file = fopen(command_file_name.c_str(), "r");

	if (command_file == 0)
	{
		std::cout << "Failed to open command file " << command_file_name << std::endl;
		return -1;
	}

	auto command_buffer_size = preamble_bytes > bytes_per_command ? preamble_bytes : bytes_per_command;
	uint8_t* command_buffer = new uint8_t[command_buffer_size]{};

	try
	{
		sound_system a_sound_system;

		if (!a_sound_system.is_initialized())
		{
			throw std::exception(a_sound_system.get_error());
		}

		a_sound_system.start_worker_thread();
		if (variables.count(std::string("preamble")) > 0)
		{
			if (fread(command_buffer, 1, preamble_bytes, command_file) != preamble_bytes)
			{
				throw std::exception("Failed to read the preamble");
			}

			command preamble_command{command_type::buffer, preamble_bytes, command_buffer};
			a_sound_system.process_command(preamble_command);
		}

		command current_command;
		while (feof(command_file) == 0)
		{
			std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(millis_per_command));

			auto size_read = fread(command_buffer, 1, bytes_per_command, command_file);
			if (size_read > 0)
			{
				current_command = { command_type::buffer, size_read, command_buffer };
				a_sound_system.process_command(current_command);
			}
			else
			{
				throw std::exception("Failed to read from command file");
			}
		}
	}
	catch (const std::exception& exception)
	{
		delete[] command_buffer;
		fclose(command_file);

		std::cout << "Failed with exception \"" << exception.what() << "\"" << std::endl;

		return -1;
	}
	catch (...)
	{
		delete[] command_buffer;
		fclose(command_file);

		std::cout << "Failed on an exception" << std::endl;
		return -1;
	}

	delete[] command_buffer;
	fclose(command_file);

	return 0;
}