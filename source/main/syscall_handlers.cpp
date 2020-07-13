#include "syscall_handlers.h"
#include "syscall.h"

void handle_syscall_setcursor(emulator &emulator, Console &console)
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
}

void handle_syscall_setch(emulator &emulator, Console &console)
{
    char character = pull_byte(&emulator);
    console.PrintChar(character);
}

void handle_syscall_print(emulator &emulator, Console &console)
{
    uint16_t stringSize = pull_word(&emulator);
    bool newline = false;
    char *string = new char[stringSize + 1];
    uint16_t stringEnd = emulator.SP;
    int stringPos = (int)emulator.SP + (int)stringSize - 1;
    char currentChar;
    int x;
    for (x = 0; stringPos >= stringEnd; stringPos--)
    {
        // Discard non-printables
        currentChar = emulator.memories.data[stringPos];
        if (currentChar >= ' ')
        {
            string[x++] = currentChar;
        }
        else if (currentChar == '\n' || currentChar == '\r')
        {
            // This isn't the best assumption, since they could be in the middle of the string...
            newline = true;
        }
    }

    string[x] = 0;

    if (newline)
    {
        console.PrintLine(string);
    }
    else
    {
        console.Print(string);
    }

    // Drop the string off the stack
    pop_bytes(&emulator, stringSize);

    delete [] string;
}

void handle_syscall_setattrc(emulator &emulator, Console &console)
{
    auto attribute = pull_word(&emulator);
    console.SetAttributeAtCursor((CharacterAttribute)attribute);
}

void handle_syscall_setattr(emulator &emulator, Console &console)
{
    auto attribute = pull_word(&emulator);
    console.SetCurrentAttribute((CharacterAttribute)attribute);
}

void handle_syscall_getch(emulator &emulator, Console &console)
{
    auto isBlocking = pull_byte(&emulator);
}

void handle_current_syscall(emulator &emulator, Console &console)
{
    printf("Handling syscall 0x%04x\n", emulator.current_syscall);
    switch (emulator.current_syscall)
    {
    case SYSCALL_CLEAR:
        console.Clear();
        break;

    case SYSCALL_PRINT:
        handle_syscall_print(emulator, console);
        break;

    case SYSCALL_SETCH:
        handle_syscall_setch(emulator, console);
        break;

    case SYSCALL_SETATTR:
        handle_syscall_setattr(emulator, console);
        break;

    case SYSCALL_SETATTRC:
        handle_syscall_setattrc(emulator, console);
        break;

    case SYSCALL_SETCURSOR:
        handle_syscall_setcursor(emulator, console);
        break;

    case SYSCALL_GETCH:
        handle_syscall_getch(emulator, console);
        break;

    default:
        break;
    }
}
