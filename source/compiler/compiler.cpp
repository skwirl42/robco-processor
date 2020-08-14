#include "compiler_internal.hpp"

#include <stdexcept>
#include <boost/spirit/include/qi.hpp>

char lineBuffer[LINE_BUFFER_SIZE + 1];
compiler* compiler_data = nullptr;

namespace
{
    template<typename ... Args>
    std::string format_string(const char *format, Args ... args)
    {
        size_t length = snprintf(nullptr, 0, format, args ...) + 1;
        if (length <= 0)
        {
            throw std::runtime_error( "Error during string formatting." );
        }
        std::unique_ptr<char[]> buf(new char[length]);
        snprintf(buf.get(), length, format, args ...);
        
        // We don't want the '\0' inside
        return std::string(buf.get(), buf.get() + length - 1);
    }
}

compiler::compiler(const char *filename) : filename(filename)
{
    try
    {
        input_stream.reset(new std::ifstream(filename));

        if (!input_stream->is_open())
        {
            add_error(format_string("Can't open file %s", filename), CompilerError::IOError);
            return;
        }

        input_stream->unsetf(std::ios::skipws);

        input.reset(new std::string(std::istreambuf_iterator<char>(input_stream->rdbuf()), std::istreambuf_iterator<char>()));
        cpp_context.reset(new context_type(input->begin(), input->end(), filename));
    }
    catch(const std::exception& e)
    {
        add_error(format_string("Exception creating preprocessor: %s", e.what()), CompilerError::InitializationError);
    }
}

compiler::~compiler()
{
}

void compiler::add_include_dir(const char *path)
{
    cpp_context->add_include_path(path);
}

void compiler::add_system_include_dir(const char *path)
{
    cpp_context->add_sysinclude_path(path);
}

void compiler::preprocess(const char *cpp_out_name)
{
    try
    {
        boost::scoped_ptr<std::ostream> file;

        if (cpp_out_name != nullptr)
        {
            this->cpp_out_name = cpp_out_name;
            file.reset(new std::ofstream(cpp_out_name, std::ios_base::out));
        }

        auto first = cpp_context->begin();
        auto last = cpp_context->end();
        while (first != last)
        {
            (file ? *file : std::cout) << (*first).get_value();
            ++first;
        }
    }
    catch (const boost::wave::cpp_exception &e)
    {
        switch (e.get_severity())
        {
        case boost::wave::util::severity_remark:
        case boost::wave::util::severity_warning:
            std::cerr << e.description() << " in " << e.file_name() << ":" << e.line_no() << std::endl;
            break;

        default:
            std::cerr << "Preprocessing explosion " << e.description() << " in " << e.file_name() << ":" << e.line_no() << std::endl;
            break;
        }
    }
    catch (const boost::wrapexcept<boost::wave::cpplexer::lexing_exception> &e)
    {
        std::cerr << "Lexing exception " << e.description() << " in " << e.file_name() << ":" << e.line_no() << std::endl;
    }
    catch (const boost::wave::cpplexer::lexing_exception &e)
    {
        std::cerr << "Lexing exception " << e.description() << " in " << e.file_name() << ":" << e.line_no() << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void compiler::parse()
{

}

void compiler::generate()
{

}

void compiler::add_error(const char* err, CompilerError error)
{

}

void compiler::add_error(std::string err, CompilerError error)
{

}