#include "syscall_handlers.h"
#include "syscall.h"
#include "opcodes.h"
#include "graphics.h"

#include <deque>

namespace
{
    std::deque<int> character_queue;
}

execution_state_t handle_syscall_setcursor(emulator &emulator, Console &console)
{
    auto cursorY = pull_word(&emulator);
    auto cursorX = pull_word(&emulator);
    if (console.SetCursor(cursorX, cursorY))
    {
        push_byte(&emulator, 0);
    }
    else
    {
        push_byte(&emulator, 255);
    }
    return RUNNING;
}

execution_state_t handle_syscall_getcursor(emulator &emulator, Console &console)
{
    int cursorX;
    int cursorY;
    console.GetCursor(cursorX, cursorY);
    push_word(&emulator, cursorX);
    push_word(&emulator, cursorY);
    return RUNNING;
}

execution_state_t handle_syscall_setch(emulator &emulator, Console &console)
{
    char character = pull_byte(&emulator);
    console.PrintChar(character);
    return RUNNING;
}

execution_state_t handle_syscall_print(emulator &emulator, Console &console)
{
    uint16_t stringSize = pull_word(&emulator);
    bool newline = false;
    char *string = new char[stringSize + 1];
    char currentChar;
    int x;
    int bytesLeft = stringSize;
    for (x = stringSize - 1; x >= 0 && bytesLeft > 0; bytesLeft--)
    {
        // Discard non-printables
        currentChar = pull_byte(&emulator);
        if (currentChar >= ' ')
        {
            string[x--] = currentChar;
        }
        else if (currentChar == '\n' || currentChar == '\r')
        {
            // This isn't the best assumption, since they could be in the middle of the string...
            newline = true;
        }
    }

    string[stringSize] = 0;

    if (x >= 0)
    {
        for (int i = 0; i < stringSize; i++)
        {
            string[i] = string[i + x + 1];
        }
    }

    if (newline)
    {
        console.PrintLine(string);
    }
    else
    {
        console.Print(string);
    }

    delete [] string;
    return RUNNING;
}

execution_state_t handle_syscall_setattrc(emulator &emulator, Console &console)
{
    auto attribute = pull_word(&emulator);
    console.SetAttributeAtCursor((CharacterAttribute)attribute);
    return RUNNING;
}

execution_state_t handle_syscall_setattr(emulator &emulator, Console &console)
{
    auto attribute = pull_word(&emulator);
    console.SetCurrentAttribute((CharacterAttribute)attribute);
    return RUNNING;
}

execution_state_t handle_syscall_getch(emulator &emulator, Console &console)
{
    auto isBlocking = pull_byte(&emulator);

    if (character_queue.size() > 0)
    {
        auto word = character_queue.front();
        uint16_t uword = *((uint16_t*)&word);
        push_word(&emulator, uword);
        character_queue.pop_front();
    }
    else
    {
        if (isBlocking)
        {
            return WAITING;
        }
        else
        {
            push_word(&emulator, 0);
        }
    }

    return RUNNING;
}

union mode_byte
{
    uint8_t byte;
    graphics_mode_t mode;
};

execution_state_t handle_syscall_graphicstart(emulator& emulator, Console& console)
{
    mode_byte mode;
    mode.byte = pull_byte(&emulator);
    auto graphics_begin = emulator.X;
    auto memory_required = graphics_mem_size_for_mode(mode.mode);

    if ((graphics_begin + memory_required) > DATA_SIZE)
    {
        push_byte(&emulator, GRAPHICS_ERROR_SPACE_TOO_SMALL);
    }
    else if (mode.mode.depth > EIGHT_BITS_PER_PIXEL || mode.mode.resolution > RES_480x320)
    {
        push_byte(&emulator, GRAPHICS_ERROR_UNSUPPORTED_MODE);
    }
    else
    {
        mode.mode.enabled = true;
        emulator.graphics_mode = mode.mode;
        emulator.graphics_start = graphics_begin;
        push_byte(&emulator, GRAPHICS_ERROR_OK);
    }

    return RUNNING;
}

execution_state_t handle_syscall_graphicend(emulator& emulator, Console& console)
{
    emulator.graphics_mode.enabled = 0;
    return RUNNING;
}

void handle_current_syscall(emulator &emulator, Console &console)
{
    // printf("Handling syscall 0x%04x\n", emulator.current_syscall);
    execution_state_t nextState = RUNNING;
    switch (emulator.current_syscall)
    {
    case SYSCALL_CLEAR:
        console.Clear();
        break;

    case SYSCALL_PRINT:
        nextState = handle_syscall_print(emulator, console);
        break;

    case SYSCALL_SETCH:
        nextState = handle_syscall_setch(emulator, console);
        break;

    case SYSCALL_SETATTR:
        nextState = handle_syscall_setattr(emulator, console);
        break;

    case SYSCALL_SETATTRC:
        nextState = handle_syscall_setattrc(emulator, console);
        break;

    case SYSCALL_SETCURSOR:
        nextState = handle_syscall_setcursor(emulator, console);
        break;

    case SYSCALL_GETCURSOR:
        nextState = handle_syscall_getcursor(emulator, console);
        break;

    case SYSCALL_GETCH:
        nextState = handle_syscall_getch(emulator, console);
        break;

    case SYSCALL_GRAPHICSTART:
        nextState = handle_syscall_graphicstart(emulator, console);
        break;

    case SYSCALL_GRAPHICEND:
        nextState = handle_syscall_graphicend(emulator, console);
        break;

    case SYSCALL_EXIT:
        nextState = FINISHED;

    default:
        fprintf(stderr, "Unimplemented syscall (0x%04x)\n", emulator.current_syscall);
        break;
    }

    if (nextState != WAITING)
    {
        emulator.current_syscall = SYSCALL_NONE;
    }

    emulator.current_state = nextState;
}

void handle_keypress_for_syscall(emulator &emulator, int key)
{
    if (emulator.current_state == WAITING && emulator.current_syscall == SYSCALL_GETCH)
    {
        machine_word_t word;
        word.sword = key;
        push_word(&emulator, word.uword);
        emulator.current_syscall = SYSCALL_NONE;
        emulator.current_state = RUNNING;
    }
    else
    {
        character_queue.push_back(key);
    }
}