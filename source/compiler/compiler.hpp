#pragma once

enum class CompilerError
{
	NoError,
	SyntaxError,
	ValueOOB,
};

class compiler
{
public:
	void add_error(const char* err, CompilerError error);
};