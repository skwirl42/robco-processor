#include "compiler.hpp"

#include <boost/scoped_ptr.hpp>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Provide a filename, please!\n");
		return -1;
	}

	boost::scoped_ptr<compiler> compiler_ptr(new compiler(argv[1]));
	if (compiler_ptr->is_valid())
	{
		if (argc >= 3)
		{
			compiler_ptr->preprocess(argv[2]);
		}
		else
		{
			compiler_ptr->preprocess();
		}
	}
	
	return 0;
}