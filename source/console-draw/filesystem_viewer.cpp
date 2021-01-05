#include "filesystem_viewer.hpp"

#include "util.hpp"
#include <iostream>

namespace
{
	const int min_browser_width = 30;
	const int min_info_panel_width = 10;
	const int max_info_panel_width = 15;
	const size_t pre_ellipsis_char_count = 3;
}

filesystem_viewer::filesystem_viewer(std::filesystem::path& starting_path, int x, int y, int width, int height)
	: current_path(std::filesystem::absolute(starting_path)), drawable(x, y, width, height)
{
	// The extra 2 in here is for the border box
	if (width < min_browser_width + min_info_panel_width + 2)
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
	drawer->draw_box(box_type::double_line, fill_mode::clear, x, y, width, height);

	int remaining_width = width - 2 - min_browser_width;

	if (remaining_width > min_browser_width)
	{
		int extra_max = max_info_panel_width - min_info_panel_width;
		int extra_info_panel_width = min(remaining_width - min_browser_width, extra_max);
		remaining_width += extra_info_panel_width;
	}

	int browser_width = width - 2 - remaining_width;

	// Draw the divider as a single vertical line that joins up to the overall box frame
	int divider_x = x + width - remaining_width + 1;
	drawer->draw_text("\xD1", divider_x, y, false);
	drawer->set_rect('\xB3', divider_x, y + 1, 1, height - 2);
	drawer->draw_text("\xCF", divider_x, y + height - 1, false);

	auto current_path_string = current_path.string();
	if (current_path_string.size() > remaining_width)
	{
		auto excess_size = current_path_string.size() - remaining_width;
		current_path_string = current_path_string.substr(excess_size + 3 + 2 + 1);
		current_path_string = std::string("...") + current_path_string;
	}
	drawer->draw_text(current_path_string.c_str(), x + 3, y, true);

	if (current_directory_entries.size() > 0)
	{
		int filename_x = x + 2;
		int last_filename_y = y + height - 3;
		int first_y = y + 2;
		int file_display_count = height - 4;

		if (file_display_count < current_directory_entries.size() && current_selection > file_display_count - 1)
		{
			int first_entry = current_selection - file_display_count + 1;
			for (size_t i = 0; i < file_display_count; i++)
			{
				drawer->draw_text(get_current_relative_path(current_directory_entries[i + first_entry]).c_str(), filename_x, i + first_y, i + first_entry == current_selection);
			}
		}
		else
		{
			drawer->draw_text("..", filename_x - 1, first_y, current_selection == 0);
			for (size_t i = 1; i < current_directory_entries.size() && i < file_display_count; i++)
			{
				drawer->draw_text(get_current_relative_path(current_directory_entries[i]).c_str(), filename_x, i + first_y, i == current_selection);
			}
		}
	}
}

bool filesystem_viewer::handle_key(SDL_Keycode key)
{
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
		if (current_directory_entries[current_selection].is_directory())
		{
			current_path = current_directory_entries[current_selection].path();
			determine_current_directory_entries();
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

