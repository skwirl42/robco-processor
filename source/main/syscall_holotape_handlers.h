#pragma once

#include "emulator.h"

void handle_holotape_syscall(emulator &emulator);
void insert_holotape(const char *holotape_file);
bool holotape_initialized();
void dispose_holotape();