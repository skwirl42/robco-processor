#include "syscall.h"
#include "intrinsics.h"

void print_string(const char *string)
{
    const char *x = string;
    while (*x != 0)
    {
        push_byte(*x);
        x++;
    }

    push_word(x - string);
    syscall(SYSCALL_PRINT);
}

int main()
{
    int result = 5 + 3;
    print_string("Hello, world!\n");
    return result;
}
