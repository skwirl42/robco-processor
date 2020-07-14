#pragma once

#include <stdint.h>
#include "emulator.h"
#include "Console.h"

void handle_current_syscall(emulator &emulator, Console &console);
void handle_keypress_for_syscall(emulator &emulator, int key);
