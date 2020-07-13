#include "syscall_handlers.h"
#include "syscall.h"

typedef union
{
    uint16_t word;
    uint8_t bytes[2];
} emulator_word;

emulator_word get_top_word(emulator &emulator)
{
    emulator_word value;
    value.bytes[1] = emulator.memories.data[emulator.SP];
    value.bytes[0] = emulator.memories.data[emulator.SP+1];
    return value;
}

emulator_word pull_word(emulator &emulator)
{
    emulator_word value;
    value.bytes[1] = emulator.memories.data[emulator.SP];
    value.bytes[0] = emulator.memories.data[emulator.SP+1];
    emulator.SP += 2;
    return value;
}

char pull_char(emulator &emulator)
{
    char value = emulator.memories.data[emulator.SP];
    emulator.SP++;
    return value;
}

void push_word(emulator &emulator, emulator_word word)
{
    emulator.SP -= 2;
    emulator.memories.data[emulator.SP] = word.bytes[1];
    emulator.memories.data[emulator.SP + 1] = word.bytes[0];
}

void push_char(emulator &emulator, char character)
{
    emulator.SP--;
    emulator.memories.data[emulator.SP] = (uint8_t)character;
}

void handle_syscall_setcursor(emulator &emulator, Console &console)
{
    auto cursorY = pull_word(emulator);
    auto cursorX = pull_word(emulator);
    if (console.SetCursor(cursorX.word, cursorY.word))
    {
        push_char(emulator, 0);
    }
    else
    {
        push_char(emulator, -1);
    }
}

void handle_syscall_setch(emulator &emulator, Console &console)
{
    char character = pull_char(emulator);
    console.PrintChar(character);
}

void handle_syscall_print(emulator &emulator, Console &console)
{
    emulator_word stringSize = pull_word(emulator);
    bool newline = false;
    char *string = new char[stringSize.word + 1];
    uint16_t stringEnd = emulator.SP + 1;
    uint16_t stringPos = emulator.SP + 1 + stringSize.word;
    char currentChar;
    int x;
    for (x = 0; stringPos > stringEnd; stringPos--)
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
    emulator.SP += stringSize.word;

    delete [] string;
}

void handle_syscall_setattrc(emulator &emulator, Console &console)
{
    emulator_word attribute = get_top_word(emulator);
    console.SetAttributeAtCursor((CharacterAttribute)attribute.word);
    emulator.SP += 2;
}

void handle_syscall_setattr(emulator &emulator, Console &console)
{
    emulator_word attribute = get_top_word(emulator);
    console.SetCurrentAttribute((CharacterAttribute)attribute.word);
    emulator.SP += 2;
}

void handle_syscall_getch(emulator &emulator, Console &console)
{
    auto isBlocking = pull_char(emulator);
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
