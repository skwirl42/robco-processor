#pragma once

#include <filesystem>
#include <regex>
#include <functional>

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

#include "drawable.hpp"

class filesystem_viewer : public drawable
{
public:
	enum class selection_mode
	{
		none,
		any_file,
		filtered_files,
	};

	struct init_params
	{
		std::filesystem::path starting_path;
		rect bounds;
		selection_mode selection;
		std::regex file_pattern;
		std::function<void(std::filesystem::path selected_file)> selection_handler;
	};

public:
	filesystem_viewer(init_params& params);

	// Inherited via drawable
	virtual void draw(drawer* drawer) override;
	bool handle_key(SDL_Keycode key);

protected:
	void determine_current_directory_entries();
	std::string get_current_relative_path(std::filesystem::directory_entry const& entry);
	CharacterAttribute attributes_for_entry(std::filesystem::directory_entry const& entry, int entry_index);

protected:
	init_params params;
	std::vector<std::filesystem::directory_entry> current_directory_entries;
	std::filesystem::path current_path;
	std::filesystem::path current_focused_entry;
	int current_selection = 0;
};