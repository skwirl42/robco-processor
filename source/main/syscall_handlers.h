#pragma once

#include <stdint.h>
#include "emulator.h"
#include "Console.h"

class sound_system;

void handle_current_syscall(emulator &emulator, Console &console, sound_system* synthesizer = nullptr);
void handle_keypress_for_syscall(emulator &emulator, int key);
