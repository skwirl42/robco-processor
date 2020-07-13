#pragma once

#include <functional>

enum class CharacterAttribute : int
{
	None		= 0,
	Inverted	= 1 << 0,
};

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
