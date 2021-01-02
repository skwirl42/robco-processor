#include "ConsoleSDLRenderer.h"
#include "Console.h"
#include "emulator.h"
#include "syscall.h"

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

#include "syscall_handlers.h"
#include "syscall_holotape_handlers.h"
#include "key_conversion.h"
#include "assembler.h"
#include "opcodes.h"
#include "sound_system.hpp"
#include "exceptions.hpp"
#include "console_drawer.hpp"
#include "program_options_helpers.hpp"

namespace po = boost::program_options;

namespace
{
    const char *blank_line = "                                                            ";
    enum class EmulatorState
    {
        Emulating,
        Debugging,
        Configuring,
    };

    const int configuration_console_frame_time = 16;
} // namespace

void handle_key(SDL_Keysym &keysym, emulator &emulator, Console &console)
{
    auto keycode = sdl_keycode_to_console_key(keysym);

    if (keycode != 0)
    {
        handle_keypress_for_syscall(emulator, keycode);
    }
}

void usage(char** argv, po::options_description& options)
{
    std::filesystem::path command_path{ argv[0] };
    std::cout << "Usage: " << command_path.filename().string() << " [options]" << std::endl;
    std::cout << options << std::endl;
}

int main (int argc, char **argv)
{
    std::vector<std::string> one_of_options{"source", "exec-tape"};
    std::string font_name{};
    po::options_description cli_options("Allowed options");
    cli_options.add_options()
        ("help,?", "output the help message")
        ("include,I", po::value< std::vector < std::string>>(), "directories to include when assembling source")
        ("device,D", po::value<std::string>(), "the audio device to use")
        ("font,F", po::value<std::string>(&font_name)->default_value("font/robco-termfont.png"), "font image file")
        ("source,S", po::value<std::string>(), "assembly source file to run")
        ("tape,T", po::value<std::string>(), "a file containing holotape data to be used by the emulator")
        ("exec-tape,X", "execute the first file on the tape provided")
        ;

    po::variables_map variables;
    po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);

    emulator rcEmulator;
    ConsoleSDLRenderer *renderer = nullptr;
    bool sdl_initialized = false;
    bool emulator_initialized = false;
    sound_system* synthesizer = nullptr;
    Console console(60, 24);
    Console debugConsole(60, 24);
    Console uiConsole(60, 24);

    console_drawer ui_drawer(uiConsole);
    // Draw some debug UI
    ui_drawer.add_box(box_type::single_line, fill_mode::character, 0, 0, uiConsole.GetWidth(), uiConsole.GetHeight(), '\xE6');
    ui_drawer.define_text_field("Text Field:", [&](text_field_event event, int id, const char *contents) {
        if (event == text_field_event::text_updated || event == text_field_event::enter_pressed)
        {
            std::cout << "Text field was updated with \"" << contents << "\"" << std::endl;
        }
    }, text_event_send_mode::on_enter, 3, 3, 32, "test text", true);
    int what_id = ui_drawer.define_button("What?!", 18, 19, 8, 3, [&](button_event event, int id, int old_id) {});
    int cancel_id = ui_drawer.define_button("Cancel", 34, 19, 8, 3, [&](button_event event, int id, int old_id) {});
    int ok_id = ui_drawer.define_button("OK", 50, 19, 8, 3, [&](button_event event, int id, int old_id) {
        switch (event)
        {
        case button_event::clicked:
            std::cout << "OK clicked" << std::endl;
            ui_drawer.remove_control_by_id(what_id);
            break;

        case button_event::focused:
            std::cout << "OK got focus" << std::endl;
            break;
        
        case button_event::none:
            break;
        }
    });

    auto teardown = [&]() {
        if (holotape_initialized())
        {
            dispose_holotape();
        }

        if (emulator_initialized)
        {
            dispose_emulator(&rcEmulator);
        }

        if (synthesizer != nullptr)
        {
            delete synthesizer;
        }

        if (renderer != nullptr)
        {
            delete renderer;
        }

        if (sdl_initialized)
        {
            SDL_Quit();
        }
    };

    try
    {
        po::notify(variables);
        conflicting_options(variables, "source", "exec-tape");
        option_dependency(variables, "exec-tape", "tape");
        option_dependency(variables, "include", "source");

        one_of_options_required(variables, one_of_options);

        if (variables.count("help") > 0)
        {
            usage(argv, cli_options);
            return 1;
        }

        if (init_emulator(&rcEmulator, ARCH_ORIGINAL) != NO_ERROR)
        {
            std::cerr << "Emulator error" << std::endl;
            teardown();
            return -1;
        }

        emulator_initialized = true;

        //print_opcode_entries();

        if (variables.count("tape") != 0)
        {
            insert_holotape(variables["tape"].as<std::string>().c_str());
        }

        if (variables.count("source") > 0)
        {
            const char* sample_file = variables["source"].as<std::string>().c_str();

            const char** paths = new const char* [variables.count("include") + 1];

            for (int i = 0; i < variables.count("include"); i++)
            {
                paths[i] = variables["include"].as<std::vector<std::string>>()[i].c_str();
            }

            paths[variables.count("include")] = 0;

            assembler_data_t *assembled_data;
            assemble(sample_file, paths, nullptr, None, &assembled_data);

            delete[] paths;

            if (get_error_buffer_size(assembled_data) > 0)
            {
                std::cerr << get_error_buffer(assembled_data) << std::endl;
                teardown();
                return -1;
            }

            auto apply_result = apply_assembled_data_to_buffer(assembled_data, rcEmulator.memories.data);

            if (apply_result != ASSEMBLER_SUCCESS)
            {
                std::cerr << "Failed to properly assemble the target " << sample_file << std::endl;
                teardown();
                return -1;
            }

            auto exec_address_result = get_starting_executable_address(assembled_data, &rcEmulator.PC);

            if (exec_address_result != ASSEMBLER_SUCCESS)
            {
                std::cerr << "Couldn't get an executable address from the assembled target " << sample_file << std::endl;
                teardown();
                return -1;
            }
        }
        else if (variables.count("exec-tape") > 0)
        {
            if (variables.count("exec-tape") > 0)
            {
                rcEmulator.current_syscall = SYSCALL_EXECUTE;
                handle_current_syscall(rcEmulator, console, synthesizer);
                if (rcEmulator.SP > 0)
                {
                    // The stack should be empty after running the execute syscall, if it succeeded.
                    // This means there was an error executing from tape and the error number is on the stack
                    auto error_code = rcEmulator.memories.user_stack[0];
                    std::cerr << "Failed to execute from tape " << variables["tape"].as<std::string>() << " with error code " << (int)error_code << std::endl;
                    teardown();
                    return -1;
                }
            }
        }
        else
        {
            auto options_list = boost::join(one_of_options, ", ");
            std::cerr << "No executable was found to start the emulator. You must specify one of these options: (" << options_list << ")" << std::endl;
            usage(argv, cli_options);
            teardown();
            return -1;
        }
        
        auto result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (result != 0)
        {
            std::cerr << "Failed to initialize SDL (" << SDL_GetError() << ")" << std::endl;
            teardown();
            return -1;
        }

        sdl_initialized = true;

        if (variables.count("device") > 0)
        {
            synthesizer = new sound_system(variables["device"].as<std::string>());
        }
        else
        {
            synthesizer = new sound_system();
        }

        if (synthesizer->is_initialized())
        {
            synthesizer->start_worker_thread();
        }
        else
        {
            std::cerr << "Failed to initialize sound system with error \"" << synthesizer->get_error() << "\"" << std::endl;
            teardown();
            return -1;
        }

        if (variables.count("font") > 0)
        {
            auto fontfilename = font_name.c_str();
            // Format of the font file is 16 chars wide, 8 chars tall
            renderer = new ConsoleSDLRenderer(fontfilename, 480, 320, 0xFF00FF00, 0xFF000000, 16, 16, 100);
            renderer->Clear();
        }
        else
        {
            std::cerr << "Missing argument: font filename" << std::endl;
            usage(argv, cli_options);
            teardown();
            return -1;
        }

        if (renderer != nullptr)
        {
            bool done = false;
            bool emulate = true;
            SDL_Event event;
            int frame = 0;
            opcode_entry_t* executed_opcode = nullptr;
            int key_buffer_size = 0;
            const uint8_t *key_buffer = nullptr;

            int debugging_lines_start = console.GetHeight() - DEBUGGING_BUFFER_COUNT;
            char *debugging_buffers[DEBUGGING_BUFFER_COUNT]{};
            for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
            {
                debugging_buffers[i] = new char[LINE_BUFFER_SIZE + 1];
            }

            EmulatorState emulator_state = EmulatorState::Emulating;
            if (emulator_state == EmulatorState::Debugging)
            {
                rcEmulator.current_state = DEBUGGING;
            }

            while (!done)
            {
                inst_result_t result = SUCCESS;

                if (emulator_state == EmulatorState::Debugging)
                {
                    get_debug_info(&rcEmulator, debugging_buffers);
                }

                if (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_QUIT)
                    {
                        done = true;
                    }
                    else if (event.type == SDL_KEYDOWN)
                    {
                        if (event.key.keysym.sym == SDLK_F6)
                        {
                            if (emulator_state == EmulatorState::Configuring)
                            {
                                emulator_state = (rcEmulator.current_state == DEBUGGING) ? EmulatorState::Debugging : EmulatorState::Emulating;
                            }
                            else
                            {
                                emulator_state = EmulatorState::Configuring;
                            }
                        }
                        if (event.key.keysym.sym == SDLK_F5)
                        {
                            if (emulator_state == EmulatorState::Debugging)
                            {
                                emulator_state = EmulatorState::Emulating;
                                if (rcEmulator.current_state == DEBUGGING)
                                {
                                    rcEmulator.current_state = RUNNING;
                                }
                            }
                        }
                        else if (emulator_state == EmulatorState::Configuring)
                        {
                            bool has_shift = event.key.keysym.mod & KMOD_LSHIFT || event.key.keysym.mod & KMOD_RSHIFT;
                            if (has_shift)
                            {
                                int key = sdl_keycode_to_console_key(event.key.keysym.sym, has_shift);
                                if (key == 0)
                                {
                                    ui_drawer.handle_key(event.key.keysym.sym);
                                }
                                else
                                {
                                    ui_drawer.handle_key(key);
                                }
                            }
                            else
                            {
                                ui_drawer.handle_key(event.key.keysym.sym);
                            }
                        }
                        else
                        {
                            handle_key(event.key.keysym, rcEmulator, console);
                        }
                    }
                    else if (event.type == SDL_MOUSEBUTTONDOWN)
                    {
                        auto mouse_button_event = event.button;

                        if (mouse_button_event.button == SDL_BUTTON_LEFT)
                        {
                            if (emulator_state == EmulatorState::Configuring)
                            {
                                // TODO: Handle mouse clicks
                            }
                            else
                            {
                                if (rcEmulator.current_state == DEBUGGING)
                                {
                                    rcEmulator.current_state = RUNNING;
                                }

                                emulator_state = EmulatorState::Debugging;
                            }
                        }
                    }
                }

                key_buffer = SDL_GetKeyboardState(&key_buffer_size);
                for (int i = 0; i < key_buffer_size; i++)
                {
                    int console_keycode = sdl_scancode_to_console_key((SDL_Scancode)i);
                    if (key_buffer[i] && console_keycode > 0)
                    {
                        // TODO: How to store the info for the emulated program to access?
                        // Put into a buffer for a SYSCALL to put into emulated memory?
                        // Drop them on the stack from a SYSCALL?
                        // Explicitly set a memory location in the emulator?
                    }
                }

                bool show_screen_when_debugging = SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT);
                if (emulator_state == EmulatorState::Configuring)
                {
                    // Pause for a bit
                    auto wait_time = std::chrono::microseconds(configuration_console_frame_time);
                    auto end_time = wait_time + std::chrono::high_resolution_clock::now();
                    while (std::chrono::high_resolution_clock::now() < end_time)
                    {
                        // a frame of spinning isn't gonna hurt anything
                    }

                    ui_drawer.draw();

                    int previousBlinkFrames = renderer->GetCursorBlinkFrames();
                    if (!ui_drawer.is_cursor_enabled())
                    {
                        renderer->SetCursorBlinkFrames(0);
                    }

                    renderer->Render(&uiConsole, frame++);
                    renderer->SetCursorBlinkFrames(previousBlinkFrames);
                }
                else if (!show_screen_when_debugging && (emulator_state == EmulatorState::Debugging || rcEmulator.current_state == DEBUGGING))
                {
                    debugConsole.Clear();
                    for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
                    {
                        debugConsole.PrintLineAt(debugging_buffers[i], 2, debugging_lines_start + i);
                    }
                    renderer->Render(&debugConsole, frame++);
                }
                else
                {
                    if (rcEmulator.graphics_mode.enabled)
                    {
                        renderer->Render(&rcEmulator);
                    }
                    else
                    {
                        renderer->Render(&console, frame++);
                    }
                }

                if (emulator_state != EmulatorState::Configuring && emulator_can_execute(&rcEmulator))
                {
                    const int max_cycles = 10000;
                    int current_cycle = 0;
                    do
                    {
                        result = execute_instruction(&rcEmulator, &executed_opcode);
                        if (executed_opcode->cycles > 0)
                        {
                            current_cycle += executed_opcode->cycles;
                            auto cycle_time = std::chrono::microseconds(executed_opcode->cycles);
                            auto end_time = cycle_time + std::chrono::high_resolution_clock::now();
                            while (std::chrono::high_resolution_clock::now() < end_time)
                            {
                                // max_cycles microseconds of spinning isn't gonna hurt anything
                            }
                        }
                        else
                        {
                            current_cycle++;
                        }

                        if (emulator_state == EmulatorState::Debugging)
                        {
                            break;
                        }
                    } while (result == SUCCESS && current_cycle <= max_cycles);

                    if (result == EXECUTE_SYSCALL)
                    {
                        handle_current_syscall(rcEmulator, console, synthesizer);
                    }
                    else if (result == ILLEGAL_INSTRUCTION)
                    {
                        std::cerr << "Emulation failed with an illegal instruction" << std::endl;
                        emulate = false;
                    }

                    if (emulator_state == EmulatorState::Debugging && rcEmulator.current_state != WAITING)
                    {
                        rcEmulator.current_state = DEBUGGING;
                    }
                }
            }

            for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
            {
                delete [] (debugging_buffers[i]);
            }
        }
    }
    catch (const basic_error& error)
    {
		std::string const* error_text = boost::get_error_info<error_message>(error);
        if (error_text == nullptr)
        {
            std::cerr << "Unknown error occurred" << std::endl;
        }
        else
        {
            std::cerr << "Error: " << *error_text << std::endl;
        }
        teardown();
        return -1;
    }
    catch (po::required_option& exception)
    {
        std::cerr << "Option " << exception.get_option_name() << " is required" << std::endl;
        usage(argv, cli_options);
        teardown();
        return -1;
    }
    catch(const std::logic_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        usage(argv, cli_options);
        teardown();
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        teardown();
        return 0;
    }

    teardown();
    return 0;
}