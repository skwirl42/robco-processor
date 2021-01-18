#include "assembler_visitors.hpp"

data_def_handler::data_def_handler(assembler_data_t* data) : data(data)
{

}

parser_state data_def_handler::begin(parser_state state, const rc_assembler::data_def& begin_data)
{
	if (!in_progress && state == parser_state::normal)
	{
		in_progress = true;
		spans_multiple_lines = false;
		name = (begin_data.symbol) ? begin_data.symbol.value() : "";
		data_bytes = begin_data.bytes;

		return parser_state::data_def_in_progress;
	}

	return parser_state::error;
}

parser_state data_def_handler::add(parser_state state, const rc_assembler::byte_array& bytes)
{
	if (in_progress && state == parser_state::data_def_in_progress)
	{
		data_bytes.insert(data_bytes.end(), bytes.begin(), bytes.end());
		spans_multiple_lines = true;

		return parser_state::data_def_in_progress;
	}

	return parser_state::error;
}

parser_state data_def_handler::end(parser_state state)
{
	if (in_progress && state == parser_state::data_def_in_progress)
	{
		if (data_bytes.size() == 0)
		{
			data_bytes.push_back(0);
		}

		add_data(assembler_data, name, data_bytes);

		data_bytes.clear();
		name = "";
		spans_multiple_lines = false;
		in_progress = false;

		return parser_state::normal;
	}

	return parser_state::error;
}

struct_def_handler::struct_def_handler(assembler_data_t* data) : data(data)
{
	
}

parser_state struct_def_handler::begin(parser_state state, const rc_assembler::struct_def& begin_def)
{
	if (!in_progress && state == parser_state::normal)
	{
		in_progress = true;
		name = begin_def.symbol;
		return parser_state::struct_def_in_progress;
	}
	return parser_state::error;
}

parser_state struct_def_handler::add(parser_state state, const rc_assembler::struct_member_def& member_def)
{
	if (in_progress && state == parser_state::struct_def_in_progress)
	{
		members.push_back(member_def);
		return parser_state::struct_def_in_progress;
	}

	return parser_state::error;
}

parser_state struct_def_handler::end(parser_state state)
{
	if (in_progress && state == parser_state::struct_def_in_progress)
	{
		if (members.size() == 0)
		{
			// error!
			return parser_state::error;
		}

		// apply the struct's members to the symbol table
		int current_offset = 0;
		for (auto& member : members)
		{
			auto member_name = name + "." + member.symbol;
			handle_symbol_def(data, member_name.c_str(), current_offset, SYMBOL_WORD);
			current_offset += member.size;
		}

		auto size_of_string = "sizeof(" + name + ")";
		handle_symbol_def(data, size_of_string.c_str(), current_offset, SYMBOL_WORD);

		name = "";
		in_progress = false;
		members.clear();

		return parser_state::normal;
	}
	
	return parser_state::error;
}
