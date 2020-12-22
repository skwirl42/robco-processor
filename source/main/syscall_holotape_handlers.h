#pragma once

#include "emulator.h"

void handle_holotape_syscall(emulator &emulator);
void insert_holotape(const char *holotape_file);
void dispose_holotape();