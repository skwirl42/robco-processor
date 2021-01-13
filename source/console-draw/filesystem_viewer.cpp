#include "filesystem_viewer.hpp"

#include <iostream>

#include "util.hpp"
#include "Console.h"

namespace
{
	const int min_browser_width = 30;
	const int min_info_panel_width = 10;
	const int max_info_panel_width = 15;
	const size_t pre_ellipsis_char_count = 3;
}

filesystem_viewer::filesystem_viewer(filesystem_viewer::init_params& params)
	: params(params), current_path(std::filesystem::absolute(params.starting_path)), drawable(params.bounds)
{
	// The extra 2 in here is for the border box
	if (bounds.width < min_browser_width + min_info_panel_width + 2)
	{
		// throw
	}

	if (!std::filesystem::exists(current_path) || !std::filesystem::is_directory(current_path))
	{
		// throw
	}

	determine_current_directory_entries();
}

void filesystem_viewer::draw(drawer* drawer)
{
	drawer->draw_box(box_type::double_line, fill_mode::clear, bounds);

	int remaining_width = bounds.width - 2 - min_browser_width;

	if (remaining_width > min_browser_width)
	{
		int extra_max = max_info_panel_width - min_info_panel_width;
		int extra_info_panel_width = min(remaining_width - min_browser_width, extra_max);
		remaining_width += extra_info_panel_width;
	}

	int browser_width = bounds.width - 2 - remaining_width;

	// Draw the divider as a single vertical line that joins up to the overall box frame
	int divider_x = bounds.x + bounds.width - remaining_width + 1;
	drawer->draw_text("\xD1", divider_x, bounds.y, CharacterAttribute::None);
	drawer->set_rect('\xB3', rect{ divider_x, bounds.y + 1, 1, bounds.height - 2 });
	drawer->draw_text("\xCF", divider_x, bounds.y + bounds.height - 1, CharacterAttribute::None);

	auto current_path_string = current_path.string();
	if (current_path_string.size() > remaining_width)
	{
		auto excess_size = current_path_string.size() - remaining_width;
		current_path_string = current_path_string.substr(excess_size + 3 + 2 + 1);
		current_path_string = std::string("...") + current_path_string;
	}
	drawer->draw_text(current_path_string.c_str(), bounds.x + 3, bounds.y, CharacterAttribute::Inverted);

	if (current_directory_entries.size() > 0)
	{
		int filename_x = bounds.x + 2;
		int last_filename_y = bounds.y + bounds.height - 3;
		int first_y = bounds.y + 2;
		int file_display_count = bounds.height - 4;

		if (file_display_count < current_directory_entries.size() && current_selection > file_display_count - 1)
		{
			int first_entry = current_selection - file_display_count + 1;
			for (size_t i = 0; i < file_display_count; i++)
			{
				auto& entry = current_directory_entries[i + first_entry];
				drawer->draw_text(get_current_relative_path(entry).c_str(), filename_x, i + first_y, attributes_for_entry(entry, i + first_entry));
			}
		}
		else
		{
			auto attribute = (current_selection == 0) ? CharacterAttribute::Inverted : CharacterAttribute::None;
			drawer->draw_text("..", filename_x - 1, first_y, attribute);
			for (size_t i = 1; i < current_directory_entries.size() && i < file_display_count; i++)
			{
				auto& entry = current_directory_entries[i];
				attribute = (i == current_selection) ? CharacterAttribute::Inverted : CharacterAttribute::None;
				if (!entry.is_directory())
				{
					attribute |= CharacterAttribute::Dim;
				}
				drawer->draw_text(get_current_relative_path(entry).c_str(), filename_x, i + first_y, attributes_for_entry(entry, i));
			}
		}
	}
}

bool filesystem_viewer::handle_key(SDL_Keycode key)
{
	auto& entry = current_directory_entries[current_selection];
	switch (key)
	{
	case SDLK_UP:
		current_selection--;
		if (current_selection < 0)
		{
			current_selection = current_directory_entries.size() - 1;
		}
		return true;

	case SDLK_DOWN:
		current_selection++;
		if (current_selection >= current_directory_entries.size())
		{
			current_selection = 0;
		}
		return true;

	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		if (entry.is_directory())
		{
			current_path = entry.path();
			determine_current_directory_entries();
		}
		else if (entry.is_regular_file())
		{
			if (params.selection == selection_mode::any_file 
				|| (params.selection == selection_mode::filtered_files && std::regex_match(entry.path().filename().string(), params.file_pattern)))
			{
				params.selection_handler(entry.path());
			}
		}
		return true;

	case SDLK_DELETE:
	case SDLK_BACKSPACE:
		current_path = current_directory_entries[0].path();
		determine_current_directory_entries();
		return true;
	}

	return false;
}

void filesystem_viewer::determine_current_directory_entries()
{
	current_selection = 0;
	current_directory_entries.clear();

	current_directory_entries.push_back(std::filesystem::directory_entry(current_path.parent_path()));

	auto directory_iterator = std::filesystem::directory_iterator(current_path, std::filesystem::directory_options::skip_permission_denied);
	for (auto& entry : directory_iterator)
	{
		current_directory_entries.push_back(std::filesystem::directory_entry(entry));
	}
}

std::string filesystem_viewer::get_current_relative_path(std::filesystem::directory_entry const& entry)
{
	auto rel_path = entry.path().lexically_relative(current_path).string();
	if (entry.is_directory())
	{
		rel_path += "/";
	}

	if (rel_path.size() > min_browser_width)
	{
		auto start = rel_path.substr(0, pre_ellipsis_char_count);
		auto tail_begin = min_browser_width - pre_ellipsis_char_count - 4;
		auto tail = rel_path.substr(tail_begin);
		rel_path = start + "..." + tail;
	}

	return rel_path;
}

CharacterAttribute filesystem_viewer::attributes_for_entry(std::filesystem::directory_entry const& entry, int entry_index)
{
	auto characterAttrib = CharacterAttribute::None;

	if (entry_index == current_selection)
	{
		characterAttrib |= CharacterAttribute::Inverted;
	}

	if (!entry.is_directory() && entry.is_regular_file())
	{
		if (params.selection == selection_mode::none)
		{
			characterAttrib |= CharacterAttribute::Dim;
		}
		else if (params.selection == selection_mode::filtered_files)
		{
			if (!std::regex_match(entry.path().filename().string(), params.file_pattern))
			{
				characterAttrib |= CharacterAttribute::Dim;
			}
		}
	}

	return characterAttrib;
}

