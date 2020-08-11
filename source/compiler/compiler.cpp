#include "compiler_internal.hpp"

char lineBuffer[LINE_BUFFER_SIZE + 1];
compiler* compiler_data = nullptr;

void compiler::add_error(const char* err, CompilerError error)
{

}