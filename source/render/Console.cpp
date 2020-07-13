#include "Console.h"

#include <cstring>
#include <functional>
#include <stdio.h>

Console::Console(int width, int height) : buffer(nullptr), attributeBuffer(nullptr)
{
	this->width = width;
	this->height = height;
	Reset();
}

Console::~Console()
{
	delete [] buffer;
	delete [] attributeBuffer;
}

void Console::SetChar(int x, int y, char character)
{
	if (x >= width || y >= height)
	{
		return;
	}

	int position = y * width + x;
	buffer[position] = character;
	attributeBuffer[position] = currentAttribute;
}

char Console::GetChar(int x, int y)
{
	if (x >= width || y >= height)
	{
		return -1;
	}

	return buffer[y * width + x];
}

bool Console::SetCursor(int x, int y)
{
	if (x >= width || y >= height)
	{
		return false;
	}

	cursorX = x;
	cursorY = y;

	return true;
}

void Console::GetCursor(int &x, int &y)
{
	x = cursorX;
	y = cursorY;
}

void Console::PrintChar(const char character)
{
	if (character == '\n' || character == '\r')
	{
		NewLine(cursorY);
		return;
	}

	int position = cursorY * width + cursorX;
	buffer[position] = character;
	attributeBuffer[position] = currentAttribute;
	cursorX++;

	if (cursorX >= width)
	{
		cursorX = 0;
		cursorY++;
	}

	if (cursorY >= height)
	{
		cursorY = 0;
	}
}

void Console::PrintLine(const char *text)
{
	PrintLineAt(text, cursorX, cursorY);
}

void Console::Print(const char *text)
{
	PrintAt(text, cursorX, cursorY);
}

void Console::PrintLineAt(const char *text, int x, int y)
{
	PrintAt(text, x, y);
	NewLine(y);
}

void Console::PrintAt(const char *text, int x, int y)
{
	if (x >= width || y >= height)
	{
		// How am I gonna handle error returns?
		printf("oops\n");
		return;
	}

	int position = y * width + x;
	int stringLength = std::strlen(text);
	int finalPosition = position + stringLength;

	if (finalPosition > bufferSize)
	{
		// Truncate the string if it'd go past the end of the buffer
		printf("Truncating string\n");
		stringLength = bufferSize - stringLength;
		finalPosition = bufferSize;
	}

	std::strncpy(&buffer[position], text, stringLength);

	for (int i = position; i < finalPosition; i++)
	{
		attributeBuffer[i] = currentAttribute;
	}

	cursorY = (finalPosition / width) % height;
	cursorX = finalPosition % width;

	auto lastChar = text[stringLength-1];
	if (lastChar == '\n' || lastChar == '\r')
	{
		NewLine(cursorY);
	}
}

void Console::NewLine(int afterLine)
{
	cursorX = 0;
	cursorY = (afterLine + 1) % height;
}

void Console::SetAttribute(CharacterAttribute attribute, int x, int y)
{
	if (x >= width || y >= height)
	{
		return;
	}

	attributeBuffer[y * width + x] = attribute;
}

void Console::SetAttributeAtCursor(CharacterAttribute attribute)
{
	SetAttribute(attribute, cursorX, cursorY);
}

void Console::SetCurrentAttribute(CharacterAttribute attribute)
{
	currentAttribute = attribute;
}

CharacterAttribute Console::GetAttribute(int x, int y)
{
	if (x >= width || y >= height)
	{
		return CharacterAttribute::None;
	}

	return attributeBuffer[y * width + x];
}

void Console::Visit(std::function<void(int, int, char, CharacterAttribute)> visitor)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int position = y * width + x;
			visitor(x, y, buffer[position], attributeBuffer[position]);
		}
	}
}

void Console::Clear()
{
	std::memset(buffer, 0, bufferSize);
	std::memset(attributeBuffer, 0, bufferSize * sizeof(CharacterAttribute));
	currentAttribute = CharacterAttribute::None;
	cursorX = 0;
	cursorY = 0;
}

void Console::Reset()
{
	if (buffer != nullptr)
	{
		delete [] buffer;
	}

	bufferSize = width * height;

	buffer = new char[bufferSize];

	if (attributeBuffer != nullptr)
	{
		delete [] attributeBuffer;
	}

	attributeBuffer = new CharacterAttribute[bufferSize];
	Clear();
}
