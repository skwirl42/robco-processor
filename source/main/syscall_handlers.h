#pragma once

#include <stdint.h>
#include "emulator.h"
#include "Console.h"

void handle_current_syscall(emulator &emulator, Console &console);
