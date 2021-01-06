#pragma once

#include <functional>

// Notes on pseudo-graphics characters:
// The characters for a single-line box are:
// 0xDA 0xC4 0xBF
// 0xB3 0x20 0xB3
// 0xC0 0xC4 0xD9
//
// Unicode:
// ┌─┐
// │ │
// └─┘
//
// 0xB2 is the vertical box drawing character, 0xC4 the horizontal one
//
// Double-line box:
// 0xC9 0xCD 0xBB
// 0xBA 0x20 0xBA
// 0xC8 0xCD 0xBC
//
// Unicode:
// ╔═╗
// ║ ║
// ╚═╝
// 
// Characters 0xE0-0xEF contain 2x2 grids of pseudo-pixels per character.
// The lower 4 bits define which of the 2x2 pseudo-pixels are active,
// where the top left's index is 0, going right-to-left, top-to-bottom.
//
// The Unicode versions of some of these range from U+2596 to U+259F,
// whereas 0xE0 is blank, and the half-block versions are scattered between
// U+2580 and U+2590, although the two sets do not line up visually.
// ' ' '▘' '▝' '▀' '▖' '▌' '▞' '▛' '▗' '▚' '▐' '▜' '▄' '▙' '▟' '█'
// 
// This is assuming the font being used contains these characters

enum class CharacterAttribute : unsigned int
{
	None		= 0,
	Inverted	= 1 << 0,
	Dim			= 1 << 1,

	Dirty		= 1 << 16,
};

inline CharacterAttribute operator | (CharacterAttribute lhs, CharacterAttribute rhs)
{
	return (CharacterAttribute)((unsigned int)lhs | (unsigned int)rhs);
}

inline CharacterAttribute operator & (CharacterAttribute lhs, CharacterAttribute rhs)
{
	return (CharacterAttribute)((unsigned int)lhs & (unsigned int)rhs);
}

inline CharacterAttribute operator ~ (CharacterAttribute rhs)
{
	return (CharacterAttribute)~((unsigned int)rhs);
}

class Console
{
public:
	Console(int width, int height);
	~Console();

	int GetWidth() { return width; }
	int GetHeight() { return height; }

	void SetChar(int x, int y, char character);
	char GetChar(int x, int y);

	bool SetCursor(int x, int y);
	void GetCursor(int &x, int &y);

	void PrintChar(const char character);
	void PrintLine(const char *text);
	void Print(const char *text);
	void PrintLineAt(const char *text, int x, int y);
	void PrintAt(const char *text, int x, int y);

	void NewLine(int afterLine);

	void SetAttribute(CharacterAttribute attribute, int x, int y);
	void SetAttributeAtCursor(CharacterAttribute attribute);
	void SetCurrentAttribute(CharacterAttribute attribute);
	CharacterAttribute GetAttribute(int x, int y);

	void Clear();

	void Visit(std::function<void(int, int, char, CharacterAttribute)> visitor);

private:
	void Reset();

private:
	int width;
	int height;
	int cursorX;
	int cursorY;
	int bufferSize;
	char *buffer;
	CharacterAttribute *attributeBuffer;
	CharacterAttribute currentAttribute;
};
