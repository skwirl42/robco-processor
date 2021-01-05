#pragma once

#include <filesystem>

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

#include "drawable.hpp"

class filesystem_viewer : public drawable
{
public:
	filesystem_viewer(std::filesystem::path& starting_path, int x, int y, int width, int height);

	// Inherited via drawable
	virtual void draw(drawer* drawer) override;
	bool handle_key(SDL_Keycode key);

protected:
	void determine_current_directory_entries();
	std::string get_current_relative_path(std::filesystem::directory_entry const& entry);

protected:
	std::vector<std::filesystem::directory_entry> current_directory_entries;
	std::filesystem::path current_path;
	std::filesystem::path current_focused_entry;
	int current_selection = 0;
};