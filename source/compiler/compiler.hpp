#pragma once

#include <iostream>
#include <string>

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

#include <boost/scoped_ptr.hpp>

enum class CompilerError
{
	NoError,
	SyntaxError,
	ValueOOB,
	IOError,
	InitializationError,
};

typedef boost::wave::cpplexer::lex_iterator<boost::wave::cpplexer::lex_token<> > lex_iterator_type;
typedef boost::wave::context<std::string::iterator, lex_iterator_type> context_type;

class compiler
{
public:
	compiler(const char *filename);
	~compiler();

	void add_include_dir(const char *path);
	void add_system_include_dir(const char *path);

	void preprocess(const char *cpp_out_name = nullptr);
	void parse(std::string &input);
	void generate();
	void add_error(const char* err, CompilerError error);
	void add_error(std::string err, CompilerError error);

	bool is_valid() { return input_stream && input && cpp_context; }

private:
	boost::scoped_ptr<std::ifstream> input_stream;
	boost::scoped_ptr<std::string> input;
	boost::scoped_ptr<context_type> cpp_context;
	const char *filename;
	const char *cpp_out_name;
};